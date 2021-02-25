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

#include <vector>
#include <array>

#include "BaseNode.h"
#include "HelloMsg_m.h"

/** @brief This class describes the procedures and elements of nodes obeying
 *  the Mega-Merger protocol. All members of this class must be public in order
 *  of actions being able to access both members and member-functions.
 *  @author A.G. Medrano-Chavez.
*/
class MegaMerger : public BaseNode {
public:
  typedef std::array<int,5> CacheEntry;
protected:
  enum RowIndex {
    PORT_NUMBER = 0,
    ID,
    CID,
    LINK_KIND
  };
  enum LinkKind {
    INTERNAL = 0,
    OUTGOING,
    BRANCH,
    UNKNOWN
  };
  /** @brief Holds the port number, the id, the cid, the level and the kind of
   * link of a neighbor from N(x)
   */
  std::vector<CacheEntry> neighborCache;
  /** @brief The neighborhood size */
  unsigned neighborhoodSize;
  /** @brief Flag indicating this node is the core of its cluster */
  bool core;
  /** @brief The number unique ID of this node */
  int uid;
  /** @brief The ID of the node's cluster */
  int cid;
  /** @brief The port connecting this node with its parent */
  omnetpp::cGate* parent;
  /** @brief The number of friendly merging this cluster performs */
  int level;
  /** @brief The number of hello messages this node receives. */
  unsigned helloCounter;
protected:
  /** @brief Composes a hello message, then the message is locally broadcasted*/
  virtual void sendHello();
  /** @brief Processes a hello message updating the neighbor cache */
  virtual void processHello(MsgPtr);
  /** @brief (idle, spontaneously) -> A1: Wakes spontaneously this node up, 
   *  initalizes node state and starts a hello process*/
  class ActionOne;
  /** @brief (idle, receiving("hello")) -> A2: Wakes this node up, 
   *  initalizes node state and starts a hello process*/
  class ActionTwo;
  /** @brief (updating, receiving("hello")) -> A3: Updates the neighbor cache
   *  in order to compute the link kind as well the minimum outgoing link
   */
  class ActionThree;
public:
  /** @brief Default constructor */
  MegaMerger();
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

class MegaMerger::ActionOne : public BaseAction {
private:
  MegaMerger* node;
public:
  ActionOne(MegaMerger* n) : BaseAction("ActionOne"), node(n) { }
  void operator()(ImpulsePtr);
};

class MegaMerger::ActionTwo : public BaseAction {
private:
  MegaMerger* node;
public:
  ActionTwo(MegaMerger* n) : BaseAction("ActionTwo"), node(n) { }
  void operator()(MsgPtr);
};

class MegaMerger::ActionThree : public BaseAction {
private:
  MegaMerger* node;
public:
  ActionThree(MegaMerger* n) : BaseAction("ActionThree"), node(n) { }
  void operator()(MsgPtr);
};


#endif // MEGAMERGER_H 



