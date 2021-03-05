#include "BaseNode.h"

Register_Abstract_Class(BaseNode);

omnetpp::cMessage* BaseNode::localBroadcast(omnetpp::cMessage* msg) {
  int n = gateSize(out);
  if (msg && n > 0) {
    for (int i = 0; i < n -1; i++)
      send(msg->dup(), out, i);
    send(msg, out, n-1);
  }
  return msg;
}

omnetpp::cMessage* BaseNode::localFlooding(omnetpp::cMessage* msg) {
  if (msg->getArrivalGate()) {
    int senderID = msg->getArrivalGate()->getIndex();
    int n = gateSize(out);
    if (msg && n > 0) {
      for (int i = 0; i < n; i++)
        if (i != senderID)
          send(msg->dup(), out, i);
      delete msg;
    }
    return msg;
  }
  else {
    omnetpp::cRuntimeError("localFlooding: This message has not been sent previosly, not sender to be discarded\n");
    delete msg;
  }
  return msg;
}

omnetpp::cMessage* BaseNode::localMulticast(
  omnetpp::cMessage* msg,
  const std::vector<int>& destination
) {
  if (msg) {
    if (!destination.empty()) {
      for (auto&& port : destination)
        send(msg->dup(), out, port);
      delete msg;
    }
    else
      delete msg;
  }
  return msg;
}

void BaseNode::changeEdgeColor(int p, const char* color) const {
  gate(out, p)->getChannel()->getDisplayString().setTagArg("ls", 0, color);
}

void BaseNode::changeEdgeWidth(int p, int width) const {
  gate(out, p)->getChannel()->getDisplayString().setTagArg("ls", 1, width);
}
void BaseNode::setEdgeDotted(int p) const {
  gate(out, p)->getChannel()->getDisplayString().setTagArg("ls", 2, "d");
}

void BaseNode::setEdgeDashed(int p) const {
  gate(out, p)->getChannel()->getDisplayString().setTagArg("ls", 2, "da");
}

void BaseNode::setEdgeSolid(int p) const {
  gate(out, p)->getChannel()->getDisplayString().setTagArg("ls", 2, "s");
}

void BaseNode::spontaneously() {
  wakeUp = new omnetpp::cMessage("spontaneuosly", EventKind::IMPULSE);
  scheduleAt(par("startTime"), wakeUp);
}

void BaseNode::setTimer(omnetpp::simtime_t t) {
  if (!timeout)
    timeout = new omnetpp::cMessage("timer", EventKind::TIMEOUT);
  if (!timeout->isScheduled())
    scheduleAt(omnetpp::simTime() + t, timeout);
  else
    EV_ERROR << "The timer is already scheduled\n";
}

void BaseNode::handleMessage(omnetpp::cMessage* ev) {
  EventKind event = static_cast<EventKind>(ev->getKind());
  pair.set(status, event);
  auto it = protocol.find(pair);
  if (it != protocol.end()) {
  EV_INFO << "Node[" << getIndex() << "] meets rule ("
          << it->first.getStatus().str() << ", " 
          << ev->getName() << ") -> "
          << it->second->getName() << '\n';
    (*protocol[pair])(ev);
  }
  else
    nil(ev);
}

void BaseNode::addRule(
  const Status& s, 
  EventKind e,
  const std::shared_ptr<BaseAction>& action
) {
  pair.set(s, e);
  protocol[pair] = action;
}

int BaseNode::getLinkWeight(const char* name, int index) {
  auto edge = dynamic_cast<Edge*>(gate(name, index)->getChannel());
  return edge ->getWeight();
}

int BaseNode::getLinkWeight(omnetpp::cGate* gate, int index) {
  auto edge = dynamic_cast<Edge*>(gate);
  return edge->getWeight();
}

void BaseNode::initializeNeighborhood() {
  neighborhoodSize = gateSize(out);
  for (int i = 0; i < neighborhoodSize; i++)
    neighborhood.push_back(gate(out, i));
}