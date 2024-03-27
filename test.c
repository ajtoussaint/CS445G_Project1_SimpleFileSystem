#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

//global data
int sum; 
#define arrsize 8 //took 65s to get 64 size the slow way vs 63
unsigned char array[arrsize] = {'a'};
unsigned char key[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

pthread_mutex_t mutex[arrsize] = PTHREAD_MUTEX_INITIALIZER;

void *adder(){
	pthread_t tid = pthread_self();
	int num = ((unsigned long)tid % 10);
	printf("Thread %d initialized!\n", num);
	while(strchr(array,'a') > 0)
	{
		for(int i = 0; i < arrsize; i++){
			printf("Thread %d waits for mutex to check %d\n", num, i);
			pthread_mutex_lock(&mutex[i]);
			printf("Thread %d seizes mutex to check %d\n", num, i);
			if(array[i] == 'a'){
				printf("Thread %d found space at arr[%d]\n", num, i);
				usleep(500);//force race condition
				array[i] = key[num];
				printf("Thread %d modified arr[%d] and released mutex\n", num, i);
				pthread_mutex_unlock(&mutex[i]);
				break;
			}
			pthread_mutex_unlock(&mutex[i]);
			printf("Thread %d releases mutex after visiting %d\n", num, i);
			usleep(500);
		}
		usleep(100);
	}
	pthread_exit(0);
}

void *backadder(){
	pthread_t tid = pthread_self();
	int num = ((unsigned long)tid % 10);
	printf("Thread %d initialized!\n", num);
	while(strchr(array,'a') > 0)
	{
		for(int i = arrsize; i >= 0; i--){
			printf("Thread %d waits for mutex to check %d\n", num, i);
			pthread_mutex_lock(&mutex[i]);
			printf("Thread %d seizes mutex to check %d\n", num, i);
			if(array[i] == 'a'){
				printf("Thread %d found space at arr[%d]\n", num, i);
				usleep(500);//force race condition
				array[i] = key[num];
				printf("Thread %d modified arr[%d] and released mutex\n", num, i);
				pthread_mutex_unlock(&mutex[i]);
				break;
			}
			pthread_mutex_unlock(&mutex[i]);
			printf("Thread %d releases mutex after visiting %d\n", num, i);
			usleep(500);
		}
		usleep(100);
	}
	pthread_exit(0);
}

int main(int argc, char *argv[]){
	clock_t start = clock();
	
	pthread_t tid1;
	pthread_t tid2;
	pthread_attr_t attr; //thread attributes
	pthread_attr_init(&attr); //initialize attributes
	
	//require 1 arg, a positive int > 1 
	if(argc != 2)
	{
		fprintf(stderr, "usage:./a.out <positive integer?>\n");
		exit(0);
	}
	printf("Res: %d\n", strchr(array, 'a'));
	/*//creat our threads
	printf("Initial arr: %s\n", array);
	pthread_create(&tid1, &attr, adder, argv[1]);
	pthread_create(&tid2, &attr, backadder, argv[1]);
	printf("main created threads\n");

	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	
	printf("Final arr: %s\n", array);
	clock_t end = clock();
	double time = ((double)(end - start))/CLOCKS_PER_SEC;
	printf("Time: %d\n", (int)(time * 1000)); */
	return 0;
}


