#include "MegaMerger.h"

Define_Module(MegaMerger);

MegaMerger::MegaMerger()
  : parent(nullptr)
  , level(0)
  , helloCounter(0)
{ }

void MegaMerger::initialize() {
  if (par("initiator").boolValue())
    spontaneously();
  neighborhoodSize = gateSize(out);
  status = Status::IDLE;
  addRule(Status::IDLE, EventKind::IMPULSE, New_Action(ActionOne));
  addRule(Status::IDLE, EventKind::HELLO, New_Action(ActionTwo));
  addRule(Status::UPDATING, EventKind::HELLO, New_Action(ActionThree));
}

void MegaMerger::refreshDisplay() const {
  std::string info(status.str());
  info += '\n' + std::to_string(helloCounter);
  displayInfo(info.c_str());
}

void MegaMerger::sendHello() {
  auto msg = new HelloMsg;
  msg->setLevel(level);
  msg->setId(uid);
  msg->setCid(cid);
  localBroadcast(msg);
}

void MegaMerger::processHello(MsgPtr msg) {
  auto hello = dynamic_cast<HelloMsg*>(msg);
  CacheEntry entry;
  entry[RowIndex::PORT_NUMBER] = hello->getArrivalGate()->getIndex();
  entry[RowIndex::ID] = hello->getId();
  entry[RowIndex::CID] = hello->getCid();
  entry[RowIndex::LINK_KIND] = LinkKind::UNKNOWN;
  delete msg;
}

void MegaMerger::ActionOne::operator()(ImpulsePtr impulse) {
  node->core = true;
  node->uid = node->getIndex();
  node->cid = node->getIndex();
  node->sendHello();
  node->status = Status::UPDATING;
}

void MegaMerger::ActionTwo::operator()(MsgPtr msg) {
  node->processHello(msg);
  node->core = true;
  node->uid = node->getIndex();
  node->cid = node->getIndex();
  node->sendHello();
  node->helloCounter = 1;
  if (node->helloCounter == node->neighborhoodSize)
    node->status = Status::PROCESSING;
  else
    node->status = Status::UPDATING;

}

void MegaMerger::ActionThree::operator()(MsgPtr msg) {
  node->processHello(msg);
  node->helloCounter++;
  if (node->helloCounter == node->neighborhoodSize)
    node->status = Status::PROCESSING;
}