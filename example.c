#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

int sum; /*data shared b threads*/
void *runner(void *param);/*thread*/

int main(int argc, char *argv[])
{
	pthread_t tid; //thread id
	pthread_attr_t attr; //thread attributes
	
	if(argc != 2)
	{
		fprintf(stderr, "usage:./a.out <positive integer?>\n");
		exit(0);
	}
	
	/* get default attribute */
	pthread_attr_init(&attr);
	
	/*create the thread */
	pthread_create(&tid, &attr, runner, argv[1]);
	printf("Main thread is busy doing something ...\n");
	
	while (sum <= 1)
	{
		printf("Main");
		printf("%d\n", sum);
	}
	
	/* wait for the thread to exit */
	pthread_join(tid, NULL);
	printf("sum = %d \n", sum);
}

void *runner(void *param)
{
	int i, upper = atoi(param);
	printf("Initialized thread: %d, %d\n", i, upper);
	sum = 0;
	for(i = 1; i <= upper; i++)
	{
		sum += i;
		printf("Pthread run %d:", i);
		printf("%d\n", sum);
	}
	pthread_exit(0);
}
