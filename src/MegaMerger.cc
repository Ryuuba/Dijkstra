#include "MegaMerger.h"

Define_Module(MegaMerger);

MegaMerger::MegaMerger()
  : isMinimumSent(false)
  , cid(-1)
  , parent(-1)
  , level(0)
{ }

MegaMerger::~MegaMerger() {
  for (auto& ptr : pendingHelloCache)
    delete ptr;
  for (auto& ptr : externalQueryCache)
    delete ptr;
}

void MegaMerger::initialize() {
  if (par("initiator").boolValue())
    spontaneously();
  initializeNeighborhood();
  outgoingLinkNumber = neighborhoodSize;
  for (int i = 0; i < neighborhoodSize; i++) {
    CacheEntry entry;
    std::get<Index::WEIGHT>(entry) = getLinkWeight(out, i);
    std::get<Index::KIND>(entry) = LinkKind::UNKNOWN;
    neighborCache.push_back(std::move(entry));
  }
  status = Status::IDLE;
  addRule(Status::IDLE, EventKind::IMPULSE, New_Action(WakeUp));
  addRule(Status::IDLE, EventKind::HELLO, New_Action(BroadcastHello));
  addRule(Status::UPDATING, EventKind::HELLO, New_Action(UpdateCache));
  addRule(Status::UPDATING, EventKind::QUERY, New_Action(AnswerQuery));
  addRule(Status::PROCESSING, EventKind::MIN, New_Action(ComputeMin));
  addRule(Status::PROCESSING, EventKind::HELLO, New_Action(UnexpectedHello));
  addRule(Status::PROCESSING, EventKind::QUERY, New_Action(AnswerQuery));
  addRule(Status::CONNECTING, EventKind::QUERY, New_Action(Connect));
  addRule(Status::CONNECTING, EventKind::CHECK, New_Action(Expand));
  addRule(Status::CONNECTING, EventKind::TERMINATION, New_Action(Terminate));
  addRule(Status::CONNECTING, EventKind::HELLO, New_Action(UnexpectedHello));
  WATCH(core);
}

void MegaMerger::refreshDisplay() const {
  std::string info(status.str());
  info += '\n' + std::to_string(cid) + ' ' 
               + std::to_string(level) + ' ' 
               + std::to_string(outgoingLinkNumber) + '\n';
  for (auto& id : tree)
    info += std::to_string(
      std::get<Index::NID>(neighborCache[id])
    ) + ' ';
  displayInfo(info.c_str());
  for (int i = 0; i < neighborhoodSize; i++) {
    if (std::get<Index::KIND>(neighborCache[i]) == LinkKind::BRANCH) {
      changeEdgeColor(i, "teal");
      changeEdgeWidth(i, 4);
    }
    else if (std::get<Index::KIND>(neighborCache[i]) == LinkKind::INTERNAL) {
      changeEdgeColor(i, "teal");
      changeEdgeWidth(i, 2);
    }
    else if (std::get<Index::KIND>(neighborCache[i]) == LinkKind::UNKNOWN) 
      changeEdgeColor(i, "red");
    else //Outgoing link
      changeEdgeColor(i, "black");
  }
}

void MegaMerger::broadcastHello(int arrivalPort) {
  auto hello = new HelloMsg;
  hello->setName("hquery");
  hello->setLevel(level);
  hello->setId(uid);
  hello->setCid(cid);
  expectedHelloPort.clear();
  for (int i = 0; i < neighborhoodSize; i++) {
    if (
      std::get<Index::KIND>(neighborCache[i]) == LinkKind::OUTGOING ||
      std::get<Index::KIND>(neighborCache[i]) == LinkKind::UNKNOWN // start case
    )
      if (arrivalPort != i)
        expectedHelloPort.push_back(i);
  }
  localMulticast(hello, expectedHelloPort);
}

void MegaMerger::replyHello(HelloMsg* hello) {
  hello->setName("hreply");
  hello->setLevel(level);
  hello->setId(uid);
  hello->setCid(cid);
  send(hello, out, hello->getArrivalGate()->getIndex());
}

void MegaMerger::replyPendingHelloMsg() {
  auto it = pendingHelloCache.begin();
  while (it != pendingHelloCache.end()) {
    if (level >= (*it)->getLevel()) {
      (*it)->setName("hreply");
      (*it)->setLevel(level);
      (*it)->setId(uid);
      (*it)->setCid(cid);
      send((*it), out, (*it)->getArrivalGate()->getIndex());
      it = pendingHelloCache.erase(it);
    }
    else
      ++it;
  }
}

void MegaMerger::sendMin() {
  auto min = new MinMsg;
  if (outgoingPortIndex >= 0) {
    min->setWeight(std::get<Index::WEIGHT>(neighborCache[outgoingPortIndex]));
    min->setMinUid(std::get<Index::MIN_ID>(neighborCache[outgoingPortIndex]));
    min->setMaxUid(std::get<Index::MAX_ID>(neighborCache[outgoingPortIndex]));
  }
  else {
    min->setWeight(std::numeric_limits<double>::infinity());
    min->setMinUid(std::numeric_limits<int>::max());
    min->setMaxUid(std::numeric_limits<int>::max());
  }
  isMinimumSent = true;
  send(min, out, parent);
}

void MegaMerger::sendQuery(QueryMsg* msg) {
  if (msg)
    send(msg, out, outgoingPortIndex); //FIXME AnswerQuery
  else {
    auto query = new QueryMsg;
    query->setCid(cid);
    query->setLevel(level);
    query->setContactPointId(contactPointID);
    send(query, out, outgoingPortIndex);
  }
}

void MegaMerger::replyQuery(int port) {
  auto checkMsg = new CheckMsg;
  checkMsg->setLevel(level);
  checkMsg->setCid(cid);
  checkMsg->setUpdateStatus(
    (!isMinimumSent) ? true : false
  );
  send(checkMsg, out, port);
}

void MegaMerger::broadcastCheck(int discardedPort, CheckMsg* msg) {
  if (!msg) {
    msg = new CheckMsg;
    msg->setLevel(level);
    msg->setCid(cid);
    msg->setUpdateStatus(true);
  }
  for (int i = 0; i < neighborhoodSize; i++)
    if (i != discardedPort) 
      if (std::get<Index::KIND>(neighborCache[i]) == LinkKind::BRANCH)
        send(msg->dup(), out, i);
  delete msg; //FIXME Reutilize this message
}

void MegaMerger::broadcastTermination(MsgPtr termination) {
  if (!termination) {
    termination = new omnetpp::cMessage("termination", EventKind::TERMINATION);
    localMulticast(termination, tree);
  }
  else {
    for (auto& neighbor : tree)
      if (neighbor != termination->getArrivalGate()->getIndex())
        send(termination->dup(), out, neighbor);
    delete termination;
  }
}

void MegaMerger::updateNeighborCacheEntry(int portNumber, HelloMsg* hello) {
  CacheEntry entry;
  // The answer let this node know whether the link is internal or outgoing
  // The branch kind is assigned once the merging process is done
  LinkKind linkKind;
  auto it = find(tree.begin(), tree.end(), portNumber);
  if (it != tree.end())
    linkKind = LinkKind::BRANCH;
  else if (hello->getCid() == cid && level == hello->getLevel())
    linkKind = LinkKind::INTERNAL;
  else if (hello->getLevel() >= level)
    linkKind = LinkKind::OUTGOING;
  else
    linkKind = LinkKind::UNKNOWN;

  std::get<Index::KIND>(entry)   = linkKind;
  std::get<Index::NID>(entry)    = hello->getId();
  std::get<Index::CID>(entry)    = hello->getCid();
  std::get<Index::LEVEL>(entry)  = hello->getLevel();
  std::get<Index::MIN_ID>(entry) = (uid < std::get<Index::NID>(entry)) 
                                 ? uid 
                                 : std::get<Index::NID>(entry);
  std::get<Index::MAX_ID>(entry) = (uid > std::get<Index::NID>(entry)) 
                                 ? uid 
                                 : std::get<Index::NID>(entry);
  if (linkKind != std::get<Index::KIND>(neighborCache[portNumber])) {
    if (linkKind == LinkKind::INTERNAL)
      outgoingLinkNumber--;
  }
  neighborCache[portNumber] = std::move(entry);
}

void MegaMerger::startConvergecast() {
  minCounter = 0;
  contactPointID = -1;
  outgoingPortIndex = -1;
  isMinimumSent = false;
  computeOutgoingLink();
  if (children.empty()) {
    sendMin();
  }
}

// FIXME Possible bug: adding a link from a cluster with level lesser than 
// cluster's
void MegaMerger::computeOutgoingLink() {
  std::get<0>(outgoingLink) = std::numeric_limits<double>::infinity();
  std::get<1>(outgoingLink) = std::numeric_limits<int>::max();
  std::get<2>(outgoingLink) = std::numeric_limits<int>::max();
  int i = 0;
  for (auto& entry : neighborCache) {
    if (std::get<Index::KIND>(entry) == LinkKind::OUTGOING) {
      auto link = std::make_tuple(
        std::get<Index::WEIGHT>(entry),
        std::get<Index::MIN_ID>(entry),
        std::get<Index::MAX_ID>(entry)
      );
      if (link < outgoingLink) { //C++20 deprecated
        updateMinOutgoingLink(link);
        outgoingPortIndex = i;
      }
    }
    i++;
  }
}

void MegaMerger::updateMinOutgoingLink(Link& link) {
  std::get<Index::WEIGHT>(outgoingLink) = std::get<Index::WEIGHT>(link);
  std::get<Index::MIN_ID>(outgoingLink) = std::get<Index::MIN_ID>(link); 
  std::get<Index::MAX_ID>(outgoingLink) = std::get<Index::MAX_ID>(link);
}

void MegaMerger::solveMergingProcess (QueryMsg* externalQuery) {
  tree.push_back(outgoingPortIndex);
  std::get<Index::KIND>(neighborCache[outgoingPortIndex]) = LinkKind::BRANCH;
  outgoingLinkNumber--;
  //Case Fusion
  if (level == externalQuery->getLevel()) {
    level++;
    core = uid < externalQuery->getContactPointId();
    cid = (core) ? uid : externalQuery->getContactPointId();
    parent = (core) ? -1 : outgoingPortIndex;
    if (core) children.push_back(outgoingPortIndex);
  }
  // Case AnswerQuery by neighboring cluster
  else if (level < externalQuery->getLevel()) {
    level = externalQuery->getLevel();
    core = externalQuery->getCid();
    cid = core;
    parent = outgoingPortIndex;
  }
  // Case Absorb a neighboring cluster
  else
    children.push_back(outgoingPortIndex);
}

void MegaMerger::solveExternalQueries() {
  if (!externalQueryCache.empty()) {
    auto it = externalQueryCache.begin();
    while (it != externalQueryCache.end()) {
      if ((*it)->getLevel() < level) {
        int arrivalPort = (*it)->getArrivalGate()->getIndex();
        std::get<Index::KIND>(neighborCache[arrivalPort]) = LinkKind::BRANCH;
        tree.push_back(arrivalPort);
        children.push_back(arrivalPort);
        outgoingLinkNumber--;
        replyQuery(arrivalPort);
        delete (*it);
        it = externalQueryCache.erase(it);
      }
      else
        it++;
    }
  }
}

void MegaMerger::WakeUp::operator()(ImpulsePtr impulse) {
  node->core = true;
  node->uid = node->getIndex();
  node->cid = node->getIndex();
  node->broadcastHello();
  node->status = Status::UPDATING;
}

void MegaMerger::BroadcastHello::operator()(MsgPtr msg) {
  auto hello = dynamic_cast<HelloMsg*>(msg);
  int arrivalPort = hello->getArrivalGate()->getIndex();
  node->updateNeighborCacheEntry(arrivalPort, hello);
  node->core = true;
  node->uid = node->getIndex();
  node->cid = node->getIndex();
  node->replyHello(hello);
  node->broadcastHello(arrivalPort);
  if (node->expectedHelloPort.empty()) {
    node->computeOutgoingLink();
    node->contactPointID = node->uid;
    auto query = new QueryMsg;
    query->setCid(node->cid);
    query->setLevel(node->level);
    query->setContactPointId(node->uid);
    node->pendingQuery = query->dup();
    node->send(query, node->out, node->outgoingPortIndex);
    auto it = std::find_if(
      node->externalQueryCache.begin(),
      node->externalQueryCache.end(),
      [&](QueryMsg* externalQuery) -> bool {
        return node->cid == externalQuery->getCid();
      }
    );
    if (it != node->externalQueryCache.end()) {
      node->solveMergingProcess(query);
      node->replyPendingHelloMsg();
      if (node->outgoingLinkNumber == 0) {
        node->startConvergecast();
        node->status = (node->children.empty()) 
                     ? Status::CONNECTING
                     : Status::PROCESSING;
      }
      else {
        node->broadcastHello();
        node->status = Status::UPDATING;
      }
    }
    else
      node->status = Status::CONNECTING;
  }
  else
    node->status = Status::UPDATING;
}

void MegaMerger::UpdateCache::operator()(MsgPtr msg) {
  auto hello = dynamic_cast<HelloMsg*>(msg);
  int arrivalPort = hello->getArrivalGate()->getIndex();
  auto it = std::find (
    node->expectedHelloPort.begin(),
    node->expectedHelloPort.end(), 
    arrivalPort
  ); // FIXME This condition is not strong

  if (it != node->expectedHelloPort.end()) {
    node->updateNeighborCacheEntry(arrivalPort, hello);
    node->expectedHelloPort.erase(it);
    delete hello;
  }
  else if (node->level >= hello->getLevel())
    node->replyHello(hello);
  else
    node->pendingHelloCache.push_back(hello);

  if (node->expectedHelloPort.empty()) {
    if (node->level == 0) {
      node->computeOutgoingLink();
      node->contactPointID = node->uid;
      auto query = new QueryMsg;
      query->setCid(node->cid);
      query->setLevel(node->level);
      query->setContactPointId(node->uid);
      node->pendingQuery = query->dup();
      node->send(query, node->out, node->outgoingPortIndex);
      auto it = std::find_if(
        node->externalQueryCache.begin(),
        node->externalQueryCache.end(),
        [&](QueryMsg* externalQuery) -> bool {
          return node->cid == externalQuery->getCid();
        }
      );
      if (it != node->externalQueryCache.end()) {
        node->solveMergingProcess(query);
        node->replyPendingHelloMsg();
        if (node->outgoingLinkNumber == 0) {
          node->startConvergecast();
          node->status = (node->children.empty()) 
                       ? Status::CONNECTING
                       : Status::PROCESSING;
        }
        else {
          node->broadcastHello();
          node->status = Status::UPDATING;
        }
      }
      else
        node->status = Status::CONNECTING;
    }
    else {
      node->startConvergecast();
      node->status = (node->children.empty()) 
                   ? Status::CONNECTING
                   : Status::PROCESSING;
    }
  }
}

void MegaMerger::ComputeMin::operator()(MsgPtr msg) {
  auto minMsg = dynamic_cast<MinMsg*>(msg);
  auto link = std::make_tuple(
    minMsg->getWeight(),
    minMsg->getMinUid(),
    minMsg->getMaxUid()
  );
  if (link < node->outgoingLink) {
    node->updateMinOutgoingLink(link);
    node->outgoingPortIndex = minMsg->getArrivalGate()->getIndex();
    node->contactPointID = minMsg->getId();
  }
  node->minCounter++;
  if (node->minCounter == node->children.size()) {
    if (!node->core) {
      node->sendMin();
      node->status = Status::CONNECTING;
    }
    else { // Core case
      if (
        std::get<Index::WEIGHT>(node->outgoingLink) == std::numeric_limits<double>::infinity()
      ) {
        node->broadcastTermination(); //FIXME Implement this method
        node->status = Status::DONE;
      }
      else {
        node->sendQuery();
        node->status = Status::CONNECTING;
      }
    }
  }
  delete minMsg; // FIXME recycle the last min message
}

void MegaMerger::Connect::operator()(MsgPtr msg) {
  auto query = dynamic_cast<QueryMsg*>(msg);
  int arrivalPort = query->getArrivalGate()->getIndex();
  // Case Routing a query within the cluster of this node
  if (query->getCid() == node->cid) { 
    // Case This node is a contact point case
    if (query->getContactPointId() == node->uid) { 
      //looks an externalQuery up on the externalQueryCache
      auto it = find_if (
        node->externalQueryCache.begin(),
        node->externalQueryCache.end(),
        [&](QueryMsg* externalQuery) -> bool {
          return query->getCid() == externalQuery->getCid();
        }
      );
      // Case Exists a pending query matching that the core composes
      if (it != node->externalQueryCache.end()) {
        node->solveMergingProcess(*it);
        node->sendQuery(query);
        node->broadcastCheck(node->outgoingPortIndex);
        node->solveExternalQueries();
        node->replyPendingHelloMsg();
        if (node->outgoingLinkNumber == 0) {
          node->startConvergecast();
          node->status = (node->children.empty()) 
                       ? Status::CONNECTING
                       : Status::PROCESSING;
        }
        else {
          node->broadcastHello();
          node->status = Status::UPDATING;
        }
        delete *it;
        node->externalQueryCache.erase(it);
      }
      // Case Wait for a query comming from the neighboring cluster
      else { 
        node->pendingQuery = query->dup();
        node->sendQuery(query);
      }
    }
    else  // Case A intermediate node receives the query
      node->sendQuery(query);
  }
  // case receiving a query from a neighboring cluster not satisfaying
  // the pending query of the cluster of this node
  // possible situations: absortion or a future merging process
  else if (!node->pendingQuery) {
    // Case Absortion
    if (node->level > query->getLevel()) {
      std::get<Index::KIND>(node->neighborCache[arrivalPort]) = 
        LinkKind::BRANCH;
      node->tree.push_back(arrivalPort);
      node->children.push_back(arrivalPort);
      node->outgoingLinkNumber--;
      node->replyQuery(arrivalPort);
      delete query;
    }
    else // Impossible case
      node->externalQueryCache.push_back(query);
  }
  // case receiving a query from a neighboring cluster satisfying
  // the pending query of the cluster of this node
  else if (arrivalPort == node->outgoingPortIndex) { 
    node->solveMergingProcess(query);
    node->broadcastCheck(node->outgoingPortIndex);
    node->solveExternalQueries();
    node->replyPendingHelloMsg();
    if (node->outgoingLinkNumber == 0) {
      node->startConvergecast();
      node->status = (node->children.empty()) 
                   ? Status::CONNECTING
                   : Status::PROCESSING;
    }
    else {
      node->broadcastHello();
      node->status = Status::UPDATING;
    }
    delete node->pendingQuery;
    delete query;
  }
  else
    node->externalQueryCache.push_back(query);
}


void MegaMerger::Expand::operator()(MsgPtr msg) {
  auto checkMsg = dynamic_cast<CheckMsg*>(msg);
  bool updateStatus = checkMsg->getUpdateStatus();
  int arrivalPort = checkMsg->getArrivalGate()->getIndex();
  //Case fusion or AnswerQuery
  if (checkMsg->getLevel() > node->level) {
    node->cid = checkMsg->getCid();
    node->level = checkMsg->getLevel();
    node->core = false;
    if (node->outgoingPortIndex == arrivalPort) {
      // The parent and child change its role
      auto it = std::find(
        node->children.begin(), 
        node->children.end(), 
        node->outgoingPortIndex
      );
      if (it != node->children.end()) {
        *it = node->parent; //parent becomes child
        node->parent = node->outgoingPortIndex; //child becomes parent
      }
      else {
        node->parent = arrivalPort;
        node->tree.push_back(arrivalPort);
        std::get<Index::KIND>(node->neighborCache[arrivalPort]) =
          LinkKind::BRANCH;
        node->outgoingLinkNumber--;
      }
    }
    node->replyPendingHelloMsg();
    if (node->pendingQuery)
      delete node->pendingQuery;
  }
  else
    node->error("A6: FUCK! This should be impossible");
  node->broadcastCheck(checkMsg->getArrivalGate()->getIndex(), checkMsg);
  if (updateStatus) {
    if (node->outgoingLinkNumber > 0) {
      node->broadcastHello();
      node->status = Status::UPDATING;
    }
    else {
      node->startConvergecast();
      node->status = (node->children.empty()) 
                   ? Status::CONNECTING
                   : Status::PROCESSING;
    }
  }
}

void MegaMerger::Terminate::operator()(MsgPtr msg) {
  node->broadcastTermination(msg);
  node->status = Status::DONE;
}

void MegaMerger::AnswerQuery::operator()(MsgPtr msg) {
  auto query = dynamic_cast<QueryMsg*>(msg);
  int arrivalPort = query->getArrivalGate()->getIndex();
  // Case Absortion
  if (node->level > query->getLevel()) {
    std::get<Index::KIND>(node->neighborCache[arrivalPort]) = LinkKind::BRANCH;
    node->tree.push_back(arrivalPort);
    node->children.push_back(arrivalPort);
    node->outgoingLinkNumber--;
    node->replyQuery(arrivalPort);
    delete query;
  }
  // Case Merging
  else if (node->level == query->getLevel())
    node->externalQueryCache.push_back(query);
  else
   node->error("AnswerQuery: FUCK! This condition should not be occur\n");

}

void MegaMerger::UnexpectedHello::operator()(MsgPtr msg) {
  auto hello = dynamic_cast<HelloMsg*>(msg);
  int arrivalPort = hello->getArrivalGate()->getIndex();
  if (hello->getCid() == node->cid) {
    node->updateNeighborCacheEntry(arrivalPort, hello);
    node->replyHello(hello);
  }
  else if (node->level >= hello->getLevel()) {
    node->updateNeighborCacheEntry(arrivalPort, hello);
    node->replyHello(hello);
  }
  else
    node->pendingHelloCache.push_back(hello);
}