cplusplus{{
  #include "Event.h"
}}

message MinMsg {
  name = "min";
  kind = EventKind::MIN;
  int id; //The ID of the message composer
  double weight;  // The weight of the link
  int minUid;     // The min UID of a link
  int maxUid;     // The max UID of a link
}