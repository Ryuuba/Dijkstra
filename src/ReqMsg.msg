cplusplus{{
  #include "Event.h"
}}

message ReqMsg {
  name = "req";
  int contactPointId;  // The ID of the contact point
  int cid;             // The cluster ID
  int level;           // The cluster level
}