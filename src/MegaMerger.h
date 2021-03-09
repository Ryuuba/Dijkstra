//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#if !defined(MEGAMERGER_H)
#define MEGAMERGER_H

#include <algorithm>
#include <array>
#include <tuple>
#include <vector>

#include "BaseNode.h"
#include "HelloMsg_m.h"
#include "QueryMsg_m.h"
#include "MinMsg_m.h"
#include "ReqMsg_m.h"
#include "CheckMsg_m.h"

/** @brief This class describes the procedures and elements of nodes obeying
 *  the Mega-Merger protocol. All members of this class must be public in order
 *  of actions being able to access both members and member-functions.
 *  @author A.G. Medrano-Chavez.
*/
class MegaMerger : public BaseNode {
public:
  /** @brief A neighbor cache entry composed for these elements: the weight, 
   *  the min uid, the max uid, the neighbor ID, the cluster id, the level, the port, and
   *  the kind of link. 
   */
  typedef std::tuple<double, int, int, int, int, int, int> CacheEntry;
  /** @brief A triple of kind <weight, minUid, maxUid> */
  typedef std::tuple<double, int, int> Link;
  /** @brief Default constructor */
  MegaMerger();
  /** @brief Frees the memory of objects allocated in vector pointer */
  ~MegaMerger();
  /** @brief Modifies the simulation canvas, e.g.,
    * - shows a text string calling displayInfo() or
    * - changes the color of a link with changeLinkColor()
    */
  virtual void refreshDisplay() const override;
  /** @brief Sets the initial status of this node as well as
    * registers the rules this node obeys
    */
  virtual void initialize() override;
protected:
  enum Index {
    WEIGHT = 0, // The weight of the link
    MIN_ID,     // The min uid 
    MAX_ID,     // The max uid
    NID,        // The neighbor ID
    CID,        // The cluster ID of the neighbor
    PORT,       // The port of the neighbor
    KIND        // The kind of link
  };
  enum LinkKind {
    INTERNAL = 0, // A link that belongs the node's cluster, but not a branch
    OUTGOING,     // A link that connect this node with a neighboring cluster
    BRANCH,       // A link that belongs to the MST of the node´s cluster
    UNKNOWN       // A link whose state is unknown
  };
  /** @brief The set of tree neighbors. This data structure stores the port 
   *  number through which reaching a tree neighbor is possible.
   */
  std::vector<int> tree;
  /** @brief The set of children of this node. This data structure stores the
   *  number of port of children, not the uid.
   */
  std::vector<int> children;
  /** @brief Holds the port number, the id, the cid, the level and the kind of
   * link of a neighbor from N(x)
   */
  std::vector<CacheEntry> neighborCache;
  /** @brief This data structure stores request messages from neighboring 
   *  clusters */
  std::vector<ReqMsg*> reqCache;
  /** @brief This data structure stores request messages from neighboring 
   *  clusters */
  std::vector<QueryMsg*> queryCache;
  /** @brief This data structure stores unexpected minimum messages */
  std::vector<MinMsg*> minCache;
  /** @brief The UID of the contact from the neighboring cluster.
   */
  int expectedContactPointUid;
  /** @brief Flag indicating this node is the core of its cluster */
  bool core;
  /** @brief Flag indicating this node sends its minimum value to its
   *  core */
  bool isConvergecastFinished;
  /** @brief The number unique ID of this node */
  int uid;
  /** @brief The ID of the node's cluster */
  int cid;
  /** @brief The index of the port connecting this node with its parent */
  int parent;
  /** @brief The number of friendly merging this cluster performs */
  int level;
  /** @brief The number of hello messages this node receives */
  int helloCounter;
  /** @brief The number of min messages currently received */
  unsigned minCounter;
  /** @brief The number of remaining unknown links */
  int unknownLinkCnt;
  /** @brief The port index associated to the minimum-weight outgoing link */
  int outgoingPortIndex;
  /** @brief The ID of the contact point */
  int contactPointId;
  /** @brief The current outgoing link that has the minimum local weight */
  Link outgoingLink;
protected:
  /** @brief Composes a request message, then, forwards it through the 
   *  outgoingPortIndex until reaching the contact point
   *  @param req a request message
   */
  virtual void forwardRequest(ReqMsg* req = nullptr);
  /** @brief Composes a min message, which is sent to the parent of this node if
   *  any.
   */
  virtual void sendMin();
  /** @brief Sends a query through the port reaching a possible 
   *  minimum-weight local outgoing link
   */
  virtual void sendQuery();
  /** @brief Composes an affirmative response to a query, then sends it via
   *  unicast to the destination
   *  @param port The destination port
   */
  virtual void sendYes(int);
  /** @brief Composes a negative response to a query, then sends it via
   *  unicast to the destination
   *  @param port The destination port
   */
  virtual void sendNo(int);
  /** @brief Composes a check to a query, then sends it via
   *  unicast to the destination
   *  @param port The destination port
   */
  virtual void sendCheck(int, bool);
  /** @brief Composes a check message, then, broadcasts it (if possible), 
   *  through the spanning tree. In case a leaf node invokes this method,
   *  the check message is deleted.
   *  @param check A check message
  */
  virtual void broadcastCheck(bool, CheckMsg* msg = nullptr);
  /** @brief Composes a hello message, then, broadcasts it
  */
  virtual void broadcastHello();
 /** @brief Composes a termination message, then, broadcast it through the 
  *  spanning tree. Leaf nodes delete this message.
  */
  virtual void downstremBroadcastTermination(Msg* msg = nullptr);
  /** @brief Replies, if it is possible, query messages the query cache stores */
  virtual void replyPendingQueryMsg();
  /** @brief Starts a updating process to find the minimum-weight local edge */
  virtual void startUpdating();
  /** @brief Starts a convergecast process to compute the cluster-level 
   *  minimum-weight edge */
  virtual void startConvergecast();
  /** @brief Computes the outgoing link for which the merging process takes 
   *  place. An outgoing link is that meets these two requirements:
   *  - it has a minimum value <weight,uid,uid> and,
   *  - it connects the cluster of this node with another one with level
   *    greater than or equal to the cluster this node belongs */
  virtual void computeOutgoingLink();
  /** @brief Sets the outgoing link with a infinity weight */
  virtual void setInfinityWeight();
  /** @brief Updates the local minimum outgoing link 
   *  @param link A tuple of kind <weight, uid, uid>
  */
  virtual void updateMinOutgoingLink(Link&);
  /** @brief Solves the merging process by comparing the level of an internal
   *  query with to level of an external query
   *  @param query An external query
   */
  virtual void updateClusterState(ReqMsg*);
  /** @brief Fills the fields of a cache entry from a hello msg
   *  @param entry a cache entry
   *  @param hello a hello message
   *  @parama linkKind the kind of the link
   */
  virtual void fillCacheEntry(CacheEntry&, HelloMsg*, LinkKind);
  /** @brief Updates an entry in neighborCache
   *  @param index The index of the neighbor cache entry
   *  @param hello A received hello message
  */
  virtual void updateNeighborCacheEntry(int, CacheEntry&);
  /** @brief Solves pending requests, if any. Such requests are responded with
   *  a check message.
   */
  virtual void attendPendingRequest();
  /** @brief Initializes node-state variables */
  virtual void initializeNodeState();
  /** @brief Tries to absorb a neighboring cluster, otherwise, the req is
   *  held in the request cache
   *  @param request A request message
   */
  virtual void tryAbsorption(ReqMsg*);
  /** @brief Convergecasts the local minimum */
  virtual void convergecast();
  /** @brief Wakes spontaneously this node 
   *  up, initalizes node state and starts a hello process: nodes exchange its
   *  UID in order to compute link weights */
  class WakingUp;
  /** @brief Wakes this node 
   *  up, initalizes node state and starts a hello process: nodes exchange its
   *  UID in order to compute link weights */
  class BroadcastingHello;
  /** @brief Processes a YES message */
  class ProcessingYes;
  /** @brief Processes a YES message */
  class ProcessingNo;
  /** @brief Updates the neighbor cache when receiving the response of a query */
  class UpdatingCache;
  /** @brief : Nodes broadcast a check message to inform nodes the new level 
   *  and the new cid */
  class ClusterMerger;
  /** @brief Nodes forward the request to the cluster's minimum-weight outgoing 
   *  link */
  class ForwardingRequest;
  /** @brief Replies a query message */
  class ReplyingQuery;
  /** @brief Computing the cluster's minimum weight outgoing link via a 
   *  convergecast proceess. An intermediate node waits 
   *  for a minimum message comming from their corresponding children. When 
   *  receiving the minimum messages, this node updates both its outgoingLink
   *  and its outgoing port. Once this node receives all min messages from its
   *  children, it sends the local minimum value to its parent. Eventually, root
   *  is able to compute the cluster's minimum weight outgoing link.
   */
  class ComputingMinimum;
  /** @brief Nodes update their level, cid, and orientation if it is requiered. 
   *  Then, they start an updating process if they have more than one possible 
   * o utgoing link, otherwise, they start a connecting process through a 
   *  convergecast */
  class Expanding;
  /** @brief Nodes broadcast through the spanning tree a termination message that indicates the end of the computation of  the Mega-Merger protocol */
  class Solving;
  /** @brief Caches unexpected minimum values */
  class CachingMinimum;
  /** @brief Tries to absorb a neighboring cluster */
  class TryingAbsorption;
};

class MegaMerger::WakingUp : public BaseAction {
private:
  MegaMerger* ap;
public:
  WakingUp(MegaMerger* ptr) : BaseAction("WakingUp"), ap(ptr) { }
  void operator()(Impulse*);
};

class MegaMerger::BroadcastingHello : public BaseAction {
private:
  MegaMerger* ap;
public:
  BroadcastingHello(MegaMerger* ptr) : BaseAction("BroadcastingHello"), ap(ptr) { }
  void operator()(Msg*);
};

class MegaMerger::UpdatingCache : public BaseAction {
private:
  MegaMerger* ap;
public:
  UpdatingCache(MegaMerger* ptr)
    : BaseAction("UpdatingCache")
    , ap(ptr)
{ }
  void operator()(Msg*);
};

class MegaMerger::ReplyingQuery : public BaseAction {
private:
  MegaMerger* ap;
public:
  ReplyingQuery(MegaMerger* ptr) : BaseAction("ReplyingQuery"), ap(ptr) { }
  void operator()(Msg*);
};

class MegaMerger::ProcessingYes : public BaseAction {
private:
  MegaMerger* ap;
public:
  ProcessingYes(MegaMerger* ptr)
    : BaseAction("ProcessingYes")
    , ap(ptr)
{ }
  void operator()(Msg*);
};

class MegaMerger::ProcessingNo : public BaseAction {
private:
  MegaMerger* ap;
public:
  ProcessingNo(MegaMerger* ptr)
    : BaseAction("ProcessingNo")
    , ap(ptr)
{ }
  void operator()(Msg*);
};


class MegaMerger::ComputingMinimum : public BaseAction {
private:
  MegaMerger* ap;
public:
  ComputingMinimum(MegaMerger* ptr) : BaseAction("ComputingMinimum"), ap(ptr) { }
  void operator()(Msg*);
};

class MegaMerger::ClusterMerger : public BaseAction {
private:
  MegaMerger* ap;
public:
  ClusterMerger(MegaMerger* ptr) : BaseAction("ClusterMerger"), ap(ptr) { }
  void operator()(Msg*);
};

class MegaMerger::Expanding : public BaseAction {
private:
  MegaMerger* ap;
public:
  Expanding(MegaMerger* ptr) : BaseAction("Expanding"), ap(ptr) { }
  void operator()(Msg*);
};

class MegaMerger::Solving : public BaseAction {
private:
  MegaMerger* ap;
public:
  Solving(MegaMerger* ptr) : BaseAction("Solving"), ap(ptr) { }
  virtual void operator()(Msg*);
};

class MegaMerger::TryingAbsorption : public BaseAction {
private:
  MegaMerger* ap;
public:
  TryingAbsorption(MegaMerger* ptr)
    : BaseAction("TryingAbsorption")
    , ap(ptr)
{ }
  void operator()(Msg*);
};

class MegaMerger::ForwardingRequest : public BaseAction {
private:
  MegaMerger* ap;
public:
  ForwardingRequest(MegaMerger* ptr)
    : BaseAction("ForwardingRequest")
    , ap(ptr)
{ }
  void operator()(Msg*);
};

class MegaMerger::CachingMinimum : public BaseAction {
private:
  MegaMerger* ap;
public:
  CachingMinimum(MegaMerger* ptr)
    : BaseAction("CachingMinimum")
    , ap(ptr)
{ }
  void operator()(Msg*);
};


#endif // MEGAMERGER_H 
