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
  
   //*******************//
   //initialize your counters here//
   //*******************//
 
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
      else readMisses++;

      cacheLine *newline = fillLine(addr);
      if(op == 'w') newline->setFlags(DIRTY);   
	 
	  newline->setState(INVALIDATED);
	  
      
   }
   else
   {
      /**since it's a hit, update LRU and update dirty flag**/
      updateLRU(line);
      if(op == 'w') line->setFlags(DIRTY);
   }
   
   
   cacheLine * line2 = findLine(addr);
   printf("DEBUG3\n");
   currentTransaction = line2->doMsiReq(currentTransaction);
   printf("DEBUG03\n");
   return currentTransaction;
   
}

void Cache::Snoop(ulong addr, uchar op, int inst){
	cacheLine * line = findLine(addr);
	if (line == NULL) return; 
	line->doMsiSnoop(inst); 
}

//Does the Requestor side State Machine for MSI
int cacheLine::doMsiReq(int transaction){
	
	//returns bus intruction:
	// 0: -, 1: BusRd, 2: BusRdX, 3: BusUpgr
	
	// PrRd
	if(transaction == 1){
		if(isShared()){
			return 0;					
		} else if(isModified()){
			return 0;			
		}else if(isInvalidated()){
			setState(SHARED);
			return 1;
		}
	}
	
	//PrWr
	else {
		if(isShared()){
			setState(MODIFIED);
			return 3;
		} else if(isModified()){
			return 0;
		}else if(isInvalidated()){
			setState(MODIFIED);
			return 2;
		}	
	}
	
	return 0;
}

//Does the Snooper Side State Machine for MSI
int cacheLine::doMsiSnoop(int transaction){
	
	//returns Cache intruction:
	// 0: -, 1: Flush
	
	// -
	if(transaction == 0) return 0;
	
	// BusRd
	if(transaction == 1){
		if(isShared()){
			return 0;					
		} else if(isModified()){
			setState(SHARED);
			return 1;
		}else if(isInvalidated()){
			return 0;
		}
	}
	
	//BusRdX
	if(transaction == 2){
		if(isShared()){
			setState(INVALIDATED);
			return 0;					
		} else if(isModified()){
			setState(INVALIDATED);
			return 1;	
		}else if(isInvalidated()){
			return 0;
		}
	}
	
	//BusUpgr
	if(transaction == 3){
		if(isShared()){
			setState(INVALIDATED);
			return 0;					
		} else if(isModified()){
			return 0;	
		}else if(isInvalidated()){
			return 0;
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
   
   if(victim->getFlags() == DIRTY) {
      writeBack(addr);
   }
      
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(VALID);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::printStats()
{ 
   printf("===== Simulation results      =====\n");
   /****print out the rest of statistics here.****/
   /****follow the ouput file format**************/
}
