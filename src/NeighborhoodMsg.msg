cplusplus{{
  #include <tuple>
  #include <list>
  #include <memory>
  #include "Event.h"
  typedef std::tuple<int, int, double> NeighborhoodEntry;
  typedef std::shared_ptr<std::list<NeighborhoodEntry>> Neighborhood;
}}

class noncobject Neighborhood;

message NeighborhoodMsg {
  name = "neighborhood";
  kind = EventKind::NEIGHBORHOOD;
  int sender;
  int subtreeSize;
  Neighborhood n;
}