#include <stdio.h>
#include "elprof_logdata.h"

static int __h_max_entrys = 4;

typedef struct _entry_t
{
	log_RECORD record;
	unsigned long hash_val;
};

typedef struct _bucket_t
{
	int entry_num;
	entry_t entrys[__h_max_entrys]; 
};

int log_RECORD_pool_int(int max_size)
{
}
