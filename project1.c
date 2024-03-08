#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

struct VolumeControlBlock{
	int numberOfBlocks;
	int sizeOfBlock;
	int freeBlockCount;
	//bitmap
};

struct FileControlBlock{
	int fileSizeBlocks;
	int firstBlockIndex;
};

unsigned int ReadUIntFromArray(unsigned char *arr, int index){
	
	unsigned int value = 0;
	
	//move to specified location in array
	arr += index;
	
	//read 4 bytes at that location into value
	for(int i = 0; i < 4; i++){
		value = (value << 8) | *arr++;
	}
	
	return value;
}

void WriteUIntToArray(unsigned char *arr, int index, unsigned int value){
	//write 4 bytes of value
	for(int i=0; i<4; i++){
		arr[i + index] = (value >> ((3-i)*8)) & 0xFF;
	}
}

short int ReadShortInt(unsigned char *arr, int index){
	short int value = 0;
	arr += index;
	for(int i = 0; i < 2; i++){
		value = (value << 8) | *arr++;
	}
	
	return value;
}

void WriteShortInt(unsigned char *arr, int index, short int value){
	for(int i=0; i<2; i++){
		arr[i + index] = (value >> ((1-i)*8)) & 0xFF;
	}
}

int main() {
	//allocate 1MB of memory, each entry in the array is a single byte
	unsigned char memory[1048576];
	

	
	//first data block (0-2047) is used as the volume control block
		//4 bytes for an unsigned int for number of blocks
		//4 bytes for an unsigned int for the size of blocks
		//4 bytes for an unsigned int for the free block count
		
			//bytes can be read as integers: "unsigned int value = *(unsigned int*)array;
			/*You can also use a bit shift:
			unsigned int value = 0;
			for(int i = 0; i < bytes_to_read; i++){
				value = (value << 8) | array[start + i];
			}	
			*/
		//512 blocks are free (512 - vcb - 4dir) so 64 bytes for the bit map
		
	
	//second-fifth data block (2048 - 10239) is reserved for the directory 
		//Each entry includes filename, start block number, and file size (16 bytes)
		//4 blocks are needed to store up to 507 file entries (1 block files)
		
		//name is a series of unsigned 9 char (1 byte ec.) and a 3 byte extension = 11 bytes
		// 2 bytes for size (unsigned short int)
		// 2 bytes for location (unsigned short int) - not long enough if blocks are small!
	
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
	




