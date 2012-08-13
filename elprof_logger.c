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


static int pipefd[2];
static pid_t child_pid;
static int end_marker_str_len;
static float SECOND_PER_CLOCKS;



static int _write(int fd, char *buf, int n)
{
	int nwritten;
	while (n > 0)
	{
		nwritten = write(fd, buf, n);
		if (nwritten <= 0)
			return -1;
		else
			n -= nwritten;
		buf += nwritten;
	}
	return 0;
}

static int _read(int fd, char *buf, int n)
{
	int nread;
	int first = 1;
	int tmp = n;
	while (n > 0)
	{
		nread = read(fd, buf, n);
		if (nread == 0 && first)
			return 0;

		if (first) first = 0;

		if (nread <= 0)
			return -1;
		else
			n -= nread;
		buf += nread;
	}
	return tmp;
}

/* send log info buffer data to child process */
static int _send_log_data()
{
	char *whole_mem;
	int used_size;
	int data_size;
	int ret;
	
	whole_mem = buffer_get_whole_memory();
	used_size = buffer_get_used_size();
	data_size = used_size;
	
	/* write head */
	ret = _write(pipefd[1], (char *)&data_size, sizeof(data_size));
	if (ret < 0) 
	{ 
		perror("pipe : write head");
		return -1;
	}

	/* write data */
	ret = _write(pipefd[1], whole_mem, used_size);
	if (ret < 0) 
	{ 
		perror("pipe : write data");
		return -1;
	}

	/* write end */
	ret = _write(pipefd[1], DATA_END_MARK_STR, end_marker_str_len);
	if (ret < 0) 
	{ 
		perror("pipe : write end");
		return -1;
	}
	return 0;
}

int elprof_logger_save(elprof_CALLSTACK_RECORD *savef)
{
	char *chunk = buffer_get_next_chunk(MAX_RECORDE_STRING_LEN);
	if (!chunk)
	{
		clock_t delay_time, beg_time_marker, end_time_marker;

		beg_time_marker = get_now_time();
		if (-1 == _send_log_data())
			return -1;
		end_time_marker = get_now_time();
		delay_time = end_time_marker - beg_time_marker;

		/* reset buffer */
		buffer_reset();
		return delay_time;
	}
	else
	{
		/* save log info to buffer */
		char info[MAX_RECORDE_STRING_LEN];
		float local_time_sec = (float)savef->local_time * SECOND_PER_CLOCKS;
		float total_time_sec = (float)savef->total_time * SECOND_PER_CLOCKS;
		int size = snprintf(info, MAX_RECORDE_STRING_LEN, "%s:%ld\t%s\n%f\n%f\n", 
							savef->file_defined, 
							savef->line_defined, 
							savef->function_name, 
							local_time_sec, total_time_sec); 
		ASSERT(size > 0, "format log info fail");
		buffer_consume_memory_size(size + 1);
		return 0;
	}
}

static int _handle_log_data(char *log_data, int size)
{
	char source[MAX_SOURCE_STR_LEN];
	float local_time;
	float total_time;
	int len = 0;

	while (size > 0)
	{
		sscanf(log_data, "%[^\n]%f\n%f\n", source, &local_time, &total_time);
		log_RECORD_pool_add(source, local_time, total_time);
		len = strlen(log_data) + 1;
		log_data += len;
		size -= len;
	}
	return 0;
}

static int _child_process_running(const char *log_filename)
{
	int ret;
	int data_size; 
	char *chunk;
	char end_marker_str[64];
	int exit_id;

	if ((ret = log_RECORD_pool_init(LOG_RECORD_POOL_SIZE, log_filename)) == -1)
		return -1;
	while(1)
	{

		/* read head */
		ret = _read(pipefd[0], (char *)&data_size, sizeof(data_size));
		if (ret < 0)
		{
			perror("pipe : read head"); 
			exit_id = -1;
			break;
		}
		else if (ret == 0)/* read finish */
		{ 
			exit_id = 0; 
			break; 
		}

		/* read data */
		chunk = buffer_get_next_chunk(data_size);
		ret = _read(pipefd[0], chunk, data_size);
		if (ret <= 0)
		{ 
			perror("pipe : read data"); 
			exit_id = -1; 
			break;
		}

		/* read end */
		ASSERT(end_marker_str_len < 64, "the length of end marker string is not reasonable");
		ret = _read(pipefd[0], end_marker_str, end_marker_str_len);
		if (ret <= 0)
		{
			perror("pipe : read end");
			exit_id = -1;
			break;
		}

		/* check the packet is reasonable */
		end_marker_str[end_marker_str_len] = '\0';
		if (strcmp(end_marker_str, DATA_END_MARK_STR))
			fprintf(stderr, "runtime error : the received log packet is not a reasonable one!");
		else
			_handle_log_data(chunk, data_size);
	}
	log_RECORD_pool_free();
	return exit_id;
}

static void _child_process_exit(int exit_id)
{
	if (exit_id != 0)
		fprintf(stderr, "child process exit abnormally!");
	else
		fprintf(stderr, "child process exit normally!");

	close(pipefd[0]);
	buffer_free();
	exit(exit_id);
}

int elprof_logger_init(const char *log_filename)
{
	int ret;

	if (log_filename == NULL)
		log_filename = DEF_OUT_FILENAME; 
	end_marker_str_len = strlen(DATA_END_MARK_STR);
	SECOND_PER_CLOCKS = convert_clock_time_seconds(1);
	
	if (pipe(pipefd) == -1) { perror("pipe");return -1; }
	
	if ((child_pid = fork()) == -1) { perror("fork");return -1; }
	
	if (-1 == buffer_init(BUFFER_INIT_SIZE, BUFFER_LIMIT_SIZE))
	{ 
		fprintf(stderr, "buffer_init failed!"); 
		return -1;
	}

	if (child_pid == 0)
	{
		close(pipefd[1]);
		/* child process do writing log file */
		ret  = _child_process_running(log_filename);
		_child_process_exit(ret);
	}
	close(pipefd[0]);
	return 0;
}

void elprof_logger_stop(void)
{
	int status;
	printf("stop");
	_send_log_data();
	buffer_free();
	close(pipefd[1]);
	waitpid(child_pid, &status, 0);
	#ifdef DEBUG
	printf("parent process exit normally!");
	#endif
}

