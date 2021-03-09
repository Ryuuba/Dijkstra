
cplusplus{{
  #include <utility>
  #include <memory>
  #include "Event.h"
  typedef std::list<std::pair<int, double>> MatrixEntry;
  typedef std::shared_ptr<std::vector<MatrixEntry>> AdjacencyMatrix;
}}

class noncobject AdjacencyMatrix;

message GraphMsg {
  name = "graph";
  kind = EventKind::GRAPH;
  AdjacencyMatrix m;
}