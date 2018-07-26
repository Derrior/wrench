/**
 * Copyright (c) 2017. The WRENCH Team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <iostream>
#include "wrench/util/MessageManager.h"


namespace wrench {

    // TODO: At some point, we may want to make this with only unique pointers...

    std::map<std::string, std::vector<SimulationMessage *>> MessageManager::mailbox_messages = {};

    /**
     * @brief Insert a message in the manager's  "database"
     * @param mailbox: the name of the relevant mailbox
     * @param msg: the message
     * @throw std::runtime_error
     */
    void MessageManager::manageMessage(std::string mailbox, SimulationMessage *msg) {
      if (mailbox=="file_reception_13") {
        std::cerr << "received a message " << msg->name << "\n";
      }
      if (msg == nullptr) {
        throw std::runtime_error(
                "MessageManager::manageMessage()::Null Message cannot be saved by MessageManager"
        );
      }
      if (mailbox_messages.find(mailbox) == mailbox_messages.end()) {
        mailbox_messages.insert({mailbox, {}});
      }
      mailbox_messages[mailbox].push_back(msg);
    }

    /**
     * @brief Clean up messages for a given mailbox (so as to free up memory)
     * @param mailbox: the mailbox name
     */
    void MessageManager::cleanUpMessages(std::string mailbox) {
      std::map<std::string, std::vector<SimulationMessage *>>::iterator msg_itr;
      for (msg_itr = mailbox_messages.begin(); msg_itr != mailbox_messages.end(); msg_itr++) {
        if ((*msg_itr).first == mailbox) {
          for (size_t i = 0; i < (*msg_itr).second.size(); i++) {
            if ((*msg_itr).second[i] != nullptr) {
              delete (*msg_itr).second[i];
            }
          }
          (*msg_itr).second.clear();
          break;
        }
      }
    }

    /**
     * @brief Clean up all the messages that MessageManager has stored (so as to free up memory)
     */
    void MessageManager::cleanUpAllMessages() {
      std::map<std::string, std::vector<SimulationMessage *>>::iterator msg_itr;
      for (msg_itr = mailbox_messages.begin(); msg_itr != mailbox_messages.end(); msg_itr++) {
        for (size_t i = 0; i < (*msg_itr).second.size(); i++) {
          if ((*msg_itr).second[i] != nullptr) {
            delete (*msg_itr).second[i];
          }
        }
        if ((*msg_itr).second.size() > 0) {
          (*msg_itr).second.clear();
        }
      }
    }

    /**
     * @brief Remove a received message from the "database" of messages
     * @param mailbox: the name of the mailbox from which the message was received
     * @param msg: the message
     */
    void MessageManager::removeReceivedMessages(std::string mailbox, SimulationMessage *msg) {
      std::map<std::string, std::vector<SimulationMessage *>>::iterator msg_itr;
      for (msg_itr = mailbox_messages.begin(); msg_itr != mailbox_messages.end(); msg_itr++) {
        if ((*msg_itr).first == mailbox) {
          std::vector<SimulationMessage *>::iterator it;
          for (it = (*msg_itr).second.begin(); it != (*msg_itr).second.end(); it++) {
            if ((*it) == msg) {
              it = (*msg_itr).second.erase(it);
              break;
            }
          }
        }
      }
    }
}
