////////////////////////////////////////////////////////////////////////////////
//
//  File           : fs3_driver.c
//  Description    : This is the implementation of the standardized IO functions
//                   for used to access the FS3 storage system.
//
//   Author        : Matthew Kelleher
//   Last Modified : 12/1/21
//

// Includes
#include <string.h>
#include <cmpsc311_log.h>

// Project Includes
#include "fs3_driver.h"

//
// Defines
#define SECTOR_INDEX_NUMBER(x) ((int)(x/FS3_SECTOR_SIZE))

//
// Static Global Variables
DISK my_disk;
int16_t fileHandleCounter;

//
// Implementation

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_mount_disk
// Description  : FS3 interface, mount/initialize filesystem
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_mount_disk(void) {
	FS3CmdBlk mount;
	FS3CmdBlk cmd;
	uint8_t returnVal;

	//Checks if disk is already mounted
	if(my_disk.mounted != 1){
		//Mounts disk
		cmd = construct_fs3cmdblock(FS3_OP_MOUNT,0,0,0);
		if(network_fs3_syscall(cmd,&mount,NULL)==-1){
			//Failed syscall
			return(-1);
		}

		//Checks if mount is successful
		deconstruct_fs3cmdblock(mount,NULL,NULL,NULL,&returnVal);
		if (returnVal == 0){
			//Mount successful
			logMessage(FS3DriverLLevel, "FS3 DRVR: Mounted");
			my_disk.mounted = 1;
			tseek(0);
			my_disk.currentTrackIndex = 0;
			return(0);
		}
		else {
			//Mount failed
			logMessage(FS3DriverLLevel, "FS3 DRVR:  Mounting Failed");
			my_disk.mounted = 0;
			return(-1);
		}
	}
	else{
		//Disk already mounted
		logMessage(FS3DriverLLevel, "FS3 DRVR:  Disk already mounted");
		return(-1);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_unmount_disk
// Description  : FS3 interface, unmount the disk, close all files
//
// Inputs       : none
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_unmount_disk(void) {
	FS3CmdBlk unmount;
	FS3CmdBlk cmd;
	uint8_t returnVal;
	int i;

	if(my_disk.mounted != 0){
		//Unmounts disk
		cmd = construct_fs3cmdblock(FS3_OP_UMOUNT,0,0,0);
		if(network_fs3_syscall(cmd,&unmount,NULL)==-1){
			//Failed syscall
			return(-1);
		}

		//Checks if unmount is successful
		deconstruct_fs3cmdblock(unmount,NULL,NULL,NULL,&returnVal);
		if (returnVal == 0){
			//Unmount successful
			logMessage(FS3DriverLLevel, "FS3 DRVR: Unmounted");
			my_disk.mounted = 0;
			my_disk.currentTrackIndex = 0;

			//Closes all files
			for(i = 0; i<FS3_MAX_TOTAL_FILES; i++){
				my_disk.files[i].open = 0;
				my_disk.files[i].pos = 0;
			}
			return(0);
		}
		else {
			//Unmount failed
			logMessage(FS3DriverLLevel, "FS3 DRVR:  Unmounting Failed");
			return(-1);
		}
	}
	else{
		//Disk already unmounted
		logMessage(FS3DriverLLevel, "FS3 DRVR:  Disk already unmounted");
		return(-1);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_open
// Description  : This function opens the file and returns a file handle
//
// Inputs       : path - filename of the file to open
// Outputs      : file handle if successful, -1 if failure

int16_t fs3_open(char *path) {
	int i;
	int16_t fileHandle;
	TRACK_SECTOR_PAIR tempPair;

	//Set file descriptor
	fileHandle = fileHandleCounter;

	//Checks if file already exists
	for (i=0;i<FS3_MAX_TOTAL_FILES;i++){
		if(strcmp(my_disk.files[i].name,path) == 0){

			//Checks if file is already open
			if(my_disk.files[i].open == 1){
				logMessage(FS3DriverLLevel, "File already open");
				return(my_disk.files[i].fileHandle);
			}

			//Opens file if it exists and not yet open
			else{
				logMessage(FS3DriverLLevel, "Driver opening existing file [%s]",path);

				//Updates file information
				my_disk.files[i].open = 1;
				my_disk.files[i].pos = 0;

				return (my_disk.files[i].fileHandle); 
			}
		}
	}
	//Creates new file if didn't already exist
	logMessage(FS3DriverLLevel, "Driver creating new file [%s]",path);
	i=0;
	while(my_disk.files[i].name[0] != '\0'){
		i++;
	}

	//Saves file information 
	strcpy(my_disk.files[i].name, path);
	my_disk.files[i].fileHandle = fileHandle;
	my_disk.files[i].open = 1;
	my_disk.files[i].pos = 0;
	my_disk.files[i].length = 0;
	my_disk.files[i].numOfSectors = 0;

	//Sets starting track and sector of file
	if(get_free_track_sector_pair(&tempPair) == 0){
		my_disk.files[i].loc[0] = tempPair;
		my_disk.files[i].numOfSectors++;
	}
	else{
		logMessage(FS3DriverLLevel, "FS3 driver: failed to allocat fs3 track and sector");
		return(-1);
	}

	logMessage(FS3DriverLLevel, "File [%s] opened in driver, fh, %d.",my_disk.files[i].name,my_disk.files[i].fileHandle);
	logMessage(FS3DriverLLevel, "FS3 driver: allocated fs3 track %d, sector %d for fh/index %d/%d"
		,my_disk.files[i].loc[0].trackIndex,my_disk.files[i].loc[0].sectorIndex,my_disk.files[i].fileHandle,my_disk.files[i].pos);
	fileHandleCounter++;
	return (fileHandle); 
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_close
// Description  : This function closes the file
//
// Inputs       : fd - the file descriptor
// Outputs      : 0 if successful, -1 if failure

int16_t fs3_close(int16_t fd) {
	FILE_INFO *file;

	//Gets reference to file from file handle (returns NULL file handle not associated with file or file not open)
	file = get_file(fd);

	//Checks if file exsits and is open
	if(file != NULL){
		//Closes file
		file->open = 0;
		file->pos = 0;
		return(0);
	}
	else{
		//File handle not associated with file or file not open
		return(-1);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_read
// Description  : Reads "count" bytes from the file handle "fh" into the 
//                buffer "buf"
//
// Inputs       : fd - filename of the file to read from
//                buf - pointer to buffer to read into
//                count - number of bytes to read
// Outputs      : bytes read if successful, -1 if failure

int32_t fs3_read(int16_t fd, void *buf, int32_t count) {
	FILE_INFO *file;
	FS3CmdBlk read;
	FS3CmdBlk cmd;
	uint8_t returnVal;
	char *temp_buf;;
	char *file_buf;
	int i;
	int totalBytesToRead;
	int bytesRead;
	void *cacheReturn;
	int endSector;

	//Gets reference to file from file handle (returns NULL file handle not associated with file or file not open)
	file = get_file(fd);
	
	//Checks if file exsits and is open
	if(file != NULL){
		
		//Tracks bytes needed to still read and bytes read on current sector
		totalBytesToRead = (1+(SECTOR_INDEX_NUMBER((file->pos + count))-SECTOR_INDEX_NUMBER(file->pos)))*FS3_SECTOR_SIZE;
		bytesRead =0;

		//Buf of all file bytes needed
		file_buf = malloc(totalBytesToRead);
		temp_buf = malloc(FS3_SECTOR_SIZE);

		//Find ending sector of read
		if(count % FS3_SECTOR_SIZE == 0){
			endSector = (SECTOR_INDEX_NUMBER((file->pos + count))-1);
		}
		else{
			endSector = SECTOR_INDEX_NUMBER((file->pos + count));
		}

		//Seeks to all tracks and sectors of given file and reads
		for(i = SECTOR_INDEX_NUMBER(file->pos); i <= endSector; i++){

			//Gets cache
			cacheReturn = fs3_get_cache(file->loc[i].trackIndex,file->loc[i].sectorIndex);
			//Checks if sector is in cache
			if(cacheReturn==NULL){
				//Reads sector of given file
				if (file->loc[i].trackIndex != my_disk.currentTrackIndex){
					tseek(file->loc[i].trackIndex);
				}
				cmd = construct_fs3cmdblock(FS3_OP_RDSECT,file->loc[i].sectorIndex,0,0);
				if(network_fs3_syscall(cmd,&read,temp_buf)==-1){
					//Failed syscall
					return(-1);
				}

				//Checks if file read was succesful 
				deconstruct_fs3cmdblock(read,NULL,NULL,NULL,&returnVal);

				if(returnVal == 0){
					fs3_put_cache(file->loc[i].trackIndex,file->loc[i].sectorIndex,temp_buf);
				}
				else{
					//Failed read
					free(file_buf);
					free(temp_buf);
					file_buf = NULL;
					temp_buf = NULL;
					logMessage(FS3DriverLLevel, "FS3 DRVR: failed read on fh %d (%d bytes)",fd,count);
					return(-1);
					}
			}
			else{
				memcpy(temp_buf,cacheReturn,FS3_SECTOR_SIZE);
			}
				
			//Combines read of sector in filebuf
			if(totalBytesToRead > FS3_SECTOR_SIZE){
				memcpy(&file_buf[bytesRead], temp_buf, FS3_SECTOR_SIZE);
				totalBytesToRead -= FS3_SECTOR_SIZE;
				bytesRead += FS3_SECTOR_SIZE;
			}
			else{
				memcpy(&file_buf[bytesRead], temp_buf, totalBytesToRead);
			}
		}

		//Read needed section of filebuf into buf
		if((file->pos)+count>file->length){
			memcpy(buf, file_buf + (file->pos-(SECTOR_INDEX_NUMBER(file->pos)*FS3_SECTOR_SIZE)), (file->length-file->pos));
			file->pos = file->length;
		}
		else{
			memcpy(buf, file_buf + (file->pos-(SECTOR_INDEX_NUMBER(file->pos)*FS3_SECTOR_SIZE)), count);
			file->pos += count;
		}

		//Frees file buf
		free(file_buf);
		free(temp_buf);
		file_buf = NULL;
		temp_buf = NULL;

		logMessage(FS3DriverLLevel, "FS3 DRVR: read successful on fh %d (%d bytes)",fd,count);
		return(count);
	}
	else{
		//File handle not associated with file or file not open
		return (-1);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_write
// Description  : Writes "count" bytes to the file handle "fh" from the 
//                buffer  "buf"
//
// Inputs       : fd - filename of the file to write to
//                buf - pointer to buffer to write from
//                count - number of bytes to write
// Outputs      : bytes written if successful, -1 if failure

int32_t fs3_write(int16_t fd, void *buf, int32_t count) {
	FILE_INFO *file;
	FS3CmdBlk write;
	FS3CmdBlk cmd;
	uint8_t returnVal;
	char *temp_buf;
	char *write_buf;
	TRACK_SECTOR_PAIR tempPair;
	int32_t totalBytesWritten;
	int32_t bytesWritten;
	uint32_t originalPos;

	//Gets reference to file from file handle (returns NULL file handle not associated with file or file not open)
	file = get_file(fd);

	//Checks if file exsits and is open
	if(file != NULL){

		//Tracks bytes writtten in total and during current sector write
		totalBytesWritten = 0;
		bytesWritten = 0;
		
		//Reads all needed sectors of file for write
		while(totalBytesWritten<count){

			//Saves original pos of file before read of sector
			originalPos = file->pos;

			//Reads current sector to wite
			if(fs3_seek(fd,SECTOR_INDEX_NUMBER(file->pos)*FS3_SECTOR_SIZE)!=-1){

				temp_buf = malloc(FS3_SECTOR_SIZE);

				//If the sector is full reads the whole sector
				if(SECTOR_INDEX_NUMBER(originalPos) < (file->numOfSectors)-1){
					fs3_read(fd,temp_buf,FS3_SECTOR_SIZE);
				}

				//If the sector is partially full read part of the sector
				else if (SECTOR_INDEX_NUMBER(originalPos) == (file->numOfSectors)-1){
					fs3_read(fd,temp_buf,(file->length-((SECTOR_INDEX_NUMBER(originalPos))*FS3_SECTOR_SIZE)));
				}

				//If still need to write more bytes and out of sectors allocate new sector to write
				else{
					//Allocates new sector to file
					if(get_free_track_sector_pair(&tempPair) == 0){
						file->loc[file->numOfSectors] = tempPair;
						logMessage(FS3DriverLLevel, "FS3 driver: allocated fs3 track %d, sector %d for fh/index %d/%d"
							,file->loc[file->numOfSectors].trackIndex,file->loc[file->numOfSectors].sectorIndex,file->fileHandle, file->pos);
						file->numOfSectors++;
					}
				}

				//Seeks back to original pos to write
				fs3_seek(fd,originalPos);

				//Bytes to write
				write_buf = malloc(count-totalBytesWritten);
				memcpy(write_buf, buf + totalBytesWritten, count-totalBytesWritten);

				//Creates buf to write the amount of bytes that fits into sector
				if((count-totalBytesWritten)>(FS3_SECTOR_SIZE-(file->pos%FS3_SECTOR_SIZE))){
					memcpy(&temp_buf[(file->pos)%FS3_SECTOR_SIZE],write_buf,(FS3_SECTOR_SIZE-(file->pos%FS3_SECTOR_SIZE)));
					bytesWritten = FS3_SECTOR_SIZE-(file->pos%FS3_SECTOR_SIZE);
					totalBytesWritten += bytesWritten;
				}
				//If bytes fit in sector creates buf that write the rest of buf
				else{
					memcpy(&temp_buf[(file->pos)%FS3_SECTOR_SIZE],write_buf,(count-totalBytesWritten));
					bytesWritten = count-totalBytesWritten;
					totalBytesWritten += bytesWritten;
				}					

				//Free the write buf
				free(write_buf);
				write_buf = NULL;

				//Writes to sector of given file
				if(file->loc[SECTOR_INDEX_NUMBER(file->pos)].trackIndex != my_disk.currentTrackIndex){
					tseek(file->loc[SECTOR_INDEX_NUMBER(file->pos)].trackIndex);
				}
				cmd = construct_fs3cmdblock(FS3_OP_WRSECT,file->loc[SECTOR_INDEX_NUMBER(file->pos)].sectorIndex,0,0);
				if(network_fs3_syscall(cmd,&write,temp_buf)==-1){
					//Failed syscall
					return(-1);
				}

				//Puts new write into cache
				fs3_put_cache(file->loc[SECTOR_INDEX_NUMBER(file->pos)].trackIndex,file->loc[SECTOR_INDEX_NUMBER(file->pos)].sectorIndex, temp_buf);

				//Free the sector buf after write 
				free(temp_buf);
				temp_buf = NULL;

				//Checks if file write was succesful 
				deconstruct_fs3cmdblock(write,NULL,NULL,NULL,&returnVal);
				if (returnVal == 0){
					//Updates file info after write
					file->pos = (file->pos)+bytesWritten;
					if(file->pos > file->length)
						file->length = file->pos;
					
				}
				else{
					//Failed write
					logMessage(FS3DriverLLevel, "FS3 DRVR: failed write on fh %d (%d bytes)",fd,count);
					return(-1);
				}
			}
			else{
				//Failed seek
				return(-1);
			}
		}
		//Returns bytes written
		logMessage(FS3DriverLLevel, "FS3 DRVR: write on fh %d (%d bytes) [pos=%d, len=%d]",fd,count,file->pos,file->length);
		return(totalBytesWritten);
	}
	else{
		//File handle not associated with file or file not open
		return (-1);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : fs3_seek
// Description  : Seek to specific point in the file
//
// Inputs       : fd - filename of the file to write to
//                loc - offfset of file in relation to beginning of file
// Outputs      : 0 if successful, -1 if failure

int32_t fs3_seek(int16_t fd, uint32_t loc) {
	FILE_INFO *file;

	//Gets reference to file from file handle (returns NULL file handle not associated with file or file not open)
	file = get_file(fd);

	//Checks if file exsits and is open
	if(file != NULL){
		//Checks if loc is valid in file
		if(file->length>=loc){
			//Set file pos to loc 
			file->pos = loc;
			logMessage(FS3DriverLLevel, "File seek fh %d to %d/%d.",fd,file->pos,file->length);
			return(0);
		}
		else{
			//Loc out of range
			logMessage(FS3DriverLLevel, "Failed file seek fh %d to %d/%d.",fd,loc,file->length);
			return(-1);
		}
	}
	else{
		//File handle not associated with file or file not open
		return (0);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : construct_fs3cmdblock
// Description  : Create an FS3 cmdblock from the variable feilds
//
// Inputs       : op - opcode
//                sec - sector number
//				  trk - track number
//				  ret - return value
// Outputs      : contructed fs3 cmdblock

FS3CmdBlk construct_fs3cmdblock(uint8_t op, uint16_t sec, uint_fast32_t trk, uint8_t ret){
	FS3CmdBlk cmdblock;
	//Places register to corisponding bits in cmdblock
	cmdblock = 0;
	cmdblock = ((uint64_t)op<<60); 
	cmdblock = cmdblock | ((uint64_t)sec<<44);
	cmdblock = cmdblock | ((uint64_t)trk<<12);
	cmdblock = cmdblock | ((uint64_t)ret<<11);
	return cmdblock;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : deconstruct_fs3cmdblock
// Description  : Extracts register state from cmdblock
//
// Inputs       : cmdblock - cmdblock to extract registers from
//				  op - opcode address
//                sec - sector number address
//				  trk - track number address
//				  ret - return value address
// Outputs      : 0 if successful, -1 if failure

int deconstruct_fs3cmdblock(FS3CmdBlk cmdblock, uint8_t *op, uint16_t *sec, uint_fast32_t *trk, uint8_t *ret){
	//Isolates needed values at front of cmdblock and gets n bits dependent on register if pointer is not NULL
	if(op != NULL)
		*op = ((((uint64_t)1 << 4)-1)&(cmdblock>>60));
	if(sec != NULL)
		*sec = ((((uint64_t)1 << 16)-1)&(cmdblock>>44));
	if(trk != NULL)
		*trk = ((((uint64_t)1 << 32)-1)&(cmdblock>>12));
	if(ret != NULL)
		*ret = ((((uint64_t)1 << 1)-1)&(cmdblock>>11));
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_file
// Description  : Gets file associated with file handle if vailid file handle
//
// Inputs       : fd - file handle
// Outputs      : File if valid fd, NULL if not valid or file not open

FILE_INFO * get_file(int16_t fd){

	//Checks if file handle is associated with open file
	if(my_disk.files[fd].fileHandle == fd){
		if(my_disk.files[fd].open){
			//Return pointer to file
			return(&(my_disk.files[fd]));
		}
		else{
			//File not open
			logMessage(FS3DriverLLevel, "File not open: file handle(%d)",fd);
			return(NULL);
		}
	}

	//File handle isnt accociated with a file
	logMessage(FS3DriverLLevel, "Invalid file handle: %d",fd);
	return(NULL);
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : tseek
// Description  : Seeks track to given track
//
// Inputs       : trackToSeek - track index to seek to 
// Outputs      : 0 if successful, -1 if failure

int32_t tseek(FS3TrackIndex trackToSeek){
	FS3CmdBlk tseek;
	FS3CmdBlk cmd;
	uint8_t returnVal;

	//Seeks track to given trackToSeek
	cmd = construct_fs3cmdblock(FS3_OP_TSEEK,0,trackToSeek,0);
	if(network_fs3_syscall(cmd,&tseek,NULL)==-1){
		//Failed syscall
		return(-1);
	}
	
	deconstruct_fs3cmdblock(tseek,NULL,NULL,NULL,&returnVal);
	if(returnVal == 0){
		//Successful track seek
		logMessage(FS3DriverLLevel, "Track seeked to %d",trackToSeek);
		my_disk.currentTrackIndex = trackToSeek;
		return(0);
	}
	else {
		//Failed track seek
		logMessage(FS3DriverLLevel, "Failed track seek to %d",trackToSeek);
		return(-1);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_free_track_sector_pair
// Description  : Finds a free track and sector pair for a file
//
// Outputs      : 0 if successful, -1 if failure

int32_t get_free_track_sector_pair(TRACK_SECTOR_PAIR *pair){

	//Sets next open sector on disk
	pair->trackIndex = my_disk.nextTrack;
	pair->sectorIndex = my_disk.nextSector;

	//Increments next open sector on disk 
	my_disk.nextSector++;

	//If out of sectors on track go to next track
	if(my_disk.nextSector >= FS3_TRACK_SIZE){
		my_disk.nextSector = 0;
		my_disk.nextTrack++;
	}

	return(0);
}