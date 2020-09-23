#include <stdio.h>  
#include <stdlib.h>  
#include <unistd.h>
#include <sys/types.h>  
#include <fcntl.h>  
#include <errno.h>  
#include <time.h>  
#include <linux/input.h>  



#ifdef CONFIG 
#include <config.h>
#endif

#ifndef EVENT_BUTTON_PATH
#define EVENT_BUTTON_PATH "/dev/input/event1"
#endif 

#define MODULE_NAME "button"

static int gi_ButtonDesc;


double difftimeval(const struct timeval *start, const struct timeval *end)
{
        double d;
        time_t s;
        suseconds_t u;

        s = start->tv_sec - end->tv_sec;
        u = start->tv_usec - end->tv_usec;
        //if (u < 0)
        //        --s;

        d = s;
        d *= 1000000.0;//1 秒 = 10^6 微秒
        d += u;

        return d;
}


#define INCERVAL (800000.0)
int EventRead(void)
{
	struct input_event event;;
	static struct timeval Value_1_time;
	double diff;
	while(1)
	{
		if(read(gi_ButtonDesc, &event,sizeof(event)) != sizeof(event))
		{
			printf(MODULE_NAME": An error occurred while reading the key\n");
			return -1;
		}
#if 0
		if(event.type != EV_SYN)
		   printf("Event: time %ld.%ld, type %d, code %d,value %d\n",
				 event.time.tv_sec,event.time.tv_usec,
				 event.type,
				 event.code,
				 event.value);
#endif
		if(event.type == EV_KEY && (event.value == 1 || event.value == 2) )
		{
			if(event.value == 1)
				Value_1_time = event.time;
			else
			{
				diff = difftimeval(&event.time, &Value_1_time);
				if(diff < INCERVAL)
					continue;
			}
			return event.code;
		}	
	}
	return 0;
}
int EventInit(void)
{
	gi_ButtonDesc = open(EVENT_BUTTON_PATH,O_RDONLY);
	if(gi_ButtonDesc < 0)
	{
		printf(MODULE_NAME": Failed to open key device\n");
		return -1;
	}
	return 0;
}

void EventExit(void)
{
	close(gi_ButtonDesc);
}




