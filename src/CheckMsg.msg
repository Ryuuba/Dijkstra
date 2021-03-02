cplusplus{{
  #include "Event.h"
}}

message CheckMsg {
  name = "check";
  kind = EventKind::CHECK;
  bool updateStatus;
  int level;
  int cid;
}