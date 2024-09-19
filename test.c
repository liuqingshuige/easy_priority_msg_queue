#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "easy_priority_msg_queue.h"


static void *push(void *param)
{
	PMQHandle queue = (PMQHandle)param;
	if (!queue)
	{
		printf("invalid param, exit\n");
		exit(-1);
	}
	
	int val = 0, prio = 1;

	printf("push thread run loop\n");
	while (1)
	{
		val++;
		prio++;
		bool ret = PushPriorityMsgQueue(queue, (void *)val, prio);
		printf("push msg val: %d, prio: %d, ret: %d\n", val, prio, ret);
		usleep(1000*500);
	}
	return 0;
}

static void *pop(void *param)
{
	PMQHandle queue = (PMQHandle)param;
	if (!queue)
	{
		printf("invalid param, exit\n");
		exit(-1);
	}

	void *val;
	int prio;
	
	printf("pop thread run loop\n");
	while (1)
	{
		PopPriorityMsgQueue(queue, &val, &prio);
		{
			printf("pop msg val: %d, prio: %d, cur-sz: %d\n",
				(int)val, prio, GetPriorityMsgQueueSize(queue));
		}
		usleep(1000*800);
	}
	return 0;
}

int main(int argc, char **argv)
{
	PMQHandle queue = CreatePriorityMsgQueue(8);
	if (!queue)
	{
		printf("create queue failed\n");
		return -1;
	}
	
	pthread_t tid;
	pthread_create(&tid, NULL, push, queue);
	pthread_create(&tid, NULL, pop, queue);
	
	void *val;
	int prio;
	bool ret = false;
	
	printf("wait for sub-thread run first\n");
	sleep(2);
	
	printf("run main loop\n");
	while (1)
	{
		ret = TimedPopPriorityMsgQueue(queue, &val, &prio, 50);
		if (ret)
		{
			printf("main get msg val: %d, prio: %d, cur-sz: %d\n",
				(int)val, prio, GetPriorityMsgQueueSize(queue));
		}
		sleep(1);
	}
	
	return 0;
}


