cplusplus{{
  #include "Event.h"
}}

message QueryMsg {
  name = "query";
  kind = EventKind::QUERY;
  int level;
  int cid;
}