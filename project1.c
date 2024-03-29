#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
//used for parsing file names
#include <string.h>

//allocate 1MB of memory
#define STORAGE_LEN 1048576
unsigned char MEMORY[STORAGE_LEN] = {0}; // each array index represents a byte
//2k blocks
#define BLOCK_SIZE_BYTES 2048

//variables store the memory index of the value
struct VolumeControlBlock{
	int BLOCK_COUNT;
	int BLOCK_SIZE;
	int FREE_BLOCKS;
	int BITMAP_START;
};

//set variables to indicate data structure locations
//first data block (0-2047) is used as the volume control block
//4 bytes for an unsigned int for number of blocks - 512 [0]
//4 bytes for an unsigned int for the size of blocks - 2048 Bytes (array spaces) [4]
//4 bytes for an unsigned int for the free block count - 507 (first 5 blocks are meta) [8]
//64 bytes for the bit map because 512 blocks exist (512 - 1vcb - 4dir) = 64 [12]
const struct VolumeControlBlock VOLUME_CONTROL_BLOCK = {0, 4, 8, 12};

//number of bits in the bitmap is declared as a constant
const unsigned int BITMAP_NUM_BITS = 512;


//second-ninth data block (2048 - 16352) is reserved for the directory
	
	//first 32 bytes (2048 - 2079) are used to store head and tail for linked list as short ints
const unsigned int DIR_HEAD = 2076;
const unsigned int DIR_TAIL = 2078;
	 
	//Each entry includes filename, start block number, and file size (32 bytes)
	//8 blocks are needed to store up to 5 file entries (1 block files)
	
	//name is a series of 22 unsigned char including \0 at the end (1 byte ec.) and a 4 byte extension = 26 bytes
	// 2 bytes for size (short int)
	// 2 bytes for location (short int)	
	// 2 bytes for memory location of next entry in linked list (short int)
const unsigned int DIR_START = 2080;
const unsigned int DIR_SIZE = 16352;
const unsigned int DIR_ENTRY_LEN = 32;
const unsigned int DIR_ENTRY_NAME = 0;
const unsigned int DIR_ENTRY_EXT = 22;
const unsigned int DIR_ENTRY_SIZE = 26;
const unsigned int DIR_ENTRY_LOC = 28;
const unsigned int DIR_ENTRY_NEXT = 30;

//userful constants
#define MAX_NAME_LEN 22
#define MAX_EXT_LEN 4

//File control structures
struct FileControlBlock{
	int fileSizeBlocks;
	int firstBlockIndex;
};

void CopyFCB(struct FileControlBlock *dest, struct FileControlBlock *src){
	dest->fileSizeBlocks = src->fileSizeBlocks;
	dest->firstBlockIndex = src->firstBlockIndex;
}

#define MAX_GLOBAL_FILES 64
#define MAX_LOCAL_FILES 8

typedef struct {
	char fname[MAX_NAME_LEN + MAX_EXT_LEN];
	struct FileControlBlock fcb;
	short int instances;
}GlobalTableEntry;


GlobalTableEntry GLOBAL_FILE_TABLE[MAX_GLOBAL_FILES];

//ads an entry to the global file table and returns the index of the entry or -1 in case of failure
int AppendToGlobalTable(GlobalTableEntry gte){
	//TODO: ensure no duplicate entries
	for(int i = 0; i < MAX_GLOBAL_FILES; i++){
		//check if the space is available
		GlobalTableEntry *entry = &GLOBAL_FILE_TABLE[i];
		if(entry->fname[0] == '\0'){
			strcpy(entry->fname, gte.fname);
			CopyFCB(&(entry->fcb), &gte.fcb);
			entry->instances = gte.instances;
			printf("Added %s to GFT at location %d\n", gte.fname, i);//debug
			return i;
		}
	}
	printf("could not append %s, no space available in global file table\n", gte.fname);//debug
	return -1;
}

//returns the index of a file's position in the global file table or -1 if not found
int FindInGlobalTable(char *fname){
	for(int i = 0; i < MAX_GLOBAL_FILES; i++){
		GlobalTableEntry *entry = &GLOBAL_FILE_TABLE[i];
		if(strcmp(entry->fname, fname) == 0){
			//if the entry is found set its values to the default
			printf("Found entry at GFT[%d]\n", i);//debug
			return i;
		}
	}
	//if the loop is completed the entry was not found
	printf("file %s was not found in global table\n", fname);//debug
	return -1;
}

int RemoveFromGlobalTable(char *fname){
	for(int i = 0; i < MAX_GLOBAL_FILES; i++){
		GlobalTableEntry *entry = &GLOBAL_FILE_TABLE[i];
		if(strcmp(entry->fname, fname) == 0){
			//TODO: check for instances before removing
			if(entry->instances > 0){
				printf("Could not remove %s from GFT, file is open\n", fname);
				return -1;
			}
			//if the entry is found set its values to the default
			strcpy(entry->fname, "\0");
			entry->fcb.fileSizeBlocks = 0;
			entry->fcb.firstBlockIndex = 0;
			entry->instances = 0;
			printf("Removed %s from GFT[%d]\n", fname, i);//debug
			return 0;
		}
	}
	//if the loop is completed the entry was not found
	printf("%s successfully removed from GFT\n", fname);
	return -1;
}

struct LocalTableEntry{
	char fname[MAX_NAME_LEN + MAX_EXT_LEN];
	short int handle; //index in Global table
};

int AppendToLocalTable(struct LocalTableEntry *table, struct LocalTableEntry lte){
	//TODO: ensure no duplicate
	for(int i = 0; i < MAX_LOCAL_FILES; i++){
		struct LocalTableEntry *entry = &table[i];
		if(entry -> fname[0] == '\0'){
			printf("Inserting at local table [%d]\n", i);//debug
			strcpy(entry->fname, lte.fname);
			entry->handle = lte.handle;
			return i;
		}
	}
	printf("failed to insert %s in LFT, no space available\n", lte.fname);//debug
	return -1;
}

int FindInLocalTable(struct LocalTableEntry *table, char *fname){
	for(int i = 0; i < MAX_LOCAL_FILES; i++){
		struct LocalTableEntry *entry = &table[i];
		if(strcmp(entry->fname, fname) == 0){
			printf("Found entry at LFT[%d]\n", i);//debug
			return i;
		}
	}
	printf("File %s was not found in local table\n", fname);//debug
	return -1;
}

int RemoveFromLocalTable(struct LocalTableEntry *table, char *fname){
	for(int i = 0; i < MAX_LOCAL_FILES; i++){
		struct LocalTableEntry *entry = &table[i];
		if(strcmp(entry->fname, fname) == 0){
			printf("Removed %s from LFT[%d]\n", fname, i);//debug
			strcpy(entry->fname, "\0");
			entry->handle = 0;
			return 0;
		}
	}
	printf("File %s was not found in local table. Unable to remove\n", fname);//debug
	return -1;
}

unsigned int ReadUInt(int index){
	//ensure index in memory is a valid unsigned int location
	
	if(index > 12 || (index % 4) > 0){
		printf("Invalid unsigned integer read:%d\n", index);
		return 0;
	}
	
	unsigned int value = 0;
	
	//move to specified location in memory
	unsigned char *arr = MEMORY + index;
	
	//read 4 bytes at that location into value
	for(int i = 0; i < 4; i++){
		value = (value << 8) | *arr++;
	}
	
	return value;
}

void WriteUInt(int index, unsigned int value){
	//ensure index in memory is a valid unsigned int location
	if(index > 12 || (index % 4) > 0){
		printf("Invalid unsigned integer write:%d\n", index);
		return;
	}
	
	//write 4 bytes of value
	for(int i=0; i<4; i++){
		MEMORY[i + index] = (value >> ((3-i)*8)) & 0xFF;
	}
}

short int ReadSInt(int index){
	//ensure index in memory is a valid short int location
	if( index < DIR_HEAD || (index % DIR_ENTRY_LEN) < DIR_ENTRY_SIZE || (index % 2) > 0){
		printf("Invalid short integer read: %d\n",index);
		return 0;
	}

	short int value = 0;
	unsigned char *arr = MEMORY + index;
	for(int i = 0; i < 2; i++){
		value = (value << 8) | *arr++;
	}
	
	return value;
}

void WriteSInt(int index, short int value){
	//ensure index in memory is a valid short int location
	if( index < DIR_HEAD || (index % DIR_ENTRY_LEN) < DIR_ENTRY_SIZE || (index % 2) > 0){
		printf("Invalid short integer write: %d\n",index);
		return;
	}
	for(int i=0; i<2; i++){
		MEMORY[i + index] = (value >> ((1-i)*8)) & 0xFF;
	}
}


//helper function to read bitmap
short int ReadBit(int bitIndex){
	//get the correct byte
	unsigned char byte = MEMORY[VOLUME_CONTROL_BLOCK.BITMAP_START + bitIndex/8];
	//check the bit
	if((byte & (1 << (7-(bitIndex%8)))) == 0){
		return 0;
	}else{
		return 1;
	}
}

//helper function to write to bitmap
void WriteBit(int bitIndex, short int value){
	short int current = ReadBit(bitIndex);
	if(current != value){
		//get the relevant byte
		unsigned char byte = MEMORY[VOLUME_CONTROL_BLOCK.BITMAP_START + bitIndex/8];
		//flip the bit
		byte = byte ^ (1 << (7-(bitIndex%8)));
		//write to the array
		MEMORY[VOLUME_CONTROL_BLOCK.BITMAP_START + bitIndex/8] = byte;
	}
}

//function to find first space in the bitmap
//input size of space desired
//bitmap size is 512 for this example (512*2k blocks = 1M)
int FreeSpaceAddress(int spaceSize){
	int length = 0;
	int index = -1;
	for(int i = 0; i<BITMAP_NUM_BITS; i++){
		short int currentBit = ReadBit(i);
		//if the bit shows an open space
		if(currentBit == 0){
			//increase the tracked space length
			length++;
			//if this is the first space in a section save the index
			if(index < 0){
				index = i;
			}
			//if the section size is the desired size return the index
			if(length >= spaceSize){
				return index;
			}
		}else{
			//if the bit shows a closed space
			//reset the counter and length
			length = 0;
			index = -1;
		}
	}
	//if no space is found return(-1)
	printf("Could not find space of size %d in bitmap\n", spaceSize);
	return -1;
}

//find the first open space for a new entry in the directory
//returns the index of the start of the space for the entry or -1 to indicate full directory
short int FindDirSpace(){
	for(short int i = DIR_START; i<(DIR_START + DIR_SIZE); i +=DIR_ENTRY_LEN ){
		if(MEMORY[i] == 0x00){
			//printf("Found space in directory at: %d\n", i);//debug
			return i;
		}
	}
	//if no space is available return -1
	printf("No directory space available\n");
	return -1;
}

//validates the file name and outputs the name and extension to the corresponding char arrays
//return 0 if successful and -1 if error
int ParseFileName(const char *str, char *name, char *extension){
	char *dot = strchr(str, '.');
	int length = strlen(str);
	//printf("Parsing File: %s\n", str);//debug
	//ensure extension is given
	if(dot == NULL){
		printf("Invalid file name:%s, File name must include extension\n", str);
		return -1;
	}
	
	//validate input
	//check for special characters
	const char *illegalChars = "/\\:*?\"|";
	for(int i = 0; i < length; i++){
		if(strchr(illegalChars, str[i]) != NULL) {
			printf("Invalid file name:%s, File name cannot contain /\\:*?\"|",str);
			return -1;
		}
	}
	
	//check for multiple dots
	if(strchr(dot+1, '.') != NULL){
		printf("Invalid file name:%s A file may have only 1 extension. Include a single \".\" character",str);
		return -1;
	}
	
	//validate lengths
	if( strlen(dot + 1) > MAX_EXT_LEN){
		printf("Invalid file name:%s Extension is too long. Three characters max\n", str);
		return -1;
	}
	
	if((dot - str) >= MAX_NAME_LEN){
		printf("File name:\"%s\" is too long. 21 characters max\n", str);
		return -1;
	}
	//copy file name into the name location
	strncpy(name, str, (dot - str));
	name[dot - str] = '\0';
	
	//copy the extension to the extension location
	strcpy(extension, (dot + 1));
	//extension[MAX_EXT_LEN] = '\0';
	return 0;
} 

//Add an entry to the file directory
int AddDirEntry(const char *fname, int fsize, int flocation){
	short int entryLoc = FindDirSpace();
	if(entryLoc < 0){
		printf("Directory full. Could not add %s\n", fname);
		return -1;
	}
	
	//TODO: ensure duplicate file doesn't exist
	
	//input the file details at the entry location

	//parsing name and extension from fname
	char name[MAX_NAME_LEN]; //= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	char ext[MAX_EXT_LEN] = {0x00, 0x00, 0x00, 0x00};
	
	int valid = ParseFileName(fname, name, ext);

	if(valid < 0){
		printf("Could not add directory entry: %s is invalid file name.\n", fname);
		return -1;
	}
		
	//write name
	//printf("Writing name: %s\nto location: %d\n", name, (entryLoc + DIR_ENTRY_NAME));//debug
	for(int i = 0; i < MAX_NAME_LEN; i++){
		MEMORY[entryLoc + DIR_ENTRY_NAME + i] = (unsigned char)name[i];
	}
	//write extension
	//printf("Writing ext: %s\nto location: %d\n", ext, (entryLoc + DIR_ENTRY_EXT));//debug
	for(int i = 0; i < MAX_EXT_LEN; i++){
		MEMORY[entryLoc + DIR_ENTRY_EXT + i] = (unsigned char)ext[i];
	} 
	//write size in blocks
	//printf("Writing Sints for adddir in entry: %d",entryLoc);//debug
	WriteSInt((entryLoc + DIR_ENTRY_SIZE),fsize); 
	//write storage location
	WriteSInt((entryLoc + DIR_ENTRY_LOC), flocation);
	//write next (always 0 for the latest entry)
	WriteSInt((entryLoc + DIR_ENTRY_NEXT), 0);
	
	//if this is the first entry initialize head and tail
	if(ReadSInt(DIR_HEAD) == 0){
		WriteSInt(DIR_HEAD, entryLoc);
		WriteSInt(DIR_TAIL, entryLoc);
	}else{
		//otherwise
		//update the previous tail to point to the new entry
		WriteSInt(ReadSInt(DIR_TAIL) + DIR_ENTRY_NEXT, entryLoc);
		//update the tail to be the new entry
		WriteSInt(DIR_TAIL, entryLoc);
	}
	
	return 0;
}
//helper function for printing a directory entry
void GetDirEntry(int entryLoc, unsigned char *name, unsigned char *ext, short int *size, short int *loc, short int *next){
	for(int i = 0; i < MAX_NAME_LEN; i++){
		name[i] = MEMORY[entryLoc + DIR_ENTRY_NAME + i];
	}
	//name[MAX_NAME_LEN] = '\0';
	for(int i = 0; i < MAX_EXT_LEN; i++){
		ext[i] = MEMORY[entryLoc + DIR_ENTRY_EXT + i];
	}
	//ext[MAX_EXT_LEN] = '\0';
	//printf("Reading sints for gde at %d\n", entryLoc);//debug
	*size = ReadSInt(entryLoc + DIR_ENTRY_SIZE);
	*loc = ReadSInt(entryLoc + DIR_ENTRY_LOC);
	*next = ReadSInt(entryLoc + DIR_ENTRY_NEXT);
	return;
	
}

//prints a directory entry at a given location. Returns the location of the next entry or 0 if last entry
short int PrintDirEntry(int entryLoc){
	unsigned char name[MAX_NAME_LEN];
	unsigned char ext[MAX_EXT_LEN];
	short int size;
	short int loc;
	short int next;
	GetDirEntry(entryLoc, name, ext, &size, &loc, &next);	
	printf("%-15s %-15s %-15hd %-15hd %-15hd\n", name, ext, size, loc, next);
	return next;
}

//returns the memory location of a files directory entry based on name or -1 if it is not found/invalid name
int FindDirEntry(char *fname){
	//parse and validate fname
	char name[MAX_NAME_LEN]; //= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	char ext[MAX_EXT_LEN] = {0x00, 0x00, 0x00, 0x00};
	int valid = ParseFileName(fname, name, ext);
	if(valid < 0){
		printf("File name:%s is not valid, no dir entry\n", fname);
		return -1;
	}
	//printf("name: %s, ext: %s\n", name, ext);//debug
	//iterate directory linked list to find the file
	int entry = ReadSInt(DIR_HEAD);
	while(entry != 0){
		unsigned char entryName[MAX_NAME_LEN];
		unsigned char entryExt[MAX_EXT_LEN];
		short int size;
		short int loc;
		short int next;
		GetDirEntry(entry, entryName, entryExt, &size, &loc, &next);
			
		if(strcmp(entryName, name) == 0 && strcmp(entryExt, ext) == 0){
			//if the entry matches the search criteria
			printf("Found file in directory at location: %d\n", entry);
			return entry;
		}
		entry = next;
	}
	//if file is not found return -1
	printf("%s not found in directory\n", fname);
	return -1;
}

//removes a file's entry from the directory. Returns 0 on success and -1 on fail
int RemoveDirEntry(char *fname){
	printf("removing %s from directory...\n", fname);//debug
	//parse and validate fname
	char name[MAX_NAME_LEN]; //= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	char ext[MAX_EXT_LEN] = {0x00, 0x00, 0x00, 0x00};
	int valid = ParseFileName(fname, name, ext);
	if(valid < 0){
		printf("File name: %s is not valid, couldnt remove from directory\n", fname);
		return -1;
	}
	int entry = ReadSInt(DIR_HEAD);
	int prev = ReadSInt(DIR_HEAD);
	while(entry != 0){
		unsigned char entryName[MAX_NAME_LEN];
		unsigned char entryExt[MAX_EXT_LEN];
		short int size;
		short int loc;
		short int next;
		GetDirEntry(entry, entryName, entryExt, &size, &loc, &next);
				
		if(strcmp(entryName, name) == 0 && strcmp(entryExt, ext) == 0){
			//if the entry matches the search criteria
			break;
		}
		prev = entry;
		entry = next;
	}
	if(entry == 0){
		printf("Could not remove %s from dir, does not exist\n", fname);
		return -1;
	}else{
		//if we are removing the head entry make the next entry the head
		short int headLoc = ReadSInt(DIR_HEAD);
		if(headLoc == entry){
			WriteSInt(DIR_HEAD, ReadSInt(headLoc + DIR_ENTRY_NEXT));
			//if the head is 0 then the last entry is removed and tail must also be updated to 0
			if(ReadSInt(DIR_HEAD) == 0){
				WriteSInt(DIR_TAIL, 0);
			}
		}else{
			//the entry being removed is not the head
			//make the previous entry's next pointer point to the entry previously pointed to by the removed
			WriteSInt(prev + DIR_ENTRY_NEXT, ReadSInt(entry + DIR_ENTRY_NEXT));
			//if the tail was removed set the tail to prev
			if(ReadSInt(DIR_TAIL) == entry){
				WriteSInt(DIR_TAIL, prev);
			}
		}
		//null the entry data
		for(int i = 0; i < DIR_ENTRY_LEN; i++){
			MEMORY[entry + i] = 0x00;
		}
		
		//successful removal
		printf("%s successfully removed from directory!\n", fname);//debug
		return 0;
	}
}

void PrintDir(){
	//TODO: remove next for production
	//get VCB information
	int free = ReadUInt(VOLUME_CONTROL_BLOCK.FREE_BLOCKS);
	int total = ReadUInt(VOLUME_CONTROL_BLOCK.BLOCK_COUNT);
	printf("%d of %d Blocks are free:\n", free, total);
	
	printf("%-15s %-15s %-15s %-15s %-15s\n", "Name", "Extension", "Size", "Start Address", "Next");
	printf("---------------------------------------------------------------------\n");//perfect size
	int entry = ReadSInt(DIR_HEAD);
	while(entry != 0){
		unsigned char entryName[MAX_NAME_LEN];
		unsigned char entryExt[MAX_EXT_LEN];
		short int size;
		short int loc;
		short int next;
		GetDirEntry(entry, entryName, entryExt, &size, &loc, &next);
		
		printf("%-15s %-15s %-15d %-15d %-15d\n", entryName, entryExt, size, loc, next);	
		
		entry = next;
	}
}


//updates global and local process tables accordingly, returns index of file in global table
//returns -1 in event of error
int Open(char *fname, struct LocalTableEntry *localOpenFiles){
	//search local process table for instance, if found -> error
	for(int i = 0; i < MAX_LOCAL_FILES; i++){
		if(localOpenFiles[i].fname == fname){
			printf("Cannot open a file that is already open (%s)\n", fname);
			return -1;
		}
	}
	
	struct FileControlBlock fcb;
	int handle = -1;

	//Search the GFT for an instance of the file
	handle = FindInGlobalTable(fname);
	if(handle > 0){
		//get fcb from global table
		CopyFCB(&fcb, &GLOBAL_FILE_TABLE[handle].fcb);
		//increment instances in GFT
		GLOBAL_FILE_TABLE[handle].instances++;
	}else{
		//else create a new FCB for the file based on directory info
		int dirEntry = FindDirEntry(fname);
		//TODO: case for not found
		//printf("reading open at %d\n", dirEntry); //DEBGU
		fcb.fileSizeBlocks = ReadSInt(dirEntry + DIR_ENTRY_SIZE);
		fcb.firstBlockIndex = ReadSInt(dirEntry + DIR_ENTRY_LOC);
		//add the entry to the GFT and get the index
		GlobalTableEntry globalEntry;
		strcpy(globalEntry.fname, fname);
		CopyFCB(&globalEntry.fcb, &fcb);
		globalEntry.instances = 1;
		handle = AppendToGlobalTable(globalEntry);
		//check for failed append
		if(handle < 0){
			printf("Failed to append %s to global table, open aborted\n", fname);
			return -1;
		}
	}
	//at this point handle is the correct index and the global table has been successfully updated
	
	//update the local process table with a new entry
	struct LocalTableEntry lte;
	strcpy(lte.fname, fname);
	lte.handle = handle;
	AppendToLocalTable(localOpenFiles, lte);
	
	//return the handle
	return handle;
}

//pointer to a FileControlBlock struct for output, size of the file in blocks, file name
int Create(char *name, short int size, struct LocalTableEntry *localOpenFiles){
	//ensure name is not taken
	if(FindDirEntry(name) > 0){
		printf("Could not Create File. Another file with that namne already exists\n");
		return -1;
	}	
	//ensure the file size is at least less than the free block count
	int freeBlocks = ReadUInt(VOLUME_CONTROL_BLOCK.FREE_BLOCKS);
	if(size > freeBlocks){
		printf("Could not Create File. Desired File Size of %d is too big!\n", size);
		return -1;
	}else{
		//search the bitmap for a free space of corresponding size
		int blockIndex = FreeSpaceAddress(size);
		printf("blockIndex is %d\n", blockIndex);//shows the index of the first space found
		if(blockIndex < 0){
			//if no space exists error (for now)
			printf("There is not a large enough space available for that file\n");
			//TODO: maybe apply compaction...
			return -1;
		}else{
			//create a directory entry with fname, start block, size
			int addRes = AddDirEntry(name, size, blockIndex);
			if(addRes < 0){
				printf("Failed to add %s to directory, could not create\n", name);
				return -1;
			}
			//if space exists mark it as used
			for(int i = blockIndex; i < (size + blockIndex); i++){
				WriteBit(i, 1);
			}
			//update vcb free block count
			WriteUInt(VOLUME_CONTROL_BLOCK.FREE_BLOCKS, (freeBlocks - size));
			//run the open function to give the fcb values
			int handle = Open(name, localOpenFiles);
			return handle;
		}
			
	}
}

int Read(int hand, unsigned char *output){
	//search gft for handle based on name
		//if not found err 
	
	GlobalTableEntry entry = GLOBAL_FILE_TABLE[hand];
	if(entry.fname[0] == '\0' || entry.fcb.firstBlockIndex == 0){
		printf("Could not find file to read, bad handle\n");
		return -1;
	}
	
	//get fcb from gft
	int start = entry.fcb.firstBlockIndex * BLOCK_SIZE_BYTES;
	int size = entry.fcb.fileSizeBlocks * BLOCK_SIZE_BYTES;
	
	//strncpy into a sized array at file location and size
	unsigned char buffer[size]; //block size in bytes
	
	strncpy(buffer, &MEMORY[start], size);
	
	//add null terminator if not found
	if(strchr(buffer, '\0') < 0){
		buffer[size - 1] = '\0';
	}
	//return the array
	strcpy(output, buffer);
	return 0;
}

int Write(int hand, char *input){
	//search gft and check name
	
	//get fcb
	
	GlobalTableEntry entry = GLOBAL_FILE_TABLE[hand];
	if(entry.fname[0] == '\0' || entry.fcb.firstBlockIndex == 0){
		printf("Could not find file to write, bad handle\n");
		return -1;
	}
	
	//get fcb from gft
	int start = entry.fcb.firstBlockIndex * BLOCK_SIZE_BYTES;
	int size = entry.fcb.fileSizeBlocks * BLOCK_SIZE_BYTES;
	
	//strncpy into a sized array at file location and size
	unsigned char buffer[size]; //block size in bytes
	
	//check input size
	if(strlen(input) > size){
		printf("input string larger than file size, cannot write\n");
		return -1;
	}
	//move input into buffer to manage null term
	strcpy(buffer, input);
	if(strchr(buffer, '\0') < 0){
		buffer[size - 1] = '\0';
	}

	//strncpy into appropriate space
	strcpy(&MEMORY[start], buffer);	
	//return 0
	return 0;
}

int Close(int handle, struct LocalTableEntry *localOpenFiles){
	//search gft for entry
		//if not found error
	//if found get the name
	GlobalTableEntry *entry = &GLOBAL_FILE_TABLE[handle];
	if(entry->fname[0] == '\0' || entry->fcb.firstBlockIndex == 0){
		printf("Could not find file to close, bad handle\n");
		return -1;
	}
	
	//get fcb from gft
	char *name = entry->fname;
	
	//remove from local file table
	int res = RemoveFromLocalTable(localOpenFiles, name);
	if(res < 0){
		printf("error removing file from local table\n");
		return -1;
	}	
	//decrement global table instances
	entry->instances--;
	//if instances is 0 remove from global table
	if(entry->instances < 1){
		RemoveFromGlobalTable(name);
	} 
}

int Delete(char *fname){
	//check global table for an instance
	int exists = FindInGlobalTable(fname);
	if(exists >= 0){
		printf("%s is open in some process, cannot be deleted\n", fname);
		return -1;
	}
	//get file data from the directory
	int entryLoc = FindDirEntry(fname);
	unsigned char name[MAX_NAME_LEN];
	unsigned char ext[MAX_EXT_LEN];
	short int size;
	short int loc;
	short int next;
	GetDirEntry(entryLoc, name, ext, &size, &loc, &next);
	
	//Remove file from directory
	int removeRes = RemoveDirEntry(fname);
		
	if(removeRes < 0){
		printf("failed to remove %s from directory (res: %d), delete aborted\n", fname, removeRes);
		return -1;
	}
	
	//clear the actural data from the file
	unsigned char buffer[size*BLOCK_SIZE_BYTES];
	for(int i = 0; i < size*BLOCK_SIZE_BYTES; i++){
		buffer[i] = 0x00;
	}
	strcpy(&MEMORY[loc * BLOCK_SIZE_BYTES], buffer);
	
	//update the vcb's bitmap and free block count
	int current = ReadUInt(VOLUME_CONTROL_BLOCK.FREE_BLOCKS);
	WriteUInt(VOLUME_CONTROL_BLOCK.FREE_BLOCKS, (current + size));
	
	for(int i = loc; i < (size + loc); i++){
		WriteBit(i, 0);
	}
			
	
		
	return 0;
}


int main() {
	
	//initialize default values:
	WriteUInt(VOLUME_CONTROL_BLOCK.BLOCK_COUNT, 512);
	WriteUInt(VOLUME_CONTROL_BLOCK.BLOCK_SIZE, BLOCK_SIZE_BYTES);
	WriteUInt(VOLUME_CONTROL_BLOCK.FREE_BLOCKS, 503);
	//Write the first 9 bits of the bitmap as in use
	for(int i =0; i<9; i++){
		WriteBit(i, 1);
	}

	unsigned int num = ReadUInt(VOLUME_CONTROL_BLOCK.BLOCK_COUNT);	
	unsigned int size = ReadUInt(VOLUME_CONTROL_BLOCK.BLOCK_SIZE);	
	unsigned int free = ReadUInt(VOLUME_CONTROL_BLOCK.FREE_BLOCKS);	
	
	printf("Memory Initialized! \n%u blocks were created of size %u bytes with %u blocks free.\n\n", num, size, free);
	
	struct LocalTableEntry localTable[MAX_LOCAL_FILES] = {0};
	
	//shows bitmap
	void Bitmap(){
		for(int i = 0; i < 64 ; i++){
			if(i % 8 == 0){
				printf("\n");
			} 
			printf("%u",ReadBit(i));
		}
		printf("\n");
	}
	
	//testing Create Function
	int x = Create("world.txt", 20, localTable);
	int y = Create("p.c", 5, localTable);
	int za = Create("niqndoewidnwodnwoiedjwoid.cs", 1, localTable);
	
	PrintDir();
	
	//Testing Open Function
	printf("\n\nOpen res:\nGFT0: %s, %d, %d, %d (expect world.txt, 20, 9, 1)\n LFT: %s, %d (expect world.txt,0)\n\n", GLOBAL_FILE_TABLE[x].fname, GLOBAL_FILE_TABLE[x].fcb.fileSizeBlocks, GLOBAL_FILE_TABLE[x].fcb.firstBlockIndex, GLOBAL_FILE_TABLE[x].instances, localTable[0].fname, localTable[0].handle);

		printf("\n\nOpen res2:\nGFT1: %s, %d, %d, %d (expect prog.c, 5, 29, 1)\n LFT: %s, %d (expect prog.c,1)\n\n", GLOBAL_FILE_TABLE[y].fname, GLOBAL_FILE_TABLE[y].fcb.fileSizeBlocks, GLOBAL_FILE_TABLE[y].fcb.firstBlockIndex, GLOBAL_FILE_TABLE[y].instances, localTable[1].fname, localTable[1].handle);


	printf("File handles recieved: %d and %d\n", x, y);
	
	//Testing read write
	unsigned char buffer[20 * BLOCK_SIZE_BYTES];
	Read(x, buffer);
	printf("X initial read:%s\n", buffer); 
	unsigned char buffls[] = "Hello World!";
	unsigned char ytest[] = "Whether we wanted it or not weve stepped into war with the cabal on mars!\nSo lets get to taking out their command one by one...\n\n";
	Write(x, buffls);
	Write(y, ytest);
	unsigned char yout[5 * BLOCK_SIZE_BYTES];
	Read(x, buffer);
	printf("X final read: %s\n", buffer);
	Read(y, yout);
	printf("Y final read: %s\n", yout);
	
	int closeRes = Close(x, localTable);
	printf("close res: %d\n", closeRes);
	
	printf("\n\nClose res:\nGFT0: %s, %d, %d, %d (expect , 0, 0, 0)\n LFT: %s, %d (expect ,0)\n\n", GLOBAL_FILE_TABLE[x].fname, GLOBAL_FILE_TABLE[x].fcb.fileSizeBlocks, GLOBAL_FILE_TABLE[x].fcb.firstBlockIndex, GLOBAL_FILE_TABLE[x].instances, localTable[0].fname, localTable[0].handle);
	
	Delete("world.txt");
	Read(x, buffer);
	printf("X deleted read: %s\n", buffer);
	Write(x, "hello world!");
	
	PrintDir();
	
	printf("\n");
	return 0;
}

/*//testing type punning
	unsigned char array[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};
	unsigned int value = 0;
	
	//testing array input
	array[6] = 1;
	array[7] = 255;
	
	for (int i = 0; i<4; i++) {
		value = (value << 8) | array[i];
	}
	
	printf("Integer: %u\n", value);
	
	for (int i = 4; i<8; i++) {
		value = (value << 8) | array[i];
	}
	
	printf("Integer2: %u\n", value);*/
	
//testing read func
	/*unsigned char array[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
	unsigned int value = 0;
	value = ReadUIntFromArray(array, 0);
	printf("Value: %u\n", value);
	value = ReadUIntFromArray(array, 4);
	printf("Value2: %u\n", value);*/
	
//testing writing
	/*unsigned char array[4] = {0x00, 0x00, 0x00, 0x00};
	array[2] = 'a';
	printf("Initial Values: %c, %c, %c, %c", array[0], array[1], array[2], array[3]);
	unsigned int num = 217;
	WriteUIntToArray(array,0,num);
	printf("Final Values: %c, %c, %c, %c\n", array[0], array[1], array[2], array[3]);
	unsigned int read_res = ReadUIntFromArray(array,0);
	printf("Read: %u\n", read_res);*/
	
//testing short int functions
	/*unsigned char array[4] = {0x00, 0x99, 0x90, 0xFA};
	short int mint = ReadShortInt(array,0);
	printf("old: %hd\n", mint);
	WriteShortInt(array,0,217);
	mint = ReadShortInt(array,0);
	printf("new value: %hd\n", mint);
	WriteShortInt(array,2,182);
	short int mint2 = ReadShortInt(array,2);
	mint = ReadShortInt(array,0);
	printf("finally %hd and %hd\n", mint, mint2);*/
	
//testing space finding
	/*unsigned char array[4] = {0b10101100, 0b00111111, 0b11100000, 0b11111111};
	//read bit at location n L->R
	int n = 10;
	int x = ReadBit(array, 0, 19);
	int y = ReadBit(array, 0, 31);
	printf("x:%d (should be 0) and y:%d (should be 1)\n", x, y);*/
	
//testing space finding
	/*unsigned char array[4] = {0b10101100, 0b00111111, 0b11100000, 0b11111111};
	//read bit at location n L->R
	int n = 10;
	int x = ReadBit(array, 0, 19);
	int y = ReadBit(array, 0, 31);
	//printf("x:%d (should be 0) and y:%d (should be 1)\n", x, y);
	int z = FreeSpaceAddress(9, array, 0, 32);
	printf("Expecting -1: Actual %d\n", z);*/
	
//testing bit writing
	/*unsigned char array[4] = {0b00000000, 0b00000000, 0b00000000, 0b00000000};
	int x = ReadBit(array, 0, 19);
	WriteBit(1, array, 0, 19);
	int y = ReadBit(array, 0, 19);
	printf("Result 1:%d, Result2:%d",x,y);*/

//test parsing a file name
	/*unsigned char array[10];
	char name[7];
	char ext[3];
	
	//initial array
	printf("Initial: ");
	for(int i = 0; i<10; i++){
		printf("%02X ", array[i]);
	}
	printf("\n");
	
	ParseFileName("world.cs", name, ext);
	printf("Name: %s, Ext: %s\n", name, ext);
	
	//copy in name
	for(int i = 0; i<7; i++){
		array[i] = (unsigned char)name[i];
	}
	
	for(int i = 0; i < 3; i++){
		array[i+7] = (unsigned char)ext[i];
	}
	
	//final array
	printf("Final: ");
	for(int i = 0; i<10; i++){
		printf("%02X ", array[i]);
	}
	printf("\n");
	for(int i = 0; i<10; i++){
		printf("%c", array[i]);
	}
	printf("\n");
	*/
	
//testing creating a directory entry
	/*AddDirEntry("world.cs", 10, 5);
	AddDirEntry("note.txt", 1, 16);
	AddDirEntry("vibe.c", 10, 17);
	RemoveDirEntry("vibe.c");
	AddDirEntry("cj.cs", 5, 17);

	
	printf("\nHead: %hd, Tail: %hd\n", ReadSInt(DIR_HEAD), ReadSInt(DIR_TAIL));
	
	PrintDir();
	
	printf("\n");
	printf("Hex read:");
	for(int i = DIR_START; i < (DIR_START + DIR_ENTRY_LEN*5); i++){
		if( i % 16 == 0){
			printf("\n");
			printf("%d: ", i);
		}
		printf("%02X ", MEMORY[i]);
		
	}*/

//testing global file table
	/*printf("\n\n");
	GlobalTableEntry x, y;
	strcpy(x.fname, "world.txt");
	x.fcb.fileSizeBlocks = 77;
	x.fcb.firstBlockIndex = 77;
	x.instances = 1;
	strcpy(y.fname, "zoom.c");
	y.fcb.fileSizeBlocks = 1;
	y.fcb.firstBlockIndex = 1;
	y.instances = 1;
	FindInGlobalTable("world.txt");
	AppendToGlobalTable(x);
	FindInGlobalTable("world.txt");
	AppendToGlobalTable(y);
	RemoveFromGlobalTable("world.txt");
	AppendToGlobalTable(y);*/
	
//testing appending to local table
	/*struct LocalTableEntry localTable[MAX_LOCAL_FILES];
	struct LocalTableEntry x,y;
	strcpy(x.fname,"world.txt");
	x.handle = 9;
	
	strcpy(y.fname, "note.cs");
	y.handle = 100;
	
	AppendToLocalTable(localTable, x);
	printf("LT0: %s (expect world.txt)\n", localTable[0].fname);
	RemoveFromLocalTable(localTable, "world.txt");
	printf("LT0: %s (expect nothing)\n", localTable[0].fname);
	FindInLocalTable(localTable, "world.txt");//expect not found
	AppendToLocalTable(localTable, y);
	printf("LT0: %s (expect note.cs)\n", localTable[0].fname);
	AppendToLocalTable(localTable, x);
	printf("LT1: %s (expect world.txt)\n", localTable[1].fname);
	FindInLocalTable(localTable, "world.txt");//expect found*/



