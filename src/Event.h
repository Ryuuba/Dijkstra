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
/** @brief Represents an spontaneous impulse */
typedef omnetpp::cMessage* ImpulsePtr;
/** @brief Represents the expiration of a timer */
typedef omnetpp::cMessage* TimeoutPtr;
/** @brief Represents the reception of a message */
typedef omnetpp::cMessage* MsgPtr;

enum EventKind {
  /** @brief An spontaneous impulse */
  IMPULSE = 0,
  /** @brief The expiration of a timer */
  TIMER,
  /** @brief The reception of a query */
  QUERY,
  /** @brief The reception of a reply */
  REPLY,
  /** @brief The reception of an affirmative response */
  YES,
  /** @brief The reception of an negative response */
  NO,
  /** @brief The reception of an acknowlegdement */
  ACK,
  /** @brief The reception of a negative acknowlegdement */
  NACK,
  /** @brief The reception of a token */
  TOKEN,
  /** @brief The reception of an announcement to inform a task is completed */
  CHECK,
  /** @brief The reception of an message to inform global termination */
  TERMINATION,
  /** @brief A request to activation */
  ACTIVATE,
  /** @brief A message indicating the reception of |N(x)| - 1 messages*/
  SATURATION,
  /** @brief A message use to solve the Election problem */
  ELECTION,
  // Append other categories from here
  /** @brief A hello message */
  HELLO
};

#endif