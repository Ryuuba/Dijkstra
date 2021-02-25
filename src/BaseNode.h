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
#include <unordered_map>
#include <functional>
#include <memory>

#include "Enabler.h"
#include "BaseAction.h"

class BaseNode : public omnetpp::cSimpleModule {
private:
  /** @brief An event causing this node wakes up */
  ImpulsePtr wakeUp;
  /** @brief A timer that rings after sometime. It is set by the setTimer()
   *  method */
  TimeoutPtr timeout; 
  /** @brief This object is used tu add protocol rules */
  Enabler pair;
  /** @brief The set of rules B(x) this node obeys. The structure of a rule is:
   *  (status, event) -> action. The elements of the pair (status, event) 
   *  corresponds to objects of kind Status and EventKind, properly. Note that
   *  both the spontaneous impulse and the ringing of a timer are produced by 
   *  self-messages, thus the second element (the event) is a message. The 
   *  action is a functor that the user of this class must register in 
   *  the initialize method as follows:
   * 
   *  protocol[enabler] = action,
   * 
   *  where the enabler is an object of kind Enabler and action is a functor
   *  extending the abstract class Action. Each action must be defined by the 
   *  user of this class in the Action.cc file
   * 
   */
  std::unordered_map<Enabler, std::shared_ptr<BaseAction>, EnablerHasher> protocol;
protected:
  /** @brief The current status of this node */
  Status status;
  /** @brief The name of the output port */
  const char* out = "port$o";
public:
  /** @brief Default constructor */
  BaseNode() : wakeUp(nullptr), timeout(nullptr), pair(), status() { }
  /** @brief Default destructor which tries to delete 
   *  the event "spontaneously" */
  virtual ~BaseNode() { 
    cancelAndDelete(wakeUp); 
    cancelAndDelete(timeout);
  }
  /** @brief Sets the initial status of protocols according to its role. In 
   *  addition, records the rules this node obeys.
   */
  virtual void initialize() = 0;
  /** @brief Invokes the action corresponding to a given (status, event) pair.
   *  If the action is undefined, then nil is invoke.
   */
  virtual void handleMessage(omnetpp::cMessage*);
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
  virtual void changeInfoColor(const char* color) const {
    getDisplayString().setTagArg("t", 2, color);
  }
  /** @brief A warning message stating a node executes nil */
  virtual void nil(omnetpp::cMessage* ev) {
    EV_WARN << "Undefinded action, assuming (" << status.str() << ", " 
            << ev->getName() << ") -> nil" << ", deleting object.\n";
    delete ev;
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
  /** @brief Spontaneously, gets up this node. Wake-up time is set in 
   *  omnetpp.ini (parameter startTime). By default, nodes spontaneously wake
   *  up at t = 0 s.
  */
  virtual void spontaneously();
  /** @brief Sets a timer 
   *  @param first - The time to trigger a timeout event from this moment
  */
  virtual void setTimer(omnetpp::simtime_t);
  /** @brief Adds new rule to the protocol this node obeys. In order to add
   *  an action, use the New_Action macro since this method needs a shared
   *  pointer pointing to a valid action functor.
  */
  virtual void addRule(const Status&, EventKind ev, const std::shared_ptr<BaseAction>&); 
};

#endif // BASENODE_H
