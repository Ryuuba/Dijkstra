#include "Dijkstra.h"

Define_Module(Dijkstra);

void Dijkstra::initialize() {
  MegaMerger::initialize();
  source = par("source").boolValue();
  destination = par("destination").boolValue();
  addRule(Status::CONNECTING, EventKind::TERMINATION, New_Action(StartingConvergecast));
  addRule(Status::FOLLOWER, EventKind::NEIGHBORHOOD, New_Action(ConvergecastingNeighborhood));
  addRule(Status::LEADER, EventKind::NEIGHBORHOOD, New_Action(ConvergecastingNeighborhood));
  addRule(Status::PROCESSING, EventKind::GRAPH, New_Action(ComputingRT));
  addRule(Status::ROUTING, EventKind::DATA, New_Action(Routing));
}

void Dijkstra::sendNeighborhood(NeighborhoodMsg* msg) {
  NeighborhoodEntry entry;
  if (!msg) 
    msg = new NeighborhoodMsg;
  msg->setSubtreeSize(networkSize);
  msg->setSender(uid);
  for (auto& neighbor : neighborCache) {
    std::get<0>(entry) = uid;
    std::get<1>(entry) = std::get<MegaMerger::Index::NID>(neighbor);
    std::get<2>(entry) = std::get<MegaMerger::Index::WEIGHT>(neighbor);
    n->push_back(entry);
  }
  msg->setN(n);
  send(msg, out, parent);
}

void Dijkstra::sendGraph(GraphMsg* msg) {
  if (!msg) {
    msg = new GraphMsg;
    msg->setM(graph);
  }
  localMulticast(msg, children);
}

void Dijkstra::computeGraph() {
  graph = std::make_shared<std::vector<MatrixEntry>>(networkSize);
  MatrixEntry row;
  std::pair<int, double> element;
  for (auto& item : (*n)) {
    element.first = std::get<1>(item);  //dest
    element.second = std::get<2>(item); //weight
    (*graph)[std::get<0>(item)].push_back(element);
  }
}

void Dijkstra::computeRoutingTable() {
  using std::get;
  std::vector<int> unvisited(networkSize);
  RTEntry rtEntry;
  //initialize
  for (int i = 0; i < networkSize; i++) {
    unvisited.push_back(i);
    if (i == uid)
      routingTable[i] = std::make_tuple<int, int, double>(-1, -1, 0.0);
    else {
      auto it = std::find_if (
        neighborCache.begin(), 
        neighborCache.end(), 
        [&](const CacheEntry& cacheEntry)->bool {
          return std::get<MegaMerger::Index::NID>(cacheEntry) == i;
        }
      ); 
      if (it != neighborCache.end()) {
        get<0>(rtEntry) = uid; //prev uid
        get<1>(rtEntry) = get<MegaMerger::Index::PORT>(neighborCache[i]);
        get<2>(rtEntry) = get<MegaMerger::Index::WEIGHT>(neighborCache[i]);
        routingTable[i] = std::move(rtEntry);
        std::cout << "Neighbor " << i << " is in N(" <<getIndex() << ")\n";
        std::cout << "Weight " << get<2>(routingTable[i]) << '\n';
      }
      else {
        get<0>(rtEntry) = -1; //prev uid
        get<1>(rtEntry) = -1; //port
        get<2>(rtEntry) = std::numeric_limits<double>::infinity();
        routingTable[i] = std::move(rtEntry);
      }
    }
  }
  while (!unvisited.empty()) {
    int w = -1;
    double weight = std::numeric_limits<double>::infinity();
    std::vector<int>::iterator current;
    for (auto it = unvisited.begin(); it != unvisited.end(); ++it) {
      double minWeight = std::min(weight, get<2>(routingTable[*it]));
      if (minWeight < weight) {
        weight = minWeight;
        w = *it;
        current = it;
      }
    }
    if (w >= 0) {
      for (auto& v : (*graph)[w]) {
        if (
          std::find(unvisited.begin(), unvisited.end(), v.first) != 
          unvisited.end()
        ){
          double distance = std::min(
            get<2>(routingTable[v.first]),
            get<2>(routingTable[w]) + v.second
          );
          if (get<2>(routingTable[v.first]) != distance) {
            get<0>(rtEntry) = w; //prev uid
            get<2>(rtEntry) = distance;
            routingTable[v.first] = std::move(rtEntry);
          }
        }
      }
    }
    if (current != unvisited.end())
      unvisited.erase(current);
  }
}

void Dijkstra::printRoutingTable() {
  std::cout << "Routing table of node " << getIndex() << '\n';
  for (auto& entry : routingTable)
    std::cout << "Destination: " << entry.first << '\n'
              << "Previous node: " << std::get<0>(entry.second) << '\n'
              << "Port: " << std::get<1>(entry.second) << '\n'
              << "Distance: " << std::get<2>(entry.second) << '\n';
  std::cout << std::endl;

}

void Dijkstra::StartingConvergecast::operator()(Msg* msg) {
  solving(msg);
  ap->counter = 0;
  ap->networkSize = 1;
  ap->n = std::make_shared<std::list<NeighborhoodEntry>>();
  if (ap->children.empty()) {
    ap->sendNeighborhood();
    ap->status = Status::PROCESSING;
  }
}

void Dijkstra::ConvergecastingNeighborhood::operator()(Msg* msg) {
  auto nMsg = dynamic_cast<NeighborhoodMsg*>(msg);
  if (ap->leaderFlag && ap->status == Status::LEADER) {
    NeighborhoodEntry entry;
    ap->counter = 0;
    ap->networkSize = 1;
    ap->n = std::make_shared<std::list<NeighborhoodEntry>>();
    for (auto& neighbor : ap->neighborCache) {
      std::get<0>(entry) = ap->uid;
      std::get<1>(entry) = std::get<MegaMerger::Index::NID>(neighbor);
      std::get<2>(entry) = std::get<MegaMerger::Index::WEIGHT>(neighbor);
      ap->n->push_back(entry);
    }
    ap->leaderFlag = false;
  }
  ap->counter++;
  ap->networkSize += nMsg->getSubtreeSize();
  ap->n->insert(ap->n->end(), nMsg->getN()->begin(), nMsg->getN()->end());
  if (ap->counter == ap->children.size()) {
    if (ap->status == Status::LEADER) {
      ap->computeGraph();
      ap->printGraph();
      ap->computeRoutingTable();
      ap->printRoutingTable();
      ap->sendGraph();
      ap->status = Status::ROUTING;
    }
    else
      ap->sendNeighborhood(nMsg);
  }
}

void Dijkstra::ComputingRT::operator()(Msg* msg) {
  auto graphMsg = dynamic_cast<GraphMsg*>(msg);
  ap->graph = graphMsg->getM();
  ap->networkSize = ap->graph->size();
  ap->computeRoutingTable();
  ap->sendGraph(graphMsg);
  ap->status = Status::ROUTING;
}

void Dijkstra::Routing::operator()(Msg* msg) {
  delete msg;
}

void Dijkstra::printGraph() {
  std::cout << "network size: " << networkSize << '\n';
  for (int i = 0; i < networkSize; i++){
    std::cout << "Node[" << i << "] = { ";
    for (auto& item : (*graph)[i]) {
      std::cout << '(' << item.first << ", " << item.second << ") ";
    }
    std::cout << "}\n";
  }
}