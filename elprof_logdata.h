#ifndef __ELPROF_LOGDATA_H__
#define __ELPROF_LOGDATA_H__

#define MAX_SOURCE_STR_LEN 256

typedef struct _log_RECORD
{
	char source[MAX_SOURCE_STR_LEN];
	float local_time;
	float total_time;
	int count;

}log_RECORD;

int log_RECORD_pool_init(int max_size);
void log_RECORD_pool_free();

log_RECORD * log_RECORD_pool_add(log_RECORD );

#endif
