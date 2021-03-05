cplusplus {{
  #include "Event.h"
}}


message HelloMsg {
  name = "hello";
  kind = EventKind::HELLO;
  int uid;
}