#ifndef FS3_CACHE_INCLUDED
#define FS3_CACHE_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_cache.h
//  Description    : This is the interface for the sector cache in the FS3
//                   filesystem.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Sun 17 Oct 2021 09:36:52 AM EDT
//

// Include
#include <fs3_controller.h>
#include <stdlib.h>
#include <string.h>
#include <fs3_common.h>

// Defines
#define FS3_DEFAULT_CACHE_SIZE 2048; // 2048 cache entries, by default

//Structures

typedef struct
{
    int contains;   //If sector is in cache
    int loc;        //Where in cache is sector
} CACHE_SECTOR;

typedef CACHE_SECTOR CacheTrack[FS3_TRACK_SIZE]; // A track
typedef struct
{
    FS3TrackIndex trackIndex;           //Track index of sector in cache block
    FS3SectorIndex sectorIndex;         //Sector index of sector in cache block
    char *sectorBytes;                  //Bytes of sector in cache block
} CACHE_LINE;

typedef struct
{
    int inserts;       //Tracks insterts into cache
    int gets;          //Tracks gets of cache
    int hits;          //Tracks hits in cache
    int misses;        //Tracks misses in cache
} CACHE_STATS;
typedef struct
{
    CACHE_LINE *cacheLines;                        //Array of cache lines
    uint16_t size;                                 //Size of cache
    int initialized;                               //Keeps track if cache is initialized (1:true)
    CACHE_STATS stats;                             //Stats of cache
    uint16_t cacheLinesTaken;                      //Number of chache lines taken
    int lastAccessedLine;                          //Last line accessed
    CacheTrack containedSectors[FS3_MAX_TRACKS];     //Keeps of sectors in cache for fast search
} CACHE;

//
// Cache Functions

int fs3_init_cache(uint16_t cachelines);
    // Initialize the cache with a fixed number of cache lines

int fs3_close_cache(void);
    // Close the cache, freeing any buffers held in it

int fs3_put_cache(FS3TrackIndex trk, FS3SectorIndex sct, void *buf);
    // Put an element in the cache

void * fs3_get_cache(FS3TrackIndex trk, FS3SectorIndex sct);
    // Get an element from the cache (returns NULL if not found)

int fs3_log_cache_metrics(void);
    // Log the metrics for the cache 

#endif
