//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef EVENT_H
#define EVENT_H

#include <omnetpp.h>
//TODO Hide Impulse and timeout messages
/** @brief Represents an spontaneous impulse */
typedef omnetpp::cMessage Impulse;
/** @brief Represents the expiration of a timer */
typedef omnetpp::cMessage Timeout;
/** @brief Represents the reception of a message */
typedef omnetpp::cMessage Msg;

enum EventKind {
  /** @brief An spontaneous impulse */
  IMPULSE = 0,
  /** @brief The expiration of a timer */
  TIMEOUT,
  /** @brief The reception of request for forwarding a REQ msg to the 
   * minimum-weight outgoing link */
  FWD,
  /** @brief The reception of a request for merger clusters */
  REQ,
  /** @brief The reception of an answer of a request */
  REPLY,
  /** @brief The reception of a query to ask for the state variables of a node */
  QUERY,
  /** @brief The reception of a positive answer of a query */
  YES,
  /** @brief The reception of an negative response */
  NO,
  /** @brief The reception of an announcement to inform a task is completed */
  CHECK,
  /** @brief The reception of an message to inform global termination */
  TERMINATION,
  /** @brief The reception of hello message */
  HELLO,
  /** @brief The reception of a message containg a local minimum */
  MIN
};

#endif