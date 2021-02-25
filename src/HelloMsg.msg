cplusplus {{
  #include "Event.h"
}}


message HelloMsg {
  name = "hello";
  kind = EventKind::HELLO;
  int id;
  int cid;
  int level;
}