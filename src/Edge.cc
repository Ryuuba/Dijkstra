#include "Edge.h"

Define_Channel(Edge);

void Edge::initialize() {
  cDelayChannel::initialize();
  if (par("showWeight")) {
    int precision = par("precision");
    weight = par("weight");
    auto weight_str = std::to_string(weight).substr(0, std::to_string(weight).find(".") + precision + 1);
    getDisplayString().setTagArg("t", 0, weight_str.c_str());
    getDisplayString().setTagArg("t", 1, "l");
    getDisplayString().setTagArg("t", 2, "black");
  }
}

double Edge::getWeight() {
  return weight;
}

