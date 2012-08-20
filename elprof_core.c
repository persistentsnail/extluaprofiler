#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elprof_core.h"
#include "elprof_logger.h"
#include "clocks.h"
#include "common.h"


void elprof_callhookIN(elprof_STATE *S, const char *func_name, const char *file, int linedefined)
{
	S->stack_level ++;
	clock_t now_time_marker;
	elprof_CALLSTACK_RECORD *parentf = S->stack_top;
	elprof_CALLSTACK_RECORD *newf = CALLSTACK_RECORD_new();
	#ifdef DEBUG
	if (!newf)
	{
	printf("mem not enough\nstack is\n");
	elprof_CALLSTACK_RECORD *p = S->stack_top;
	while (p)
	{
		fprintf(stderr, "%s : %d : %s\n", p->file_defined, p->line_defined, p->function_name);
		p = p->next;
	}
	}
	#endif
	CALLSTACK_push(&(S->stack_top), newf);

	newf->file_defined  = file;
	newf->line_defined   = linedefined;
	newf->function_name = func_name;
	newf->local_time    = 0;
	newf->total_time    = 0;

	now_time_marker = get_now_time();
	newf->local_time_marker = now_time_marker;
	newf->total_time_marker = now_time_marker;
	
	if (parentf)
		parentf->local_time += now_time_marker - parentf->local_time_marker;
}

int elprof_callhookOUT(elprof_STATE *S)
{
	clock_t now_time_marker;
	int delay_time;
	elprof_CALLSTACK_RECORD *leavef;
	if (S->stack_level-- == 0)
	{
		S->stack_level = 0;
		return 0;
	}
	now_time_marker = get_now_time();
	leavef = CALLSTACK_pop(&(S->stack_top));
	leavef->local_time += now_time_marker - leavef->local_time_marker;
	leavef->total_time += now_time_marker - leavef->total_time_marker;

	if (leavef->line_defined != -1)
	{/* save call stack info */
		delay_time = elprof_logger_save(leavef); 
		if (delay_time < 0)
		{
			/*fprintf(stderr, "save the call of %s : %d : %s failed!",
							leavef->file_defined,
							leavef->line_defined,
							leavef->function_name
				   );*/
		}
		else if (delay_time > 0)
		{
				/* fix every call stack records' time */
				elprof_CALLSTACK p;
				p = S->stack_top;
				now_time_marker = get_now_time(); /* for top record local_time_marker */
				while (p)
				{
						p->total_time -= delay_time;
						p = p->next;
				}
		}
	}
	CALLSTACK_RECORD_delete(leavef);
	if (S->stack_level != 0)
		S->stack_top->local_time_marker = now_time_marker;
	return 1;
}

elprof_STATE *elprof_core_init(const char *out_filename)
{
	elprof_STATE *S;
	S = (elprof_STATE *)malloc(sizeof(elprof_STATE));
	if (S)
	{
		S->stack_level = 0;
		S->stack_top = NULL;

		/* create CALLSTACK RECORDS pool */
		if (CALLSTACK_RECORD_pool_create(128) == -1 || elprof_logger_init(out_filename) == -1)
		{
			free(S);
			return NULL;
		}
	}
	return S;
}

void elprof_core_finalize(elprof_STATE *S)
{
	if (S)
	{
		while(elprof_callhookOUT(S)) ;
		CALLSTACK_RECORD_pool_destroy();
		free(S);
		elprof_logger_stop();
		S = NULL;
	}
}
