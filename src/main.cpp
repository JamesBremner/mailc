/**
 * @file
 */

#include "cliutils.h"
#include "imap.h"
#include "smtp.h"

/**
 * @brief Vocabulory that will be used by readline
 */
std::vector<std::string> vocabulory{"help",   "send",     "quit",   "read",
                                    "search", "delete",   "sync",   "list",
                                    "create", "deletemb", "rename", "move",
                                    "noop"};


/**
 * @brief The mail functiom
 *
 * @param argc Number of cli arguments
 * @param argv List of cli arguments
 *
 * @return Returns 0 on exit
 */
int main(int argc, char **argv) {


  std::cout << "IMAP SMTP Mail Client" << std::endl;
  config config{993,
                465,
                "imap.zoho.com",
                "smtp.zoho.com",
                "test@prateekkumar.in",
                "testtest",
                "Prateek Test"};

  IMAPConnection imap(config.imap_server, config.imap_port);
  if (imap.login(config.username, config.password)) {
    std::cout << "Login successful" << std::endl;
  }

  std::string cmd;

    if( argc != 2 ) {
      // default to send command
    cmd = "send";
} else
{
  cmd = argv[1];
}

    std::stringstream scmd(cmd);
    scmd >> cmd;
    if (cmd == "help") {
      std::cout << "help\t\tDisplay this list" << std::endl;
      std::cout << "send\t\tSend an email" << std::endl;
      std::cout << "quit\t\tQuit the program" << std::endl;
      std::cout << "read\t\tRead mail from a mailbox" << std::endl;
      std::cout << "search\t\tSearch for mails" << std::endl;
      std::cout << "delete\t\tDelete mail from a mailbox" << std::endl;
      std::cout << "sync\t\tSync the folders and mails" << std::endl;
      std::cout << "list\t\tList mailboxes" << std::endl;
      std::cout << "create\t\tCreate a new mailbox" << std::endl;
      std::cout << "deletemb\tDelete an existing mailbox" << std::endl;
      std::cout << "rename\t\tRename a mailbox" << std::endl;
      std::cout << "move\t\tMove a mail to another mailbox" << std::endl;
      std::cout << "noop\t\tKeep connection open to server" << std::endl;
    } else if (cmd == "quit") {
      exit( 0 );
    } else if (cmd == "list") {
      auto mailboxes = cliutils::getMailboxes();
      for (const auto &mailbox : mailboxes) {
        std::cout << mailbox << std::endl;
      }
    } else if (cmd == "create") {
      std::string mailbox;
      std::cout << "mailbox> ";
      std::getline(std::cin, mailbox);
      if (cliutils::createMailbox(imap, mailbox)) {
        std::cout << "Created mailbox" << std::endl;
      } else {
        std::cout << "Mailbox could not be created" << std::endl;
      }
    } else if (cmd == "deletemb") {
      std::string mailbox;
      std::cout << "mailbox> ";
      std::getline(std::cin, mailbox);
      if (cliutils::deleteMailbox(imap, mailbox)) {
        std::cout << "Deleted mailbox" << std::endl;
      } else {
        std::cout << "Mailbox could not be deleted" << std::endl;
      }
    } else if (cmd == "rename") {
      std::string old_mailbox, new_mailbox;
      std::cout << "old_mailbox> ";
      std::getline(std::cin, old_mailbox);
      std::cout << "new_mailbox> ";
      std::getline(std::cin, new_mailbox);
      if (cliutils::renameMailbox(imap, old_mailbox, new_mailbox)) {
        std::cout << "Renamed mailbox" << std::endl;
      } else {
        std::cout << "Mailbox could not be renamed" << std::endl;
      }
    } else if (cmd == "search") {
      std::string mailbox, from, to, subject, text, nottext, since, before;
      std::cout << "mailbox [inbox]> ";
      std::getline(std::cin, mailbox);
      if (mailbox.size() == 0) mailbox = "inbox";
      std::cout << "from (optional)> ";
      std::getline(std::cin, from);
      std::cout << "to (optional)> ";
      std::getline(std::cin, to);
      std::cout << "subject (optional)> ";
      std::getline(std::cin, subject);
      std::cout << "text (optional)> ";
      std::getline(std::cin, text);
      std::cout << "not_text (optional)> ";
      std::getline(std::cin, nottext);
      std::cout << "since (optional)> ";
      std::getline(std::cin, since);
      std::cout << "before (optional)> ";
      std::getline(std::cin, before);
      std::vector<int> mailuids = cliutils::searchMails(
          imap, mailbox, from, to, subject, text, nottext, since, before);
      auto mails = cliutils::getMails(imap, mailbox, mailuids);
      std::cout << mails.size() << " matching mails found." << std::endl;
      for (const auto &mail : mails) {
        if (mail.uid >= 0) {
          std::cout << mail.uid << "\t" << mail.from << "\t\t" << mail.to
                    << "\n"
                    << "\t" << mail.subject << std::endl;
        }
      }
    } else if (cmd == "read") {
      std::string mailbox, uid_str;
      std::cout << "mailbox [inbox]> ";
      std::getline(std::cin, mailbox);
      if (mailbox.size() == 0) mailbox = "inbox";
      std::cout << "mailuid> ";
      std::getline(std::cin, uid_str);
      Mail mail = cliutils::readMail(mailbox, std::stoi(uid_str));
      if (mail.uid >= 0) {
        std::cout << "From: " << mail.from << std::endl;
        std::cout << "To: " << mail.to << std::endl;
        std::cout << "Subject: " << mail.subject << std::endl;
        std::cout << "Date: " << mail.date << std::endl;
        std::cout << "Body: " << std::endl;
        std::cout << mail.text << std::endl;
      } else {
        std::cout << "Mail not found." << std::endl;
      }
    } else if (cmd == "delete") {
      std::string mailbox, uid_str;
      std::cout << "mailbox [inbox]> ";
      std::getline(std::cin, mailbox);
      if (mailbox.size() == 0) mailbox = "inbox";
      std::cout << "mailuid> ";
      std::getline(std::cin, uid_str);
      if (cliutils::deleteMail(imap, mailbox, std::stoi(uid_str))) {
        std::cout << "Deleted." << std::endl;
      } else {
        std::cout << "Error deleting!" << std::endl;
      }
    } else if (cmd == "send") {
      std::string to, subject, body, line;
      std::cout << "to> ";
      std::getline(std::cin, to);
      std::cout << "subject> ";
      std::getline(std::cin, subject);
      std::cout << "body> (Type RETURN.RETURN to end input)" << std::endl;
      while (body.size() < 5 ||
             body.substr(body.size() - 5, 5) != "\r\n.\r\n") {
        std::getline(std::cin, line);
        body += line + "\r\n";
      }

      body = body.substr(0, body.size() - 5);

      if (cliutils::sendMail(config, to, subject, body)) {
        std::cout << "Sent mail." << std::endl;
        cliutils::sync(imap);
      } else {
        std::cout << "Mail could not be sent." << std::endl;
      }
    } else if (cmd == "noop") {
      if (cliutils::noop(imap)) {
        std::cout << "Noop successful" << std::endl;
      } else {
        std::cout << "Could not noop" << std::endl;
      }
    } else if (cmd == "sync") {
      if (cliutils::sync(imap)) {
        std::cout << "Sync complete" << std::endl;
      } else {
        std::cout << "Could not sync" << std::endl;
      }
    } else if (cmd == "move") {
      std::string oldmailbox, newmailbox, uid_str;
      std::cout << "from_mailbox> ";
      std::getline(std::cin, oldmailbox);
      std::cout << "uid> ";
      std::getline(std::cin, uid_str);
      std::cout << "to_mailbox> ";
      std::getline(std::cin, newmailbox);
      if (cliutils::moveMail(imap, oldmailbox, std::stoi(uid_str),
                             newmailbox)) {
        std::cout << "Moved mail." << std::endl;
      } else {
        std::cout << "Mail could not be moved." << std::endl;
      }

    } else {
      std::cout << "Invalid command. Type help for valid commands."
                << std::endl;
    }
 
  std::cout << std::endl;

  return 0;
}

/**
 * @brief Generates next possible command
 * @details Generates command that will be used by readline
 * to give suggestion at cli
 *
 * @param text Text input by user
 * @param state State of the generator
 *
 * @return Next possible matching command, null otherwise
 */
char *command_generator(const char *text, int state) {
  static std::vector<std::string> matches;
  static size_t match_index = 0;

  if (state == 0) {
    matches.clear();
    match_index = 0;

    std::string textstr(text);
    for (auto word : vocabulory) {
      if (word.size() >= textstr.size() &&
          word.compare(0, textstr.size(), textstr) == 0) {
        matches.push_back(word);
      }
    }
  }

  if (match_index >= matches.size()) {
    return nullptr;
  } else {
    return strdup(matches[match_index++].c_str());
  }
}

