/*******************************************************
                          cache.h
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum {
   INVALID = 0,
   SHARED,
   MODIFIED,
   EXCLUSIVE,
};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:Shared, 2:Modified 
   ulong seq; 
 
public:
   cacheLine()                { tag = 0; Flags = 0; }
   ulong getTag()             { return tag; }
   ulong getFlags()           { return Flags;}
   ulong getSeq()             { return seq; }
   void setSeq(ulong Seq)     { seq = Seq;}
   void setFlags(ulong flags) {  Flags = flags;}
   void setTag(ulong a)       { tag = a; }
   void invalidate()          { tag = 0; Flags = INVALID; } //useful function
   
   bool isValid()             { return ((Flags) != INVALID); }
   bool isShared()             { return ((Flags) == SHARED); }
   bool isModified()             { return ((Flags) == MODIFIED); }
   bool isInvalidated()             { return ((Flags) == INVALID); }
   bool isExclusive()             { return ((Flags) == EXCLUSIVE); }
};

class Cache
{
protected:
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks;

   //******///
   //add coherence counters here///
   //******///

   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)   { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag) { return (tag << (log2Blk));}
   
public:
    ulong currentCycle;  
     
    Cache(int,int,int);
   ~Cache() { delete cache;}
   
   
   //Counters
   float totalMissRate;
   int cache2cache;
   int memoryTransactions;
   int interventions;
   int invalidations;
   int flushes;
   int BusRdX;
   int BusUpgr;
   int wastedSnoop;
   int usefulSnoop;
   int filteredSnoop;
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   
   ulong getRM()     {return readMisses;} 
   ulong getWM()     {return writeMisses;} 
   ulong getReads()  {return reads;}       
   ulong getWrites() {return writes;}
   ulong getWB()     {return writeBacks;}
   
   void writeBack(ulong) {writeBacks++; memoryTransactions++;}
   int Access(ulong,uchar);
   int AccessMSIBus(ulong,uchar);
   int AccessMESI(ulong,uchar,int);
   int AccessMESISnoop(ulong,uchar,int);
   void Snoop(ulong,uchar,int);
   void SnoopMSIBus(ulong,uchar,int);
   void SnoopMESI(ulong,uchar,int);
   void SnoopMESISnoop(ulong,uchar,int);
   void printStats(int,int);
   void updateLRU(cacheLine *);
   int doMsiReq(cacheLine *,int);
   int doMsiSnoop(cacheLine *,int);
   int doMsiBusReq(cacheLine *,int);
   int doMsiBusSnoop(cacheLine *,int);
   int doMESIReq(cacheLine *,int,int);
   int doMESISnoop(cacheLine *,int);
   int doMESISnoopReq(cacheLine *,int,int);
   int doMESISnoopSnoop(cacheLine *,int);

   //******///
   //add other functions to handle bus transactions///
   //******///
   //returns bus intruction:
	// 0: -, 1: BusRd, 2: BusRdX, 3: BusUpgr

};

#endif
