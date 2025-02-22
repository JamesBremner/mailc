/**
 * @file
 */

#include "socket.h"

std::tuple<bool, std::string> Socket::create(std::string hostname, int port) {
  ssl = NULL;

  struct hostent *host;
  struct sockaddr_in addr;

  if ((host = gethostbyname(hostname.c_str())) == NULL) {
    return std::make_tuple(false, "DNS failed.");
  }

  sockid = socket(AF_INET, SOCK_STREAM, 0);
  if (sockid == -1) {
    // TODO Handle this error gracefully
    return std::make_tuple(false, "Socket unable to create.");
  }

  // memset(&addr, '\0', sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = *(long *)(host->h_addr);

  if (connect(sockid, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    close(sockid);
    return std::make_tuple(false, "Socket connect failed.");
  }

  // fcntl(sockid, F_SETFL, O_NONBLOCK);

  return std::make_tuple(true, "");
}

std::tuple<bool, std::string> Socket::createSSL() {
  OpenSSL_add_ssl_algorithms();
  SSL_load_error_strings();

  const SSL_METHOD *meth = SSLv23_method();
  ctx = SSL_CTX_new(meth);

  if (ctx == NULL) {
    return std::make_tuple(false, "OpenSSL SSL_CTX_new failed.");
  }

  char buf[1024];

  X509 *cert;
  char *str;

  ssl = SSL_new(ctx);
  SSL_set_fd(ssl, sockid);

  if (SSL_connect(ssl) == -1) {
    ERR_print_errors_fp(stderr);
  } else {
    // char *msg = "Hello???";

    printf("Connected with %s encryption\n", SSL_get_cipher(ssl));
    {
      cert = SSL_get_peer_certificate(ssl);  // How to get ssl?
      char *line;

      if (cert != NULL) {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line); /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);      /* free the malloc'ed string */
        X509_free(cert); /* free the malloc'ed certificate copy */
      } else {
        return std::make_tuple(false, "No certificate.");
      }
    }

    std::string reply = "";

    int err = 0;

    do {
      err = SSL_read(ssl, buf, sizeof(buf) - 1);
      if (err < 0) return std::make_tuple(false, "SSL_read failed.");
      buf[err] = '\0';
      reply += std::string(buf);
    } while (err == sizeof(buf) - 1);

    // std::cerr << "Received: " << reply << std::endl;

    SSL_set_read_ahead(ssl, 1);
  }

  createThread();

  return std::make_tuple(true, "");
}

Socket::~Socket() {
  // join = true;
  receiver.detach();
  SSL_shutdown(ssl);
  close(sockid);
  SSL_CTX_free(ctx);
  SSL_free(ssl);
}

void Socket::createThread() {
  using namespace std::chrono_literals;
  receiver = std::thread([this]() {
    char buf[1024];
    std::string reply = "";
    while (true) {
      int err = SSL_read(ssl, buf, sizeof(buf) - 1);
      if (err < 0) {
        std::cout << "SSL_read failed" << std::endl;
        return;
      }
      buf[err] = '\0';
      reply += std::string(buf);
      // std::cerr << std::string(buf) << std::string(buf).size() << std::endl;
      mtx.lock();
      do {
        std::size_t found = reply.find("\r\n");
        if (found != std::string::npos) {
          messages.push(reply.substr(0, found));
          reply = reply.substr(found + 2);
        } else
          break;
      } while (true);
      mtx.unlock();
    }
    // std::cerr << "Thread complete" << std::endl;
  });
}

bool Socket::send(const std::string &s) {
  // std::cerr << s << std::endl;
  // if (ssl == NULL) std::cerr << "IAMNULL" << std::endl;
  int err = SSL_write(ssl, s.c_str(), s.size());
  if (err == -1) {
    ERR_print_errors_fp(stderr);
    return false;
  }
  return true;
}

std::string Socket::receive() {
  std::string msg;
  while (true) {
    mtx.lock();
    if (!messages.empty()) {
      msg = messages.front();
      messages.pop();
      // std::cerr << msg << msg.size() << std::endl;
      break;
    }
    mtx.unlock();
  }
  mtx.unlock();
  return msg + "\r\n";
}

std::string Socket::receive(std::regex rgx) {
  std::string msg = "";
  while (true) {
    std::string line = receive();
    msg += line;
    line = line.substr(0, line.size() - 2);
    // std::cerr << "LINE: " << line << std::endl;
    if (std::regex_match(line, rgx)) {
      // std::cerr << "Regex Match: " << line << std::endl;
      break;
    }
  }
  return msg;
}
