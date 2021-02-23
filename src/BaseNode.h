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

#if !defined(BASENODE_H)
#define BASENODE_H

#include <omnetpp.h>
#include <vector>
#include "Status.h"
#include "MsgKind.h"

class BaseNode : public omnetpp::cSimpleModule {
protected:
  /** @brief A timer to schedule events on this node */
  omnetpp::cMessage* timer;
  /** @brief A message that must be instanced only when a node creates a message */
  omnetpp::cMessage* msg;
  /** @brief The time at which an experiment starts */
  omnetpp::simtime_t startTime;
  /** @brief The current status of this node */
  Status status;
protected:
  /** @brief Broadcasts a message to N(x) 
   *  @param first - a valid pointer to a message
   *  @return a null pointer to the received message
  */
  omnetpp::cMessage* localBroadcast(omnetpp::cMessage*);
  /** @brief Broadcasts a message to N(x) - {senderID}
   *  @param first - a valid pointer to a message
   *  @param second - the ID of the sender
   *  @return a null pointer to the received message
  */
  omnetpp::cMessage* localFlooding(omnetpp::cMessage*);
  /** @brief Multicasts a message to a subset of N(x)
   *  @param first - a valid pointer to a message
   *  @param second - a vector holding the IDs of receivers
   *  @return a null pointer to the received message
  */
  omnetpp::cMessage* localMulticast(
    omnetpp::cMessage*, const std::vector<int>&
  );
  /** @brief Displays a string in the simulation canvas */
  virtual void displayInfo(const char* info) const{
    getDisplayString().setTagArg("t", 0, info);
  }
  /** @brief Changes the color of the info string by a standard HTML color */
  virtual void changeInfoColor(const char* color) const{
    getDisplayString().setTagArg("t", 2, color);
  }
  /** @brief A warning message stating a node executes nil */
  virtual void nil(omnetpp::cMessage* msg) {
    EV_WARN << "Undefinded action, assuming (" << status.str() << ", " 
            << msg->getName() << ") -> nil" << '\n';
  }
  /** @brief Changes width of an edge 
   *  @param first - The number of port to access the link
   *  @param second - The width of the edge
  */
  virtual void changeEdgeWidth(int, int);
  /** @brief Changes the color of an edge 
   *  @param first - The number of port to access the link
   *  @param second - The name of a HTML standard color
  */
  virtual void changeEdgeColor(int, const char*);
public:
  /** @brief Default constructor */
  BaseNode() : timer(nullptr), msg(nullptr), startTime(0.0), status() { }
  /** @brief Default destructor which tries to delete the timer */
  virtual ~BaseNode() { cancelAndDelete(timer); }
  /** @brief Set the initial status of protocols according to its role */
  virtual void initialize() = 0;
  /** @brief Implement here the protocol actions */
  virtual void handleMessage(omnetpp::cMessage*) = 0;
};

#endif // BASENODE_H
