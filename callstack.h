#ifndef __CALL_STACK_H__
#define __CALL_STACK_H__

#include <time.h>

typedef struct _elprof_CALLSTACK_RECORD elprof_CALLSTACK_RECORD;

struct _elprof_CALLSTACK_RECORD
{
	clock_t	local_time_marker;
	clock_t total_time_marker;

	const char  *file_defined;
	const char *function_name;
	long		 line_defined;
	
	clock_t   local_time;
	clock_t   total_time;
	elprof_CALLSTACK_RECORD *next;
};

typedef elprof_CALLSTACK_RECORD *elprof_CALLSTACK;

typedef _elprof_STATE elprof_STATE;

struct _elprof_STATE
{
	int stack_level;
	elprof_CALLSTACK stack_top;
};

// CALLSTACK RECORDs pool
int CALLSTACK_RECORD_pool_create(int init_size);
elprof_CALLSTACK_RECORD *CALLSTACK_RECORD_new();
void CALLSTACK_RECORD_delete(elprof_CALLSTACK_RECORD *unused);
void CALLSTACK_RECORD_pool_destroy();

// CALLSTACK manipulate
void CALLSTACK_push(elprof_CALLSTACK *p, elprof_CALLSTACK_RECORD *r);
elprof_CALLSTACK_RECORD *CALLSTACK_pop(elprof_CALLSTACK *p);

#endif
