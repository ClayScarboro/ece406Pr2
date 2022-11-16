/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b )
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   totalMissRate = 0;
   cache2cache = 0;
   memoryTransactions = 0;
   interventions = 0;
   invalidations = 0;
   flushes = 0;
   BusRdX = 0;
   BusUpgr = 0;
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
      tagMask <<= 1;
      tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
         cache[i][j].invalidate();
      }
   }      
   
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
int Cache::Access(ulong addr,uchar op)
{
	
	// 1 = PrRd; 2 = PrWr;
	int currentTransaction = 0;
	int snoopTransaction = 0;
	
	printf("%c ",op);
	
   currentCycle++;/*per cache global counter to maintain LRU order 
                    among cache ways, updated on every cache access*/
         
   if(op == 'w') writes++;
   else          reads++;
   
   if(op == 'w') currentTransaction = 2;
   else			 currentTransaction = 1;
   
   cacheLine * line = findLine(addr);
   if(line == NULL)/*miss*/
   {
      if(op == 'w') writeMisses++;
      else {
		  memoryTransactions++;
		  readMisses++;
	  }
	  
      cacheLine *newline = fillLine(addr);
      //if(op == 'w')	newline->setFlags(MODIFIED);   
      snoopTransaction = doMsiReq(newline,currentTransaction);
	  
   } 
   else 
   {
      /**since it's a hit, update LRU and update dirty flag**/
      updateLRU(line);
      //if(op == 'w') line->setFlags(MODIFIED);
	  snoopTransaction = doMsiReq(line,currentTransaction);
   }
   
   printf("SnoopTrans: %d\n",snoopTransaction);
   
   if (snoopTransaction == 2){ BusRdX++; memoryTransactions++; }
   //else if (snoopTransaction == 3) BusUpgr++;
   return snoopTransaction;
  
}

void Cache::Snoop(ulong addr, uchar op, int inst){
	cacheLine * line = findLine(addr);
	int doFlush;
	if (line == NULL) return;
	doFlush = doMsiSnoop(line,inst); 
	if(doFlush == 2 || doFlush == -2) ++flushes;
}

//Does the Requestor side State Machine for MSI
int Cache::doMsiReq(cacheLine * line,int transaction){
	
	//returns bus intruction:
	// 0: -, 1: BusRd, 2: BusRdX
	
	// PrRd
	if(transaction == 1){
		printf("read:");
		if(line->isShared()){
			printf("shared\n");
			return 0;					
		} else if(line->isModified()){
			printf("mod\n");
			return 0;			
		}else if(line->isInvalidated()){
			printf("invalid\n");
			line->setFlags(SHARED);
			return 1;
		}
	}
	
	//PrWr
	else {
		printf("write:");
		if(line->isShared()){
			line->setFlags(MODIFIED);
			printf("shared\n");
			return 2;
		} else if(line->isModified()){
			printf("mod\n");
			return 0;
		}else if(line->isInvalidated()){
			line->setFlags(MODIFIED);
			printf("invalid\n");
			return 2;
		}	
	}
	
	return 0;
}

//Does the Snooper Side State Machine for MSI
int Cache::doMsiSnoop(cacheLine * line, int transaction){
	
	//returns Cache intruction:
	// 1: -, 2: Flush, -# = flush;
	
	// -
	if(transaction == 0) return 0;
	
	// BusRd
	if(transaction == 1){
		if(line->isShared()){
			return 1;					
		} else if(line->isModified()){
			line->setFlags(SHARED);
			return 2;
		}else if(line->isInvalidated()){
			return 1;
		}
	}
	
	//BusRdX
	if(transaction == 2){
		if(line->isShared()){
			line->invalidate();
			invalidations++;
			return -1;					
		} else if(line->isModified()){
			line->invalidate();
			invalidations++;
			return -2;	
		}else if(line->isInvalidated()){
			return 1;
		}
	}
	return 0;
	
}





/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
   if(cache[i][j].isValid()) {
      if(cache[i][j].getTag() == tag)
      {
         pos = j; 
         break; 
      }
   }
   if(pos == assoc) {
      return NULL;
   }
   else {
      return &(cache[i][pos]); 
   }
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
   line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) { 
         return &(cache[i][j]); 
      }   
   }

   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].getSeq() <= min) { 
         victim = j; 
         min = cache[i][j].getSeq();}
   } 

   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   
   if(victim->getFlags() == MODIFIED) {
      writeBack(addr);
   }
      
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(SHARED);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats(int proc)
{ 

	totalMissRate = (((float) (readMisses + writeMisses)) / (reads + writes)) * 100;

   printf("============ Simulation results (Cache %d) ============n",proc);
   printf("01. number of reads: %lu\n",reads);
   printf("02. number of read misses: %lu\n",readMisses);
   printf("03. number of writes: %lu\n",writes);
   printf("04. number of write misses: %lu\n",writeMisses);
   printf("05. total miss rate: %2.2f",totalMissRate);
   printf("%%\n");
   printf("06. number of writebacks: %lu\n",writeBacks);
   printf("07. number of cache-to-cache transfers: %d\n",cache2cache);
   printf("08. number of memory transactions: %d\n",memoryTransactions);
   printf("09. number of interventions: %d\n",interventions);
   printf("10. number of invalidations: %d\n",invalidations);
   printf("11. number of flushes: %d\n",flushes);
   printf("12. number of BusRdX: %d\n",BusRdX);
   printf("13. number of BusUpgr: %d\n",BusUpgr);
}
