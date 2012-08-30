#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elprof_core.h"
#include "elprof_logger.h"
#include "clocks.h"
#include "common.h"

#define MAX_STATE_SIZE 512

typedef struct _states_list_node
{
	elprof_STATE eS;
	void *luaS;
	int id;
	int free;

	clock_t stop_time_marker;
}SListNode;

static SListNode s_states[MAX_STATE_SIZE];
static int s_maxSize;
static int s_freeIds[MAX_STATE_SIZE];
static int s_freeSize;


static void states_init()
{
	int i;
	for (i = 0; i < MAX_STATE_SIZE; i++)
	{
		s_freeIds[i] = MAX_STATE_SIZE - 1 - i;
		s_states[i].id = i;
		s_states[i].free = 1;
	}
	s_freeSize = MAX_STATE_SIZE;
	s_maxSize = 0;
}

static void states_free()
{
	int i;
	for (i = 0; i < s_maxSize; i++)
	{
		if (!s_states[i].free)
			while(1 == elprof_callhookOUT(s_states[i].luaS));
	}
}

static SListNode * states_get_state(void *luaS)
{
	int i;
	for (i = 0; i < s_maxSize; i++)
	{
		if (!s_states[i].free && s_states[i].luaS == luaS)
			return &s_states[i];
	}
	return NULL;
}

/*
 * output state info for debug
 */
static void output_state(SListNode *state)
{
	elprof_STATE *S = &state->eS;
	elprof_CALLSTACK_RECORD *p = S->stack_top;
	fprintf(stderr, "***********new state, free is %d, stack_level is %d***********\n", state->free, S->stack_level);
	while (p)
	{
		fprintf(stderr, "%s : %d : %s\n", p->file_defined, p->line_defined, p->function_name);
		p = p->next;
	}
}
static int s_dcnt = 0;

static SListNode * states_new_state(void *luaS)
{
	s_dcnt ++;
	if (s_freeSize <= 0)
	{
		int i;
		fprintf(stderr, "not enough state, s_dcnt is %d\n", s_dcnt);
		for (i = 0; i < s_maxSize; i++)
			output_state(&s_states[i]);
		return NULL;
	}
	int id = s_freeIds[--s_freeSize];
	SListNode *state = &s_states[id];
	if (id + 1 > s_maxSize)
		s_maxSize = id + 1;

	state->luaS = luaS;
	state->free = 0;
	state->stop_time_marker = 0;
	state->eS.stack_level = 0;
	state->eS.stack_top   = NULL;
	return state;
}

static void states_free_state(SListNode *state)
{
	s_dcnt --;
	s_freeIds[s_freeSize++] = state->id;
	if (s_freeSize > MAX_STATE_SIZE)
	{
		fprintf(stderr, "the number of state which freed is exceed\n");
		return;
	}
	if (state->id + 1 == s_maxSize)
		s_maxSize--;
	state->free = 1;
}

static void fix_callstack_time(elprof_CALLSTACK stack_top, int delay_time)
{
	while (stack_top)
	{
		stack_top->total_time -= delay_time;
		stack_top = stack_top->next;
	}
}

static void *pre_luaS = NULL;
static SListNode *pre_S = NULL;

static SListNode * get_state_hooker(void *luaS)
{
	SListNode *S;

	if (luaS == pre_luaS && pre_S->free == 0)
		return pre_S;
	S = states_get_state(luaS);
	return (S = S? S : states_new_state(luaS)); 
}

static void on_thread_changed(SListNode *cur_S, clock_t now_time_marker)
{
	if (pre_S)
		pre_S->stop_time_marker = now_time_marker;
	fix_callstack_time(cur_S->eS.stack_top, now_time_marker - cur_S->stop_time_marker);
	pre_luaS = cur_S->luaS;
	pre_S    = cur_S;
}

int elprof_callhookIN(void *luaS, const char *func_name, const char *file, int linedefined)
{
	SListNode *state = get_state_hooker(luaS);
	if (!state) return -1;
	elprof_STATE *S = &state->eS;


	clock_t now_time_marker;
	elprof_CALLSTACK_RECORD *parentf = S->stack_top;
	elprof_CALLSTACK_RECORD *newf = CALLSTACK_RECORD_new();
	
	if (!newf)
	{
	#ifdef DEBUG
		int i;
		fprintf(stderr, "not enough callstack rec, s_dcnt is %d\n", s_dcnt);
		for (i = 0; i < MAX_STATE_SIZE; i++)
			output_state(&s_states[i]);
	#endif
		return -1;
	}

	CALLSTACK_push(&(S->stack_top), newf);
	S->stack_level ++;

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
	if (luaS != pre_luaS)
		on_thread_changed(state, now_time_marker);
	return 0;
}

int elprof_callhookOUT(void *luaS)
{
	clock_t now_time_marker;
	int delay_time;
	elprof_CALLSTACK_RECORD *leavef;

	elprof_STATE *S;
	SListNode *state = get_state_hooker(luaS);
	if (!state) return -1;
	S = &state->eS;
		
	if (S->stack_level-- == 0)
	{
		states_free_state(state);
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
			now_time_marker = get_now_time(); /* for top record local_time_marker */
			/* fix every call stack records' time */
			fix_callstack_time(S->stack_top, delay_time);
		}
	}
	CALLSTACK_RECORD_delete(leavef);
	if (S->stack_level != 0)
		S->stack_top->local_time_marker = now_time_marker;
	if (luaS != pre_luaS)
		on_thread_changed(state, now_time_marker);
	if (S->stack_level == 0)
		states_free_state(state);
	return 1;
}

int elprof_core_init(void *luaS, const char *out_filename)
{
	states_init();
		/* create CALLSTACK RECORDS pool */
	if (CALLSTACK_RECORD_pool_create(1024) == -1 || elprof_logger_init(out_filename) == -1)
			return -1;
	return 0;
}

void elprof_core_finalize()
{
	states_free();
	CALLSTACK_RECORD_pool_destroy();
	elprof_logger_stop();
}
