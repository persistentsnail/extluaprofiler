#include <stdio.h>
#include "clocks.h"

/*
   Here you can choose what time function you are going to use.
   These two defines ('TIMES' and 'CLOCK') correspond to the usage of
   functions times() and clock() respectively.
        Which one is better? It depends on your needs:
                TIMES - returns the clock ticks since the system was up
              (you may use it if you want to measure a system
              delay for a task, like time spent to get input from keyboard)
                CLOCK - returns the clock ticks dedicated to the program
                        (this should be prefered in a multithread system and is
              the default choice)

   note: I guess TIMES don't work for win32
*/

#ifdef TIMES

        #include <sys/times.h>

        static struct tms t;

        #define times(t) times(t)

#else /* ifdef CLOCK */

        #define times(t) clock()

#endif

clock_t get_now_time(void)
{
	return times(&t);
}

float convert_clock_time_seconds(clock_t time_arg)
{
	return (float)time_arg / (float)CLOCKS_PER_SEC;
}
