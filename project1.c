#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
//used for parsing file names
#include <string.h>

//allocate 1MB of memory
unsigned char MEMORY[1048576]; // each array index represents a byte

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


//second-fifth data block (2048 - 10239) is reserved for the directory
	
	//first 16 bytes (2048 - 2063) are used to store head and tail for linked list as short ints
const unsigned int DIR_HEAD = 2058;
const unsigned int DIR_TAIL = 2060;
	 
	//Each entry includes filename, start block number, and file size (16 bytes)
	//4 blocks are needed to store up to 507 file entries (1 block files)
	
	//name is a series of 7 unsigned  char (1 byte ec.) and a 3 byte extension = 10 bytes
	// 2 bytes for size (short int)
	// 2 bytes for location (short int)	
	// 2 bytes for memory location of next entry in linked list (short int)
const unsigned int DIR_START = 2064;
const unsigned int DIR_SIZE = 8176;
const unsigned int DIR_ENTRY_END = 16;
const unsigned int DIR_ENTRY_NAME = 0;
const unsigned int DIR_ENTRY_EXT = 7;
const unsigned int DIR_ENTRY_SIZE = 10;
const unsigned int DIR_ENTRY_LOC = 12;
const unsigned int DIR_ENTRY_NEXT = 14;

struct FileControlBlock{
	int fileSizeBlocks;
	int firstBlockIndex;
};



unsigned int ReadUInt(int index){
	//ensure index in memory is a valid unsigned int location
	
	if(index > 12 || (index % 4) > 0){
		printf("Invalid unsigned integer read\n");
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
		printf("Invalid unsigned integer write\n");
		return;
	}
	
	//write 4 bytes of value
	for(int i=0; i<4; i++){
		MEMORY[i + index] = (value >> ((3-i)*8)) & 0xFF;
	}
}

short int ReadSInt(int index){
	//ensure index in memory is a valid short int location
	if( index < 2048 || (index % 16) < 10 || (index % 2) > 0){
		printf("Invalid short integer read\n");
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
	if( index < 2048 || (index % 16) < 10 || (index % 2) > 0){
		printf("Invalid short integer write\n");
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
	return -1;
}

//find the first open space for a new entry in the directory
//returns the index of the start of the space for the entry or -1 to indicate full directory
short int FindDirSpace(){
	for(short int i = DIR_START; i<(DIR_START + DIR_SIZE); i +=16 ){
		if(MEMORY[i] == 0x00){
			return i;
		}
	}
	//if no space is available return -1
	return -1;
}

//validates the file name and outputs the name and extension to the corresponding char arrays
//return 0 if successful and -1 if error
int ParseFileName(const char *str, char *name, char *extension){
	char *dot = strchr(str, '.');
	int length = strlen(str);
	
	//ensure extension is given
	if(dot == NULL){
		printf("Invalid file name. File name must include extension\n");
		return -1;
	}
	
	//validate input
	//check for special characters
	const char *illegalChars = "/\\:*?\"|";
	for(int i = 0; i < length; i++){
		if(strchr(illegalChars, str[i]) != NULL) {
			printf("Invalid file name. File name cannot contain /\\:*?\"|");
			return -1;
		}
	}
	
	//check for multiple dots
	if(strchr(dot+1, '.') != NULL){
		printf("Invalid file name. A file may have only 1 extension. Include a single \".\" character");
		return -1;
	}
	
	//validate lengths
	if( strlen(dot + 1) > 3){
		printf("Invalid file name. Extension is too long. Three characters max");
		return -1;
	}
	
	if((dot - str) > 7){
		printf("File name is too long. Seven characters max");
		return -1;
	}

	//copy file name into the name location
	strncpy(name, str, (dot - str));
	//copy the extension to the extension location
	strcpy(extension, (dot + 1));
	
	return 0;
} 

//Add an entry to the file directory
void AddDirEntry(const char *fname, int fsize, int flocation){
	short int entryLoc = FindDirSpace();
	if(entryLoc < 0){
		printf("Directory full. Could not add file");
		return;
	}
	//input the file details at the entry location
	
	//write name
	//write extension
	//write size in blocks
	WriteSInt((entryLoc + DIR_ENTRY_SIZE),fsize); 
	//write storage location
	WriteSInt((entryLoc + DIR_ENTRY_LOC), flocation);
	
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
	
}

//pointer to a FileControlBlock struct for output, size of the file in blocks, file name
void Create(struct FileControlBlock *fcb, short int size, char *name){
	//print inputs to test validity - remove
	printf("I'm getting:\nsize:%u\nname:%s\nvcb fbc:%u\ndir:%u\n", size, name, VOLUME_CONTROL_BLOCK.FREE_BLOCKS, DIR_START);
	
	//ensure the file size is at least less than the free block count
	if(size > ReadUInt(VOLUME_CONTROL_BLOCK.FREE_BLOCKS)){
		printf("Could not Create File. Desired File Size is too big!\n");
	}else{
		//search the bitmap for a free space of corresponding size
		int blockIndex = FreeSpaceAddress(size);
		printf("blockIndex is %d\n", blockIndex);//shows the index of the first space found
		if(blockIndex < 0){
			//if no space exists error (for now)
			printf("There is not a large enough space available for that file\n");
			//maybe apply compaction...
		}else{
			//if space exists mark it as used
			for(int i = blockIndex; i < (size + blockIndex); i++){
				WriteBit(i, 1);
			}
			//create a directory entry with fname, start block, size
				//parse out file name and extension
				//start block is blockIndex and size is given
				//find the next open directory entry (see notecard)
			//update vcb free block count
			//run the open function to give the fcb values
		}
			
	}
}

int main() {
	
	//initialize default values:
	WriteUInt(VOLUME_CONTROL_BLOCK.BLOCK_COUNT, 512);
	WriteUInt(VOLUME_CONTROL_BLOCK.BLOCK_SIZE, 2048);
	WriteUInt(VOLUME_CONTROL_BLOCK.FREE_BLOCKS, 507);
	//Write the first 5 bits of the bitmap as in use
	for(int i =0; i<5; i++){
		WriteBit(i, 1);
	}

	unsigned int num = ReadUInt(VOLUME_CONTROL_BLOCK.BLOCK_COUNT);	
	unsigned int size = ReadUInt(VOLUME_CONTROL_BLOCK.BLOCK_SIZE);	
	unsigned int free = ReadUInt(VOLUME_CONTROL_BLOCK.FREE_BLOCKS);	
	
	printf("Memory Initialized! \n%u blocks were created of size %u bytes with %u blocks free.\n\n", num, size, free);
	
	//testing Create Function
	struct FileControlBlock fcb;
	Create(&fcb, 7, "world.txt");
	
	//testing string manip
	char str[] = "world.cs";
	
	printf("Testing string: %s\n", str);
	
	for(int i = 0; i < 32; i++){
		printf("%u",ReadBit(i));
	}
	printf("\n");
	
	//test parsing a file name
	char name[7];
	char ext[3];
	ParseFileName(str, name, ext);
	printf("Name: %s, Ext: %s\n", name, ext);
	
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




