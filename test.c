#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

int sum; /*data shared by threads*/
void *adder(void *param);

int main(int argc, char *argv[]){
	
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
	
	//creat our threads
	pthread_create(&tid1, &attr, adder, argv[1]);
	pthread_create(&tid2, &attr, adder, argv[1]);
	printf("main created threads\n");
	while(sum <= 1)
	{
		printf("main waiting...\n");
	}
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	
	printf("Final Sum: %d", sum);
	return 0;
}

void *adder(void *param){
	pthread_t tid = pthread_self();

	printf("Thread initialized!\n");
	int i = atoi(param);
	while(sum < i)
	{
		printf("%lu is adding...\n", tid);
		sum++;
		printf("SUM: %d\n", sum);
		sleep(1);
	}
	pthread_exit(0);
}
