////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_cache.c
//  Description    : This is the implementation of the cache for the 
//                   FS3 filesystem interface.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Sun 17 Oct 2021 09:36:52 AM EDT
//

// Includes
#include <cmpsc311_log.h>

// Project Includes
#include <fs3_cache.h>

//
// Support Macros/Data

CACHE myCache;

//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_init_cache
// Description  : Initialize the cache with a fixed number of cache lines
//
// Inputs       : cachelines - the number of cache lines to include in cache
// Outputs      : 0 if successful, -1 if failure

int fs3_init_cache(uint16_t cachelines) {
    int i;

    //Checks if cache is already initialized
    if(myCache.initialized != 1){
        //Allocates cacheBlocks
        if((myCache.cacheLines = malloc(cachelines*sizeof(CACHE_LINE))) != NULL){
            //Sets cache variables
            myCache.size = cachelines;
            myCache.initialized = 1;
            myCache.lastAccessedLine = 0;

            //Sets all lines to defult values
            for(i=0; i<myCache.size;i++){
                myCache.cacheLines[i].sectorBytes = NULL;
                myCache.cacheLines[i].sectorIndex = 0;
                myCache.cacheLines[i].trackIndex = 0;
            }

            //Sets all cache stats to zero
            myCache.stats.gets = 0;
            myCache.stats.hits = 0;
            myCache.stats.inserts = 0;
            myCache.stats.misses = 0;

            logMessage(LOG_OUTPUT_LEVEL, "Succesfully initialized cache with %d lines",cachelines);
            return(0);
        }
        else{
            logMessage(FS3DriverLLevel, "Failed to initialized cache with %d lines",cachelines);
            return(-1);
        }
    }
    else{
        logMessage(FS3DriverLLevel, "Cache already initialized");
        return(-1);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_close_cache
// Description  : Close the cache, freeing any buffers held in it
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int fs3_close_cache(void)  {
    int i;

    //Checks if cache is initialized
    if(myCache.initialized == 1){

        //Frees all sectors in cache
        for(i=0;i<myCache.cacheLinesTaken;i++){
            free(myCache.cacheLines[i].sectorBytes);
            myCache.cacheLines[i].sectorBytes = NULL;
        }

        //Frees cacheBlocks
        free(myCache.cacheLines);
        myCache.cacheLines = NULL;

        logMessage(FS3DriverLLevel, "Cache closed, deleted %d items", myCache.cacheLinesTaken);
        return(0);
    }
    else{
        logMessage(FS3DriverLLevel, "Cache already closed");
        return(-1);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_put_cache
// Description  : Put an element in the cache
//
// Inputs       : trk - the track number of the sector to put in cache
//                sct - the sector number of the sector to put in cache
// Outputs      : 0 if inserted, -1 if not inserted

int fs3_put_cache(FS3TrackIndex trk, FS3SectorIndex sct, void *buf) {
    CACHE_LINE newCacheLine;

    //Checks if cache is initalized
    if(myCache.initialized == 1){

        //Checks if sector already in cache
        if(myCache.containedSectors[trk][sct].contains == 1){
            //Updates if already in cache
            memcpy(myCache.cacheLines[myCache.containedSectors[trk][sct].loc].sectorBytes, buf, FS3_SECTOR_SIZE);

            //Sets to least recently used
            myCache.lastAccessedLine = myCache.containedSectors[trk][sct].loc;

            logMessage(LOG_INFO_LEVEL, "Updated cache item Trk %d Sct %d", myCache.cacheLines[myCache.containedSectors[trk][sct].loc].trackIndex, 
            myCache.cacheLines[myCache.containedSectors[trk][sct].loc].sectorIndex);
            return(0);
        }

        //Sets up new cache line to put in cache
        newCacheLine.trackIndex = trk;
        newCacheLine.sectorIndex = sct;
        newCacheLine.sectorBytes = malloc(FS3_SECTOR_SIZE);
        memcpy(newCacheLine.sectorBytes, buf, FS3_SECTOR_SIZE);

        //Checks if cache is full
        if(myCache.cacheLinesTaken == myCache.size){
            
        logMessage(LOG_INFO_LEVEL, "Ejecting cache item Trk %d Sct %d", myCache.cacheLines[myCache.lastAccessedLine].trackIndex, 
            myCache.cacheLines[myCache.lastAccessedLine].sectorIndex);

        //Ejects LRU cache line 
        myCache.containedSectors[myCache.cacheLines[myCache.lastAccessedLine].trackIndex][myCache.cacheLines[myCache.lastAccessedLine].sectorIndex].contains = 0;
        free(myCache.cacheLines[myCache.lastAccessedLine].sectorBytes);
        myCache.cacheLines[myCache.lastAccessedLine].sectorBytes = NULL;

         //Adds new cache line
        myCache.cacheLines[myCache.lastAccessedLine] = newCacheLine;
        myCache.containedSectors[trk][sct].contains = 1;
        myCache.containedSectors[trk][sct].loc = myCache.lastAccessedLine;
        }
        else{
            //Adds new cache line
            myCache.cacheLinesTaken++;
            myCache.cacheLines[myCache.cacheLinesTaken-1] = newCacheLine;
            myCache.containedSectors[trk][sct].contains = 1;
            myCache.containedSectors[trk][sct].loc = myCache.cacheLinesTaken-1;
        }
        logMessage(LOG_INFO_LEVEL, "Added cache item Trk %d Sct %d", trk, sct);
        myCache.stats.inserts++;
        return(0);
    }
    else{
        logMessage(FS3DriverLLevel, "Cache not initialized");
        return(-1);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_get_cache
// Description  : Get an element from the cache (
//
// Inputs       : trk - the track number of the sector to find
//                sct - the sector number of the sector to find
// Outputs      : returns NULL if not found or failed, pointer to buffer if found

void * fs3_get_cache(FS3TrackIndex trk, FS3SectorIndex sct)  {

    //Checks if cache is initalized
    if(myCache.initialized == 1){

        myCache.stats.gets++;

        if(myCache.containedSectors[trk][sct].contains == 1){

            //Sector found
            logMessage(LOG_INFO_LEVEL, "Getting cache item Trk %d Sct %d (found!)", trk, sct);

            //Sets time accessed
            myCache.lastAccessedLine = myCache.containedSectors[trk][sct].loc;

            myCache.stats.hits++;
            return(myCache.cacheLines[myCache.containedSectors[trk][sct].loc].sectorBytes);
        }

        //Sector not in cache
        logMessage(LOG_INFO_LEVEL, "Getting cache item Trk %d Sct %d (not found!)", trk, sct);
        myCache.stats.misses++;
        return(NULL);
    }
    else{
        logMessage(FS3DriverLLevel, "Cache not initialized");
        return(NULL);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_log_cache_metrics
// Description  : Log the metrics for the cache 
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int fs3_log_cache_metrics(void) {
    float hitRatio;

    //Logs all chache matrics
    logMessage(LOG_OUTPUT_LEVEL, "** FS3 cache Metrics **");
    logMessage(LOG_OUTPUT_LEVEL, "Cache inserts    [%d]", myCache.stats.inserts);
    logMessage(LOG_OUTPUT_LEVEL, "Cache gets       [%d]", myCache.stats.gets);
    logMessage(LOG_OUTPUT_LEVEL, "Cache hits       [%d]", myCache.stats.hits);
    logMessage(LOG_OUTPUT_LEVEL, "Cache misses     [%d]", myCache.stats.misses);

    //Calculates hit ratio
    if(myCache.stats.gets != 0){
        hitRatio = (float)((float)myCache.stats.hits/(float)myCache.stats.gets)*100.0;
    }
    else {
        hitRatio = 0;
    }
    logMessage(LOG_OUTPUT_LEVEL, "Cache hit ratio  [%%%.2f]", hitRatio);
    return(0);
}