#ifndef __ELPROF_LOGDATA_H__
#define __ELPROF_LOGDATA_H__

#define MAX_SOURCE_STR_LEN 256

typedef struct _log_RECORD
{
	char source[MAX_SOURCE_STR_LEN];
	float local_time;
	float total_time;
	int call_times;

}log_RECORD;

int log_RECORD_pool_init(int size);
void log_RECORD_pool_free();

void log_RECORD_pool_add(char *source, float local_time, float total_time);

#endif
