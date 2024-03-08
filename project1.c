#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

struct FileControlBlock{
	int fileSizeBlocks;
	int firstBlockIndex;
};

//variables store the memory index of the value
struct VolumeControlBlock{
	int numberOfBlocks;
	int sizeOfBlock;
	int freeBlockCount;
	int bitmap;
};

unsigned int ReadUInt(unsigned char *arr, int index){
	
	unsigned int value = 0;
	
	//move to specified location in array
	arr += index;
	
	//read 4 bytes at that location into value
	for(int i = 0; i < 4; i++){
		value = (value << 8) | *arr++;
	}
	
	return value;
}

void WriteUInt(unsigned char *arr, int index, unsigned int value){
	//write 4 bytes of value
	for(int i=0; i<4; i++){
		arr[i + index] = (value >> ((3-i)*8)) & 0xFF;
	}
}

short int ReadSInt(unsigned char *arr, int index){
	short int value = 0;
	arr += index;
	for(int i = 0; i < 2; i++){
		value = (value << 8) | *arr++;
	}
	
	return value;
}

void WriteSInt(unsigned char *arr, int index, short int value){
	for(int i=0; i<2; i++){
		arr[i + index] = (value >> ((1-i)*8)) & 0xFF;
	}
}


//helper function to read bitmap
short int ReadBit(unsigned char *arr, int bitmapStart, int bitIndex){
	//get the correct byte
	unsigned char byte = arr[bitmapStart + bitIndex/8];
	//check the bit
	if((byte & (1 << (7-(bitIndex%8)))) == 0){
		return 0;
	}else{
		return 1;
	}
}

//helper function to write to bitmap
void WriteBit(short int value, unsigned char *arr, int bitmapStart, int bitIndex){
	short int current = ReadBit(arr, bitmapStart, bitIndex);
	if(current != value){
		//get the relevant byte
		unsigned char byte = arr[bitmapStart + bitIndex/8];
		//flip the bit
		byte = byte ^ (1 << (7-(bitIndex%8)));
		//write to the array
		arr[bitmapStart + bitIndex/8] = byte;
	}
}

//helper function to find first space in bitmap of size n
//input size of space desired and index of first block will be returned
//bitmap size is 512 for this example (512*2k blocks = 1M)
int FreeSpaceAddress(int spaceSize, unsigned char *arr, int bitmapStart, int bitmapSize){
	int length = 0;
	int index = -1;
	for(int i = 0; i<bitmapSize; i++){
		short int currentBit = ReadBit(arr, bitmapStart, i);
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

//pointer to a FileControlBlock struct for output, size of the file in blocks, file name
void _Create(struct FileControlBlock *fcb, short int size, char *name, unsigned char *memory, struct VolumeControlBlock *vcb, unsigned int directory){
	//print inputs to test validity - remove
	printf("I'm getting:\nsize:%u\nname:%s\nvcb fbc:%u\ndir:%u\n", size, name, vcb->freeBlockCount, directory);
	
	//ensure the file size is at least less than the free block count
	if(size > ReadUInt(memory,(vcb->freeBlockCount))){
		printf("Could not Create File. Desired File Size is too big!\n");
	}else{
		//search the bitmap for a free space of corresponding size
		int blockIndex = FreeSpaceAddress(size, memory, vcb->bitmap, ReadUInt(memory, vcb->numberOfBlocks));
		printf("blockIndex is %d\n", blockIndex);//shows the index of the first space found
		if(blockIndex < 0){
			//if no space exists error (for now)
			printf("There is not a large enough space available for that file\n");
		}else{
			//if space exists...
			
			//fill it in and get the location
			for(int i =0; i<size; i++){
				WriteBit(1, memory, vcb->bitmap, (blockIndex + i));
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
//helper function to prevent redeclaring constant values
void Create(struct FileControlBlock *fcb, short int size, char *name);


int main() {
	//allocate 1MB of memory
	unsigned char memory[1048576]; // each array index represents a byte
	
	//set variables to indicate data structure locations
	//first data block (0-2047) is used as the volume control block
	//4 bytes for an unsigned int for number of blocks - 512 [0]
	//4 bytes for an unsigned int for the size of blocks - 2048 Bytes (array spaces) [4]
	//4 bytes for an unsigned int for the free block count - 507 (first 5 blocks are meta) [8]
	//64 bytes for the bit map because 512 blocks exist (512 - 1vcb - 4dir) = 64 [12]
	struct VolumeControlBlock volumeControlBlock;
	volumeControlBlock.numberOfBlocks = 0;
	volumeControlBlock.sizeOfBlock = 4;
	volumeControlBlock.freeBlockCount = 8;
	volumeControlBlock.bitmap = 12;

	//second-fifth data block (2048 - 10239) is reserved for the directory 
		//Each entry includes filename, start block number, and file size (16 bytes)
		//4 blocks are needed to store up to 507 file entries (1 block files)
		
		//name is a series of unsigned 9 char (1 byte ec.) and a 3 byte extension = 12 bytes
		// 2 bytes for size (unsigned short int)
		// 2 bytes for location (unsigned short int)	
	const unsigned int DIRECTORY = 2048;
	const unsigned int DIRECTORY_ENTRY_SIZE = 16;
	
	//initialize default values:
	WriteUInt(memory, volumeControlBlock.numberOfBlocks, 512);
	WriteUInt(memory, volumeControlBlock.sizeOfBlock, 2048);
	WriteUInt(memory, volumeControlBlock.freeBlockCount, 507);
	//Write the first 5 bits of the bitmap as in use
	for(int i =0; i<5; i++){
		WriteBit(1, memory, volumeControlBlock.bitmap, i);
	}

	unsigned int num = ReadUInt(memory, volumeControlBlock.numberOfBlocks);	
	unsigned int size = ReadUInt(memory, volumeControlBlock.sizeOfBlock);	
	unsigned int free = ReadUInt(memory, volumeControlBlock.freeBlockCount);	
	
	printf("Memory Initialized! \n%u blocks were created of size %u bytes with %u blocks free.\n\n", num, size, free);
	
	//function to create a file
	void Create(struct FileControlBlock *fcb, short int size, char *name){
		_Create(fcb,size,name,memory,&volumeControlBlock,DIRECTORY);
	}
	//testing Create Function
	struct FileControlBlock fcb;
	Create(&fcb,10, "world.txt");//expect index at block 5, takes up 10 blocks, next guy at 15
	
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




