#include <stdio.h>
#include <stdlib.h>
#include "callstack.h"
#define POOL_SIZE_LIMIT 1024

/**************************************************
 * CALLSTACK RECORDS pool implements
 **************************************************/

typedef struct _C_R_pool
{
	void *records[POOL_SIZE_LIMIT];
	int pool_size;
	int left_size;
} C_R_pool;

static C_R_pool s_CRpool;
static elprof_CALLSTACK_RECORD *s_eC_records;

static void _alloc_records_into_pool(int count, int beg_index, int end_index)
{
	int index;
	s_eC_records = (elprof_CALLSTACK_RECORD *)malloc(sizeof(elprof_CALLSTACK_RECORD) * count);

	for (index = beg_index; index < end_index && count > 0; index++, count--)
		s_CRpool.records[index] = &s_eC_records[index];
}

int CALLSTACK_RECORD_pool_create(int init_size)
{
	/* too large size, It is necessary to do? */
	if (init_size > POOL_SIZE_LIMIT)
		return -1;
	s_eC_records = NULL;
	
	_alloc_records_into_pool(init_size, 0, init_size);
	s_CRpool.pool_size = init_size;
	s_CRpool.left_size = init_size;
	return 0;
}

elprof_CALLSTACK_RECORD *CALLSTACK_RECORD_new()
{
	if (s_CRpool.left_size <= 0)
		return NULL;
	return (elprof_CALLSTACK_RECORD *)s_CRpool.records[--s_CRpool.left_size];
}

void CALLSTACK_RECORD_delete(elprof_CALLSTACK_RECORD *unused)
{
	if (++s_CRpool.left_size > s_CRpool.pool_size) 
		return;
	s_CRpool.records[s_CRpool.left_size - 1] = unused;
}

void CALLSTACK_RECORD_pool_destroy()
{
	free(s_eC_records);
}

/**************************************************
 ** CALLSTACK manipulation implements
 **************************************************/

void CALLSTACK_push(elprof_CALLSTACK *p, elprof_CALLSTACK_RECORD *r)
{
	r->next = *p;
	*p = r;
}

elprof_CALLSTACK_RECORD *CALLSTACK_pop(elprof_CALLSTACK *p)
{
	elprof_CALLSTACK_RECORD *r = *p;
	*p = (*p)->next;
	return r;
}
