/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;

#include "cache.h"
int main(int argc, char *argv[])
{
    
    ifstream fin;
    FILE * pFile;

    if(argv[1] == NULL){
         printf("input format: ");
         printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
         exit(0);
        }

    ulong cache_size     = atoi(argv[1]);
    ulong cache_assoc    = atoi(argv[2]);
    ulong blk_size       = atoi(argv[3]);
    ulong num_processors = atoi(argv[4]);
    ulong protocol       = atoi(argv[5]); /* 0:MSI 1:MSI BusUpgr 2:MESI 3:MESI Snoop FIlter */
    char *fname        = (char *) malloc(20);
    fname              = argv[6];

    printf("===== 506 Coherence Simulator Configuration =====\n");
    printf("L1_SIZE: %lu\n",cache_size);
	printf("L1_ASSOC: %lu\n",cache_assoc);
	printf("L1_BLOCKSIZE: %lu\n",blk_size);
	printf("NUMBER OF PROCESSORS: %lu\n",num_processors);
	printf("COHERENCE PROTOCOL: ");
	if(protocol == 0) printf("MSI\n");
	if(protocol == 1) printf("MSI BusUpgr\n");
	if(protocol == 2) printf("MESI\n");
	if(protocol == 3) printf("MESI Snoop Filter\n");
	printf("TRACE FILE: %s\n",fname);
    
    // Using pointers so that we can use inheritance */
    Cache** cacheArray = (Cache **) malloc(num_processors * sizeof(Cache));
    for(ulong i = 0; i < num_processors; i++) {
		// if(protocol == 0) {
            cacheArray[i] = new Cache(cache_size, cache_assoc, blk_size);
        //}
    }

    pFile = fopen (fname,"r");
    if(pFile == 0)
    {   
        printf("Trace file problem\n");
        exit(0);
    }
    
    ulong proc;
    char op;
    ulong addr;
	int inst;

    int line = 1;
	int doMorePrint = 0;
    while(fscanf(pFile, "%lu %c %lx", &proc, &op, &addr) != EOF)
    {
#ifdef _DEBUG
        //printf("%d\n", line);
#endif
		
		if(protocol == 0){
		// --------------- MSI ------------------
			//Cache and Requestor
			inst = cacheArray[proc]->Access(addr,op);
			
			
			//Snooper
			for(ulong i = 0; i < num_processors; i++){
				if(proc == i ) continue;
				else cacheArray[i]->Snoop(addr,op,inst);
			}
		} else if (protocol == 1){
			// --------------- MSIBusUpgr ------------------
			//Cache and Requestor
			inst = cacheArray[proc]->AccessMSIBus(addr,op);
			
			
			//Snooper
			for(ulong i = 0; i < num_processors; i++){
				if(proc == i ) continue;
				else cacheArray[i]->SnoopMSIBus(addr,op,inst);
			}


		} else if (protocol == 2){
			// --------------- MESI ------------------
			//Cache and Requestor
			
			int onlyCopy = 1;
			for(ulong i = 0; i < num_processors; i++){
				cacheLine * check;
				if(proc == i ) continue;
				else check = cacheArray[i]->findLine(addr);
				if(check != NULL) onlyCopy = 0;
			}
			
			inst = cacheArray[proc]->AccessMESI(addr,op,onlyCopy);
			
			
			//Snooper
			for(ulong i = 0; i < num_processors; i++){
				if(proc == i ) continue;
				else cacheArray[i]->SnoopMESI(addr,op,inst);
			}


		} else if (protocol == 3){
			// --------------- MESI Snoop Filter ------------------
			//Cache and Requestor
			doMorePrint = 1;
			int onlyCopy = 1;
			for(ulong i = 0; i < num_processors; i++){
				cacheLine * check;
				if(proc == i ) continue;
				else check = cacheArray[i]->findLine(addr);
				if(check != NULL) onlyCopy = 0;
			}
			
			
			inst = cacheArray[proc]->AccessMESISnoop(addr,op,onlyCopy);
			
			
			//Snooper
			for(ulong i = 0; i < num_processors; i++){
				if(proc == i ) continue;
				else cacheArray[i]->SnoopMESISnoop(addr,op,inst);
			}


		}			
		
        line++;
    }

    fclose(pFile);

    for(int i = 0; i < (int)num_processors; i++){
		cacheArray[i]->printStats(i,doMorePrint);
	}
    
}
