#include "MegaMerger.h"

Define_Module(MegaMerger);

MegaMerger::MegaMerger()
  : expectedContactPointUid(-1)
  , isConvergecastFinished(false)
  , cid(-1)
  , parent(-1)
  , level(-1)
{ }

MegaMerger::~MegaMerger() {
  for (auto& ptr : queryCache)
    delete ptr;
  for (auto& ptr : reqCache)
    delete ptr;
}

void MegaMerger::initialize() {
  if (par("initiator").boolValue())
    spontaneously();
  initializeNeighborhood();
  
  for (int i = 0; i < neighborhoodSize; i++) {
    CacheEntry entry(
      std::numeric_limits<double>::infinity(),
      std::numeric_limits<int>::max(),
      std::numeric_limits<int>::max(),
      -1,
      -1,
      LinkKind::UNKNOWN
    );
    neighborCache.push_back(std::move(entry));
  }
  unknownLinkCnt = neighborhoodSize;
  addRule(Status::IDLE, EventKind::IMPULSE, New_Action(WakingUp));
  addRule(Status::IDLE, EventKind::HELLO, New_Action(BroadcastingHello));
  addRule(Status::CONNECTING, EventKind::REQ, New_Action(ClusterMerger));
  addRule(Status::CONNECTING, EventKind::FWD, New_Action(ForwardingRequest));
  addRule(Status::CONNECTING, EventKind::QUERY, New_Action(ReplyingQuery));
  addRule(Status::CONNECTING, EventKind::CHECK, New_Action(Expanding));
  addRule(Status::CONNECTING, EventKind::TERMINATION, New_Action(Solving));
  addRule(Status::UPDATING, EventKind::HELLO, New_Action(UpdatingCache));
  addRule(Status::UPDATING, EventKind::QUERY, New_Action(ReplyingQuery));
  addRule(Status::UPDATING, EventKind::YES, New_Action(ProcessingYes));
  addRule(Status::UPDATING, EventKind::NO, New_Action(ProcessingNo));
  addRule(Status::UPDATING, EventKind::REQ, New_Action(TryingAbsorption));
  addRule(Status::UPDATING, EventKind::MIN, New_Action(CachingMinimum));
  addRule(Status::PROCESSING, EventKind::MIN, New_Action(ComputingMinimum));
  addRule(Status::PROCESSING, EventKind::QUERY, New_Action(ReplyingQuery));
  addRule(Status::PROCESSING, EventKind::REQ, New_Action(TryingAbsorption));
  status = Status::IDLE;
  WATCH(unknownLinkCnt);
  WATCH(outgoingPortIndex);
}

void MegaMerger::refreshDisplay() const {
  std::string info(status.str());
  info += '\n' + std::to_string(cid) + ' ' 
               + std::to_string(level) + '\n';
  displayInfo(info.c_str());
  for (int i = 0; i < neighborhoodSize; i++) {
    if (std::get<Index::KIND>(neighborCache[i]) == LinkKind::BRANCH) {
      changeEdgeColor(i, "teal");
      changeEdgeWidth(i, 4);
    }
    else if (std::get<Index::KIND>(neighborCache[i]) == LinkKind::INTERNAL) {
      changeEdgeColor(i, "teal");
      changeEdgeWidth(i, 4);
      setEdgeDashed(i);
    }
    else if (std::get<Index::KIND>(neighborCache[i]) == LinkKind::UNKNOWN) 
      changeEdgeColor(i, "black");
    else //Outgoing link
      changeEdgeColor(i, "orange");
  }
}

void MegaMerger::sendMin() {
  auto min = new MinMsg;
  if (outgoingPortIndex >= 0) {
    min->setWeight(std::get<Index::WEIGHT>(outgoingLink));
    min->setMinUid(std::get<Index::MIN_ID>(outgoingLink));
    min->setMaxUid(std::get<Index::MAX_ID>(outgoingLink));
  }
  else {
    min->setWeight(std::numeric_limits<double>::infinity());
    min->setMinUid(std::numeric_limits<int>::max());
    min->setMaxUid(std::numeric_limits<int>::max());
  }
  send(min, out, parent);
}

void MegaMerger::forwardRequest(ReqMsg* req) {
  if (!req) {
    req = new ReqMsg;
    req->setCid(cid);
    req->setLevel(level);
    req->setContactPointId(contactPointId);
    req->setKind(EventKind::FWD);
  }
  if (req->getContactPointId() == uid) {
    req->setKind(EventKind::REQ);
  }
  send(req, out, outgoingPortIndex);
}

void MegaMerger::sendQuery() {
  auto query = new QueryMsg;
  query->setCid(cid);
  query->setLevel(level);
  send(query, out, outgoingPortIndex);
}

void MegaMerger::sendYes(int port) {
  if (port != outgoingPortIndex) {
    auto yes = new Msg("yes", EventKind::YES);
    send(yes, out, port);
  }
}

void MegaMerger::sendNo(int port) {
  if (port != outgoingPortIndex) {
    auto no = new Msg("no", EventKind::NO);
    send(no, out, port);
  }
}

void MegaMerger::sendCheck(int port, bool changeStatus) {
  auto check = new CheckMsg;
  check->setCid(cid);
  check->setLevel(level);
  check->setUpdateStatus(changeStatus);
  send(check, out, port);
}

void MegaMerger::broadcastHello() {
  auto hello = new HelloMsg;
  hello->setUid(uid);
  for (int i = 1; i < neighborhoodSize; i++) 
      send(hello->dup(), out, i);
  send(hello, out, 0);
}

void MegaMerger::broadcastCheck(bool changeStatus, CheckMsg* msg) {
  int arrivalGate;
  if (!msg) {
    arrivalGate = -1;
    msg = new CheckMsg;
    msg->setLevel(level);
    msg->setCid(cid);
    msg->setUpdateStatus(changeStatus);
  }
  else
    arrivalGate = msg->getArrivalGate()->getIndex();
  for (auto& neighbor : tree)
    if (neighbor != arrivalGate)
      send(msg->dup(), out, neighbor);
  delete msg;
}

void MegaMerger::downstremBroadcastTermination(Msg* termination) {
  if (!termination)
    termination = new omnetpp::cMessage("termination", EventKind::TERMINATION);
  localMulticast(termination, children);
}

void MegaMerger::updateNeighborCacheEntry(int portNumber, CacheEntry& entry) {
  neighborCache[portNumber] = std::move(entry);
}

void MegaMerger::startUpdating() {
  expectedContactPointUid = -1;
  contactPointId = -1;
  sendQuery();
}

void MegaMerger::startConvergecast() {
  using std::get;
  minCounter = 0;
  if (children.empty()) {
    isConvergecastFinished = true;
    sendMin();
    status = Status::CONNECTING;
  }
  else if (!minCache.empty()) {
    auto it = minCache.begin();
    while (it != minCache.end()) {
      auto link = std::make_tuple(
        (*it)->getWeight(),
        (*it)->getMinUid(),
        (*it)->getMaxUid()
      );
      if (link < outgoingLink) {
        updateMinOutgoingLink(link);
        outgoingPortIndex = (*it)->getArrivalGate()->getIndex();
        contactPointId = (*it)->getUid();
      }
      delete (*it);
      it = minCache.erase(it);
      minCounter++;
    }
    if (minCounter == children.size()) {
      convergecast();
    }
  }
  else
    status = Status::PROCESSING;
}

void MegaMerger::convergecast() {
  using std::get;
  if (!core) {
    isConvergecastFinished = true;
    sendMin();
    status = Status::CONNECTING;
  }
  else if (
    get<Index::WEIGHT>(outgoingLink) == std::numeric_limits<double>::infinity()
  ) {
    downstremBroadcastTermination();
    status = Status::LEADER;
  }
  else {
    isConvergecastFinished = true;
    if (contactPointId == uid)
      expectedContactPointUid = std::get<Index::NID>(neighborCache[outgoingPortIndex]);
    forwardRequest();
    status = Status::CONNECTING;
  }
}

void MegaMerger::setInfinityWeight() {
  using std::get;
  get<Index::WEIGHT>(outgoingLink) = std::numeric_limits<double>::infinity();
  get<Index::MIN_ID>(outgoingLink) = std::numeric_limits<int>::max();
  get<Index::MAX_ID>(outgoingLink) = std::numeric_limits<int>::max();
  outgoingPortIndex = -1;
}

void MegaMerger::computeOutgoingLink() {
  using std::get;
  setInfinityWeight();
  int i = 0;
  for (auto& entry : neighborCache) {
    if (get<Index::KIND>(entry) == LinkKind::UNKNOWN) {
      auto link = std::make_tuple(
        get<Index::WEIGHT>(entry),
        get<Index::MIN_ID>(entry),
        get<Index::MAX_ID>(entry)
      );
      if (link < outgoingLink) {
        updateMinOutgoingLink(link);
        outgoingPortIndex = i;
      }
    }
    i++;
  }
}

void MegaMerger::updateMinOutgoingLink(Link& link) {
  using std::get;
  get<Index::WEIGHT>(outgoingLink) = get<Index::WEIGHT>(link);
  get<Index::MIN_ID>(outgoingLink) = get<Index::MIN_ID>(link); 
  get<Index::MAX_ID>(outgoingLink) = get<Index::MAX_ID>(link);
}

void MegaMerger::fillCacheEntry(
  CacheEntry& entry, HelloMsg* hello, LinkKind kind
) {
  using std::get;
  int arrivalGate = hello->getArrivalGate()->getIndex();
  get<Index::WEIGHT>(entry) = getLinkWeight(out, arrivalGate);
  get<Index::MIN_ID>(entry) = (hello->getUid() < uid) ? hello->getUid() : uid;
  get<Index::MAX_ID>(entry) = (hello->getUid() > uid) ? hello->getUid() : uid;
  get<Index::NID>(entry) = hello->getUid();
  get<Index::CID>(entry) = hello->getUid();
  get<Index::KIND>(entry) = kind;
}

//FIXME: Analyze this member function
void MegaMerger::updateClusterState(ReqMsg* req) {
  //Case Fusion
  if (level == req->getLevel()) {
    level++;
    core = uid < req->getContactPointId();
    cid = (core) ? uid : req->getContactPointId();
    parent = (core) ? -1 : outgoingPortIndex;
    if (core) children.push_back(outgoingPortIndex);
    std::get<Index::CID>(neighborCache[outgoingPortIndex]) = cid;
  }
  // Case AnswerQuery by neighboring cluster
  else if (level < req->getLevel()) {
    level = req->getLevel();
    cid = req->getCid();
    core = false;
    parent = outgoingPortIndex;
    std::get<Index::CID>(neighborCache[outgoingPortIndex]) = cid;
  }
  broadcastCheck(true);
  tree.push_back(outgoingPortIndex);
  std::get<Index::KIND>(neighborCache[outgoingPortIndex]) = LinkKind::BRANCH;
  unknownLinkCnt--;
}

void MegaMerger::replyPendingQueryMsg() {
  auto it = queryCache.begin();
  int arrivalGate;
  while (it != queryCache.end()) {
    arrivalGate = (*it)->getArrivalGate()->getIndex();
    if (level >= (*it)->getLevel()) {
      if (cid == (*it)->getCid()) {
        std::get<Index::KIND>(neighborCache[arrivalGate]) = LinkKind::INTERNAL;
        unknownLinkCnt--;
        sendNo(arrivalGate);
      }
      else
        sendYes(arrivalGate);
      delete (*it);
      it = queryCache.erase(it);
    }
    else 
      ++it;
  }
}

void MegaMerger::attendPendingRequest() {
  if (!reqCache.empty()) {
    auto it = reqCache.begin();
    while (it != reqCache.end()) {
      if ((*it)->getLevel() < level) {
        int arrivalGate = (*it)->getArrivalGate()->getIndex();
        std::get<Index::KIND>(neighborCache[arrivalGate]) = LinkKind::BRANCH;
        unknownLinkCnt--;
        tree.push_back(arrivalGate);
        children.push_back(arrivalGate);
        sendCheck(arrivalGate, true);
        delete (*it);
        it = reqCache.erase(it);
      }
      else
        it++;
    }
  }
}

void MegaMerger::initializeNodeState() {
  uid = getIndex();
  cid = getIndex();
  level = 0;
  core = true;
}

void MegaMerger::tryAbsorption(ReqMsg* req) {
  int arrivalGate = req->getArrivalGate()->getIndex();
  bool isBranch = (level == 0) ? false : req->getContactPointId() == 
    std::get<Index::NID>(neighborCache[outgoingPortIndex]);
  // Case absortion
  if (level > req->getLevel()) {
    if (isBranch) {
      std::get<Index::KIND>(neighborCache[arrivalGate]) = LinkKind::BRANCH;
      unknownLinkCnt--;
      tree.push_back(arrivalGate);
      children.push_back(arrivalGate);
      if (unknownLinkCnt > 0) {
        computeOutgoingLink();
        sendQuery();
      }
      else {
        setInfinityWeight();
        startConvergecast();
      }
    }
    else {
      std::get<Index::KIND>(neighborCache[arrivalGate]) = LinkKind::BRANCH;
      unknownLinkCnt--;
      tree.push_back(arrivalGate);
      children.push_back(arrivalGate);
      sendCheck(arrivalGate, isConvergecastFinished ? false : true);
    }
    delete req;
  }
  // Case merger or future absorption
  else
    reqCache.push_back(req);
}

void MegaMerger::WakingUp::operator()(Impulse* impulse) {
  ap->initializeNodeState();
  ap->helloCounter = 0;
  ap->broadcastHello();
  ap->status = Status::UPDATING;
}

void MegaMerger::BroadcastingHello::operator()(Msg* msg) {
  auto hello = dynamic_cast<HelloMsg*>(msg);
  int arrivalPort = hello->getArrivalGate()->getIndex();
  CacheEntry entry;
  ap->initializeNodeState();
  ap->fillCacheEntry(entry, hello, LinkKind::UNKNOWN);
  ap->updateNeighborCacheEntry(arrivalPort, entry);
  ap->helloCounter = 1;
  ap->broadcastHello();
  // Transition to updating
  if (ap->neighborhoodSize > 1)
    ap->status = Status::UPDATING;
  // Transition to connecting
  else {
    ap->computeOutgoingLink();
    ap->contactPointId = ap->uid;
    ap->expectedContactPointUid = 
      std::get<Index::NID>(ap->neighborCache[ap->outgoingPortIndex]);
    ap->forwardRequest();
    ap->status = Status::CONNECTING;
  }
  delete msg;
}

void MegaMerger::UpdatingCache::operator()(Msg* msg) {
  auto hello = dynamic_cast<HelloMsg*>(msg);
  int arrivalPort = hello->getArrivalGate()->getIndex();
  CacheEntry entry;
  ap->fillCacheEntry(entry, hello, LinkKind::UNKNOWN);
  ap->updateNeighborCacheEntry(arrivalPort, entry);
  ap->helloCounter++;
  // Transition to connecting
  if (ap->neighborhoodSize == ap->helloCounter) {
    ap->computeOutgoingLink();
    ap->contactPointId = ap->uid;
    ap->expectedContactPointUid = 
      std::get<Index::NID>(ap->neighborCache[ap->outgoingPortIndex]);
    ap->forwardRequest();
    ap->status = Status::CONNECTING;
  }
  delete msg;
}

void MegaMerger::ClusterMerger::operator()(Msg* msg) {
  auto req = dynamic_cast<ReqMsg*>(msg);
  // Case Merger
  if (ap->expectedContactPointUid == req->getContactPointId()) {
    ap->updateClusterState(req);
    ap->attendPendingRequest();
    ap->replyPendingQueryMsg();
    if (ap->unknownLinkCnt > 0) {
      ap->computeOutgoingLink();
      ap->startUpdating();
      ap->isConvergecastFinished = false;
      ap->status = Status::UPDATING;
    }
    else {
      ap->setInfinityWeight();
      ap->startConvergecast();
      ap->isConvergecastFinished = false;
    }
    delete req;
  }
  // Case Absorption
  else  {
    ap->tryAbsorption(req);
  }
}

void MegaMerger::TryingAbsorption::operator()(Msg* msg) {
  auto req = dynamic_cast<ReqMsg*>(msg);
  ap->tryAbsorption(req);
}

void MegaMerger::Expanding::operator()(Msg* msg) {
  auto checkMsg = dynamic_cast<CheckMsg*>(msg);
  bool updateStatus = checkMsg->getUpdateStatus();
  int  arrivalGate = checkMsg->getArrivalGate()->getIndex();
  ap->cid = checkMsg->getCid();
  ap->level = checkMsg->getLevel();
  ap->core = false;
  std::get<Index::CID>(ap->neighborCache[arrivalGate]) = ap->cid;
  if (
    ap->expectedContactPointUid == 
    std::get<Index::NID>(ap->neighborCache[arrivalGate])
  ) { //Contact
    ap->parent = arrivalGate;
    ap->tree.push_back(arrivalGate);
    std::get<Index::KIND>(ap->neighborCache[arrivalGate]) =
      LinkKind::BRANCH;
    ap->unknownLinkCnt--;
  }
  else {
    auto it = std::find(ap->children.begin(), ap->children.end(), arrivalGate);
    // Message comes from a child
    if (it != ap->children.end()) {
      *it = ap->parent; //parent becomes child
      ap->parent = arrivalGate; //child becomes parent
    }
  }
  ap->broadcastCheck(updateStatus, checkMsg);
  ap->replyPendingQueryMsg();
  if (updateStatus) {
    if (ap->unknownLinkCnt > 0) {
      ap->computeOutgoingLink();
      ap->startUpdating();
      ap->isConvergecastFinished = false;
      ap->status = Status::UPDATING;
    }
    else {
      ap->setInfinityWeight();
      ap->startConvergecast();
      ap->isConvergecastFinished = false;
    }
  }
}

void MegaMerger::ReplyingQuery::operator()(Msg* msg) {
  auto query = dynamic_cast<QueryMsg*>(msg);
  int arrivalGate = query->getArrivalGate()->getIndex();
  std::get<Index::CID>(ap->neighborCache[arrivalGate]) = ap->cid;
  if (query->getCid() == ap->cid) {
    std::get<Index::KIND>(ap->neighborCache[arrivalGate]) = LinkKind::INTERNAL;
    ap->unknownLinkCnt--;
    ap->sendNo(arrivalGate);
    if (ap->unknownLinkCnt == 0) {
      ap->setInfinityWeight();
      ap->startConvergecast();
    }
    delete query;
  }
  else if (ap->level >= query->getLevel()) {
    ap->sendYes(arrivalGate);
    delete query;
  }
  else if ( // Overlap previous req with Q
    ap->expectedContactPointUid == 
    std::get<Index::NID>(ap->neighborCache[arrivalGate])
  ) {
    ap->cid = query->getCid();
    ap->level = query->getLevel();
    ap->core = false;
    ap->parent = arrivalGate;
    ap->broadcastCheck(true);
    ap->tree.push_back(arrivalGate);
    std::get<Index::KIND>(ap->neighborCache[arrivalGate]) =
      LinkKind::BRANCH;
    ap->unknownLinkCnt--;
    ap->replyPendingQueryMsg();
    if (ap->unknownLinkCnt > 0) {
      ap->computeOutgoingLink();
      ap->startUpdating();
      ap->isConvergecastFinished = false;
      ap->status = Status::UPDATING;
    }
    else {
      ap->setInfinityWeight();
      ap->startConvergecast();
      ap->isConvergecastFinished = false;
    }
    delete query;
  }
  else
    ap->queryCache.push_back(query);
}

void MegaMerger::ProcessingYes::operator()(Msg* msg) {
  using std::get;
  int arrivalGate = msg->getArrivalGate()->getIndex();
  ap->expectedContactPointUid = get<Index::NID>(ap->neighborCache[arrivalGate]);
  ap->contactPointId = ap->uid;
  ap->startConvergecast();
  delete msg;
}

void MegaMerger::ProcessingNo::operator()(Msg* msg) {
  using std::get;
  int arrivalGate = msg->getArrivalGate()->getIndex();
  std::get<Index::KIND>(ap->neighborCache[arrivalGate]) = LinkKind::INTERNAL;
  ap->unknownLinkCnt--;
  if (ap->unknownLinkCnt > 0) {
    ap->computeOutgoingLink();
    ap->sendQuery();
  }
  else {
    ap->setInfinityWeight();
    ap->startConvergecast();
  }
  delete msg;
}

void MegaMerger::CachingMinimum::operator()(Msg* msg) {
  auto min = dynamic_cast<MinMsg*>(msg);
  ap->minCache.push_back(min);
}

void MegaMerger::ComputingMinimum::operator()(Msg* msg) {
  auto minMsg = dynamic_cast<MinMsg*>(msg);
  auto link = std::make_tuple(
    minMsg->getWeight(),
    minMsg->getMinUid(),
    minMsg->getMaxUid()
  );
  if (link < ap->outgoingLink) {
    ap->updateMinOutgoingLink(link);
    ap->outgoingPortIndex = minMsg->getArrivalGate()->getIndex();
    ap->contactPointId = minMsg->getId();
  }
  ap->minCounter++;
  if (ap->minCounter == ap->children.size()) {
    ap->convergecast();
  }
  delete minMsg;
}

void MegaMerger::Solving::operator()(Msg* msg) {
  ap->downstremBroadcastTermination(msg);
  ap->status = Status::FOLLOWER;
}

void MegaMerger::ForwardingRequest::operator()(Msg* msg) {
  auto req = dynamic_cast<ReqMsg*>(msg);
  if (req->getContactPointId() == ap->uid) {
    ap->expectedContactPointUid = 
      std::get<Index::NID>(ap->neighborCache[ap->outgoingPortIndex]);
    auto it = find_if (
      ap->reqCache.begin(),
      ap->reqCache.end(),
      [&](ReqMsg* reqMsg) -> bool {
        return ap->expectedContactPointUid == reqMsg->getContactPointId();
      }
    );
    if (it != ap->reqCache.end()) {
      delete *it;
      ap->reqCache.erase(it);
      ap->updateClusterState(req);
      ap->attendPendingRequest();
      ap->replyPendingQueryMsg();
      if (ap->unknownLinkCnt > 0) {
        ap->computeOutgoingLink();
        ap->startUpdating();
        ap->isConvergecastFinished = false;
        ap->status = Status::UPDATING;
      }
      else {
        ap->setInfinityWeight();
        ap->startConvergecast();
        ap->isConvergecastFinished = false;
      }
    }
    else {
      ap->forwardRequest(req);
    }
  }
  else
    ap->forwardRequest(req);
}