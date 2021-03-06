//
// Generated file, do not edit! Created by nedtool 5.6 from QueryMsg.msg.
//

#ifndef __QUERYMSG_M_H
#define __QUERYMSG_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0506
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



// cplusplus {{
  #include "Event.h"
// }}

/**
 * Class generated from <tt>QueryMsg.msg:5</tt> by nedtool.
 * <pre>
 * message QueryMsg
 * {
 *     name = "query";
 *     kind = EventKind::QUERY;
 *     int level;
 *     int cid;
 * }
 * </pre>
 */
class QueryMsg : public ::omnetpp::cMessage
{
  protected:
    int level;
    int cid;

  private:
    void copy(const QueryMsg& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const QueryMsg&);

  public:
    QueryMsg(const char *name=nullptr, short kind=0);
    QueryMsg(const QueryMsg& other);
    virtual ~QueryMsg();
    QueryMsg& operator=(const QueryMsg& other);
    virtual QueryMsg *dup() const override {return new QueryMsg(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual int getLevel() const;
    virtual void setLevel(int level);
    virtual int getCid() const;
    virtual void setCid(int cid);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const QueryMsg& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, QueryMsg& obj) {obj.parsimUnpack(b);}


#endif // ifndef __QUERYMSG_M_H

