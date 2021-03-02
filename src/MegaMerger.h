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
#include "MinMsg_m.h"
#include "QueryMsg_m.h"
#include "CheckMsg_m.h"

/** @brief This class describes the procedures and elements of nodes obeying
 *  the Mega-Merger protocol. All members of this class must be public in order
 *  of actions being able to access both members and member-functions.
 *  @author A.G. Medrano-Chavez.
*/
class MegaMerger : public BaseNode {
public:
  typedef std::tuple<double, int, int, int, int, int, int> CacheEntry;
  typedef std::tuple<double, int, int> Link;
protected:
  enum Index {
    WEIGHT = 0, // The weight of the link
    MIN_ID,     // The min uid 
    MAX_ID,     // The max uid
    NID,        // The neighbor ID
    CID,        // The cluster ID
    LEVEL,      // The cluster level
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
  /** @brief This data structure stores query messages from neighboring 
   *  clusters */
  std::vector<QueryMsg*> externalQueryCache;
  /** @brief This data structure stores hello msgs that cannot be back */
  std::vector<HelloMsg*> pendingHelloCache;
  /** @brief This data structure stores the port numbers where hello msgs
   *  are expected
   */
  std::vector<int> expectedHelloPort;
  /** @brief This message stores a copy of the message the core of the cluster
   *  of this node composes.
   */
  QueryMsg* pendingQuery;
  /** @brief Flag indicating this node is the core of its cluster */
  bool core;
  /** @brief Flag indicating this node sends its minimum value to its
   *  core */
  bool isMinimumSent;
  /** @brief The number unique ID of this node */
  int uid;
  /** @brief The ID of the node's cluster */
  int cid;
  /** @brief The index of the port connecting this node with its parent */
  int parent;
  /** @brief The number of friendly merging this cluster performs */
  int level;
  /** @brief Number of outgoing links */
  int outgoingLinkNumber;
  /** @brief The number of min messages currently received */
  unsigned minCounter;
  /** @brief The port ID associated to an outgoing link */
  int outgoingPortIndex;
  /** @brief The ID of the contact point */
  int contactPointID;
  /** @brief The current outgoing link that has the minimum local weight */
  Link outgoingLink;
protected:

  /** @brief Composes a min message, which is sent to the parent of this node if
   *  any.
   */
  virtual void sendMin();
  /** @brief Composes a check message, then, broadcasts it (if possible), 
   *  through the spanning tree. In case a leaf node invokes this method,
   *  the check message is deleted.
   *  @param discardedPort The port number of the original sender
   *  @param check A check message
  */
  virtual void broadcastCheck(int, CheckMsg* msg = nullptr);
 /** @brief Composes a termination message, then, broadcast it through the 
  *  spanning tree. Leaf nodes delete this message.
  */
  virtual void broadcastTermination(MsgPtr msg = nullptr);
  /** @brief Composes a query message requesting to merge the cluster of this 
   *  node with that is reachable through the outgoing link with min value
   *  @param query A query message
   */
  virtual void sendQuery(QueryMsg* msg = nullptr);
  /** @brief Replies a query in a unicast manner with a chech message 
   *  @param port The arrival port of a external query
  */
  virtual void replyQuery(int);
  /** @brief Composes a hello message, then, sends it via multicast according 
   *  to the CID and LEVEL of the sender
  */
  virtual void broadcastHello(int arrivalPort = -1);
  /** @brief Composes a reply to a given hello message, then it is sent to its
   *  emisor via unicast
   *  @param hello A hello message
   */
  virtual void replyHello(HelloMsg*);
  /** @brief Replies, if it is possible, hello messages the hello cache stores */
  virtual void replyPendingHelloMsg();
  /** @brief Starts a convergecast process to find the minimum edge */
  virtual void startConvergecast();
  /** @brief Computes the outgoing link for which the merging process takes 
   *  place. An outgoing link is that meets these two requirements:
   *  - it has a minimum value <weight,uid,uid> and,
   *  - it connects the cluster of this node with another one with level
   *    greater than or equal to the cluster this node belongs */
  virtual void computeOutgoingLink();
  /** @brief Updates the local minimum outgoing link 
   *  @param link A tuple of kind <weight, uid, uid>
  */
  virtual void updateMinOutgoingLink(Link&);
  /** @brief Solves the merging process by comparing the level of an internal
   *  query with to level of an external query
   *  @param query An external query
   */
  virtual void solveMergingProcess(QueryMsg*);
  /** @brief Updates an entry in neighborCache
   *  @param index The index of the neighbor cache entry
   *  @param hello A received hello message
  */
  virtual void updateNeighborCacheEntry(int, HelloMsg*);
  /** @brief Solves external pending queries */
  virtual void solveExternalQueries();
  /** @brief (idle, spontaneously) -> A1: Wakes spontaneously this node up, 
   *  initalizes node state and starts a hello process*/
  class WakeUp;
  /** @brief (idle, receiving("hello")) -> A2: Wakes this node up, 
   *  initalizes node state and starts a hello process. */
  class BroadcastHello;
  /** @brief (updating, receiving("hello")) -> A3: Updates the neighbor cache
   *  in order to compute the link kind as well the minimum outgoing link.
   */
  class UpdateCache;
  /** @brief (processing, receiving("min")) -> A4: An intermediate node waits 
   *  for a minimum message comming from their corresponding children. When 
   *  receiving the minimum messages, this node updates both its outgoingLink
   *  and its outgoing port. Once this node receives all min messages from its
   *  children, it sends the local minimum value to its parent. This process
   *  is corresponds to the minimum computation via a converge cast.
   */
  class ComputeMin;
  /** @brief (connecting, receiving(query)) -> A5: Nodes forward the query to
   *  the outgoing link if the core of the cluster of this node produces it, otherwise, the query is pushed in the query externalQueryCache of this node.
   */
  class Connect;
  /** @brief (connecting, receiving(check)) -> A6: Nodes update their level,
   *  cid, and orientation if it is requiered. Then, they start an updating
   *  process if they have more than one possible outgoing link, otherwise, they
   *  start a connecting process through a convergecast
   */
  class Expand;
  /** @brief (connecting, receiving(done)) -> A7. Nodes broadcast to the
   *  spanning tree a termination message that indicates the global termination
   *  of the Mega-Merger protocol.
   */
  class Terminate;
  /** @brief 
   *  (updating, receiving(query) -> AnswerQuery or 
   *  (processing, receiving(query)) -> AnswerQuery. 
   *  This action is performed 
   *  when receiving a query message from a cluster with level less than own's.
   *  The reception of this query implies unlocking the updating process. The
   *  absorbed cluster must perform updating or connection according to its
   *  outgoing links.
   */
  class AnswerQuery;
  /** @brief This action is performed when receiving a hello message and
   *  the status of this node is not updating.
   */
  class UnexpectedHello;
public:
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
};

class MegaMerger::WakeUp : public BaseAction {
private:
  MegaMerger* node;
public:
  WakeUp(MegaMerger* n) : BaseAction("WakeUp"), node(n) { }
  void operator()(ImpulsePtr);
};

class MegaMerger::BroadcastHello : public BaseAction {
private:
  MegaMerger* node;
public:
  BroadcastHello(MegaMerger* n) : BaseAction("BroadcastHello"), node(n) { }
  void operator()(MsgPtr);
};

class MegaMerger::UpdateCache : public BaseAction {
private:
  MegaMerger* node;
public:
  UpdateCache(MegaMerger* n) : BaseAction("UpdateCache"), node(n) { }
  void operator()(MsgPtr);
};

class MegaMerger::ComputeMin : public BaseAction {
private:
  MegaMerger* node;
public:
  ComputeMin(MegaMerger* n) : BaseAction("ComputeMin"), node(n) { }
  void operator()(MsgPtr);
};

class MegaMerger::Connect : public BaseAction {
private:
  MegaMerger* node;
public:
  Connect(MegaMerger* n) : BaseAction("Connect"), node(n) { }
  void operator()(MsgPtr);
};

class MegaMerger::Expand : public BaseAction {
private:
  MegaMerger* node;
public:
  Expand(MegaMerger* n) : BaseAction("Expand"), node(n) { }
  void operator()(MsgPtr);
};

class MegaMerger::Terminate : public BaseAction {
private:
  MegaMerger* node;
public:
  Terminate(MegaMerger* n) : BaseAction("Terminate"), node(n) { }
  void operator()(MsgPtr);
};

class MegaMerger::AnswerQuery : public BaseAction {
private:
  MegaMerger* node;
public:
  AnswerQuery(MegaMerger* n) : BaseAction("AnswerQuery"), node(n) { }
  void operator()(MsgPtr);
};

class MegaMerger::UnexpectedHello : public BaseAction {
private:
  MegaMerger* node;
public:
  UnexpectedHello(MegaMerger* n) : BaseAction("UnexpectedHello"), node(n) { }
  void operator()(MsgPtr);
};

#endif // MEGAMERGER_H 



