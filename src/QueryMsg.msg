cplusplus{{
  #include "Event.h"
}}

message QueryMsg {
  name = "query";
  kind = EventKind::QUERY;
  int contactPointId;  // The ID of the contact point
  int cid;          // The cluster ID
  int level;        // The cluster level
}