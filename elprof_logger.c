#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "elprof_logger.h"
#include "buffer.h"
#include "clocks.h"
#include "elprof_logdata.h"
#include "common.h"

#define DEF_OUT_FILENAME "elprof_lzr.out"
#define MAX_RECORDE_STRING_LEN (1 << 8)
#define BUFFER_INIT_SIZE (1 << 16)
#define BUFFER_LIMIT_SIZE (1 << 17)
#define DATA_END_MARK_STR "THIS IS END"

#define LOG_RECORD_POOL_SIZE (1 << 12)


static float SECOND_PER_CLOCKS;


static int _handle_log_data();
int elprof_logger_save(elprof_CALLSTACK_RECORD *savef)
{
	clock_t delay_time, beg_time_marker, end_time_marker;
	char *chunk;
	chunk = buffer_get_next_chunk(MAX_RECORDE_STRING_LEN);
	delay_time = 0;
	if (!chunk)
	{
		beg_time_marker = get_now_time();
		_handle_log_data();
		end_time_marker = get_now_time();
		delay_time = end_time_marker - beg_time_marker;

		/* reset buffer */
		buffer_reset();
		/* retry */
		chunk = buffer_get_next_chunk(MAX_RECORDE_STRING_LEN);
		ASSERT(chunk, "buffer size is too small!");
	}

	/* save log info to buffer */
	float local_time_sec = (float)savef->local_time * SECOND_PER_CLOCKS;
	float total_time_sec = (float)savef->total_time * SECOND_PER_CLOCKS;
	ASSERT(savef->line_defined != -1, "Runtime error: C Func should have been ignored");
	int size = snprintf(chunk, MAX_RECORDE_STRING_LEN, "%f %f\n%s:%ld\t%s", 
					local_time_sec, total_time_sec, 
					savef->file_defined, 
					savef->line_defined, 
					savef->function_name);
	ASSERT(size > 0, "format log info fail");
#ifdef DEBUG
	printf("%s\n", chunk);
#endif
	buffer_consume_memory_size(size + 1);
	return delay_time;
}

#ifdef DEBUG
static int debug_count = 0;
#endif

static int _handle_log_data()
{
	float local_time;
	float total_time;
	int len = 0;
	char *split;
	char *log_data;
	int size;

#ifdef DEBUG
	debug_count ++;
#endif
	log_data = buffer_get_whole_memory();
	size = buffer_get_used_size();

	while (size > 0)
	{
		sscanf(log_data, "%f %f\n", &local_time, &total_time);
		if ((split = strchr(log_data, '\n')) == NULL)
		{
			fprintf(stderr, "Error parseing log packet data recieved from pipe");
			return -1;
		}

		log_RECORD_pool_add(split + 1, local_time, total_time);
		len = strlen(log_data) + 1;
		log_data += len;
		size -= len;
	}
	return 0;
}

int elprof_logger_init(const char *log_filename)
{
	int ret;

	if (log_filename == NULL)
		log_filename = DEF_OUT_FILENAME; 
	SECOND_PER_CLOCKS = convert_clock_time_seconds(1);
	
	
	if (-1 == buffer_init(BUFFER_INIT_SIZE, BUFFER_LIMIT_SIZE))
	{ 
		fprintf(stderr, "buffer_init failed!"); 
		return -1;
	}
	
	if ((ret = log_RECORD_pool_init(LOG_RECORD_POOL_SIZE, log_filename)) == -1)
		return -1;

	return 0;
}

void elprof_logger_stop(void)
{
	_handle_log_data();
	printf("stop");
	buffer_free();
	log_RECORD_pool_free();
	#ifdef DEBUG
	printf("parent process exit normally!");
	#endif
}

