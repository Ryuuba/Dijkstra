//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#if !defined(DIJKSTRA_H)
#define DIJKSTRA_H

#include "MegaMerger.h"
#include "NeighborhoodMsg_m.h"
#include "GraphMsg_m.h"

#include <numeric>
#include <algorithm>

class Dijkstra : public MegaMerger {
public:
  virtual void initialize() override;
  typedef std::tuple<int, int, double> RTEntry; //prev. uid, port, distance
  typedef std::unordered_map<int, RTEntry> RoutingTable;
protected:
  int networkSize;
  int counter;
  bool source;
  bool destination, leaderFlag = true;
  Neighborhood n;
  AdjacencyMatrix graph;
  RoutingTable routingTable;
protected:
  virtual void sendNeighborhood(NeighborhoodMsg* msg = nullptr);
  virtual void sendGraph(GraphMsg* msg = nullptr);
  virtual void computeGraph();
  virtual void computeRoutingTable();
  virtual void printRoutingTable();
  virtual void printGraph();
protected:
  class StartingConvergecast;
  class ConvergecastingNeighborhood;
  class ComputingRT;
  class Routing;
};

class Dijkstra::StartingConvergecast : public BaseAction {
private:
  Dijkstra* ap;
  MegaMerger::Solving solving;
public:
  StartingConvergecast(Dijkstra* ptr)
    : BaseAction("StartingConvergecast")
    , ap(ptr)
    , solving(ap)
{ }
  void operator()(Msg*);
};

class Dijkstra::ConvergecastingNeighborhood : public BaseAction {
private:
  Dijkstra* ap;
public:
  ConvergecastingNeighborhood(Dijkstra* ptr)
    : BaseAction("ConvergecastingNeighborhood")
    , ap(ptr)
{ }
  void operator()(Msg*);
};


class Dijkstra::ComputingRT : public BaseAction {
private:
  Dijkstra* ap;
public:
  ComputingRT(Dijkstra* ptr)
    : BaseAction("ComputingRT")
    , ap(ptr)
{ }
  void operator()(Msg*);
};

class Dijkstra::Routing : public BaseAction {
private:
  Dijkstra* ap;
public:
  Routing(Dijkstra* ptr)
    : BaseAction("Routing")
    , ap(ptr)
{ }
  void operator()(Msg*);
};



#endif // Dijkstra

