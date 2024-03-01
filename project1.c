#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

struct VolumeControlBlock{
	int numberOfBlocks;
	int sizeOfBlock;
	int freeBlockCount;
};

struct FileControlBlock{
	int fileSizeBlocks;
	int firstBlockIndex;
};

int main() {
	int a = 10;
	printf("%d\n",a);
	return 0;
}




