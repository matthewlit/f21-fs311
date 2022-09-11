#ifndef FS3_DRIVER_INCLUDED
#define FS3_DRIVER_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_driver.h
//  Description    : This is the header file for the standardized IO functions
//                   for used to access the FS3 storage system.
//
//  Author         : Patrick McDaniel
//  Last Modified  : Sun 19 Sep 2021 08:12:43 AM PDT
//

// Include files
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fs3_controller.h>
#include <fs3_cache.h>
#include <fs3_common.h>
#include <fs3_network.h>

// Defines
#define FS3_MAX_TOTAL_FILES 1024 // Maximum number of files ever
#define FS3_MAX_PATH_LENGTH 128 // Maximum length of filename length
#define FS3_MAX_TRACK_SECTOR_PAIRS FS3_MAX_TRACKS*FS3_TRACK_SIZE	//Max amount of sectors of file
#define FS3_MAX_FILE_LENGTH FS3_MAX_TRACK_SECTOR_PAIRS*FS3_MAX_SECTOR_SIZE	//Max amount of bytes of file

//File structure

//TRACK_SECTOR_PAIR structure
typedef struct
{
	FS3TrackIndex trackIndex;	//Track index
	FS3SectorIndex sectorIndex;	//Sector index
}TRACK_SECTOR_PAIR;
typedef struct
{
	char name[FS3_MAX_PATH_LENGTH];							//Name of file
	int16_t fileHandle;										//Unique file handle of file
	int open;												//If file is open(1 True : 0 False)
	TRACK_SECTOR_PAIR loc[FS3_MAX_TRACK_SECTOR_PAIRS];		//Track and sector locations of file
	uint32_t pos;											//Postition of file pointer
	uint32_t length;										//Length of file
	int numOfSectors;										//Number of sectors the file spans
}FILE_INFO;

// Disk structure
typedef struct
{
	int mounted;							//If disk is mounted(1 True : 0 False)
	FILE_INFO files[FS3_MAX_TOTAL_FILES];	//Files on disk
	FS3TrackIndex currentTrackIndex;		//Current track of disk your in
	int nextSector;							//Next Sector to write
	int nextTrack;							//Next Track to write
}DISK;

//
// Interface functions

int32_t fs3_mount_disk(void);
	// FS3 interface, mount/initialize filesystem

int32_t fs3_unmount_disk(void);
	// FS3 interface, unmount the disk, close all files

int16_t fs3_open(char *path);
	// This function opens a file and returns a file handle

int16_t fs3_close(int16_t fd);
	// This function closes a file

int32_t fs3_read(int16_t fd, void *buf, int32_t count);
	// Reads "count" bytes from the file handle "fh" into the buffer  "buf"

int32_t fs3_write(int16_t fd, void *buf, int32_t count);
	// Writes "count" bytes to the file handle "fh" from the buffer  "buf"

int32_t fs3_seek(int16_t fd, uint32_t loc);
	// Seek to specific point in the file

FS3CmdBlk construct_fs3cmdblock(uint8_t op, uint16_t sec, uint_fast32_t trk, uint8_t ret);
	// Create an FS3 array opcode from the variable feilds 

int deconstruct_fs3cmdblock(FS3CmdBlk cmdblock, uint8_t *op, uint16_t *sec, uint_fast32_t *trk, uint8_t *ret);
	// Extract reqister state from bus values

FILE_INFO * get_file(int16_t fd);
	// Gets file associated with file handle if vailid file handle

int32_t tseek(FS3TrackIndex track);
	// Seeks track to given track 

int32_t get_free_track_sector_pair(TRACK_SECTOR_PAIR *pair);
	//Finds a free track and sector pair for a file

#endif
