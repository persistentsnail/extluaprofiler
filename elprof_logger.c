#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>

#include "elprof_logger.h"
#include "buffer.h"
#include "clocks.h"
#include "elprof_logdata.h"

#define DEF_OUT_FILENAME "elprof_lzr.out"
#define MAX_RECORDE_STRING_LEN (1 << 8)
#define BUFFER_INIT_SIZE (1 << 16)
#define BUFFER_LIMIT_SIZE (1 << 17)
#define DATA_END_MARK_STR "THIS IS END"

#define LOG_RECORD_POOL_SIZE (1 << 12)


static int pipefd[2];
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
	}
	return 0;
}

static int _read(int fd, char *buf, int n)
{
	int nread;
	int first = true;
	while (n > 0)
	{
		nread = read(fd, buf, n);
		if (nread == 0 && first)
			return 0;
		if (first)
			first = false;
		if (nread <= 0)
			return -1;
		else
			n -= nread;
	}
	return n;
}

int elprof_logger_save(elprof_CALLSTACK_RECORD *savef)
{
	char *chunk = buffer_get_next_chunk(MAX_RECORDE_STRING_LEN);
	if (!chunk)
	{
		char *whole_mem;
		int used_size;
		int data_size;
		int ret;
		clock_t delay_time, beg_time_marker, end_time_marker;
		// send log info buffer data to child process
		whole_mem = buffer_get_whole_memory();
		used_size = buffer_get_used_size();
		data_size = used_size;

		beg_time_marker = get_now_time();
		ret = _write(pipefd[1], &data_size, sizeof(data_size));
		if (ret < 0) { perror("pipe : write head"); return -1; }
		ret = _write(pipefd[1], whole_mem, used_size);
		if (ret < 0) { perror("pipe : write data"); return -1; }
		ret = _write(pipefd[1], DATA_END_MARK_STR, end_marker_str_len);
		if (ret < 0) { perror("pipe : write end"); return -1; }
		end_time_marker = get_now_time();
		delay_time = end_time_marker - beg_time_marker;

		// reset buffer
		buffer_reset();
		return delay_time;
	}
	else
	{
		// save log info to buffer
		char info[MAX_RECORDE_STRING_LEN];
		float local_time_sec = (float)savef->local_time * SECOND_PER_CLOCKS;
		float total_time_sec = (float)savef->total_time * SECOND_PER_CLOCKS;
		int size = snprintf(info, MAX_RECORDE_STRING_LEN, "%s:%d\t%s\n%f\n%f\n", 
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
	int len;

	while (size > 0)
	{
		sscanf(log_str, "%[^\n]%f\n%f\n", source, &local_time, &total_time);
		log_RECORD_pool_add(source, local_time, total_time);

}

static int _child_process_running(const char *log_filename)
{
	int ret;
	int data_size; 
	char *chunk;
	char end_marker_str[64];
	if ((ret = log_RECORD_pool_int(LOG_RECORD_POOL_SIZE)) == -1)
		return -1;
	
	while(1)
	{
		ret = _read(pipefd[0], &data_size, sizeof(data_size));
		if (ret < 0) { perror("pipe : read head"); return -1; }
		else if (ret == 0) return 0; // read finish

		chunk = buffer_get_next_chunk(data_size);
		ret = _read(pipefd[0], chunk, data_size);
		if (ret <= 0) { perror("pipe : read data"); return -1; }

		ASSERT(end_marker_str_len < 64, "the length of end marker string is not reasonable");
		ret = _read(pipefd[0], end_marker_str, end_marker_str_len);
		if (ret <= 0} { perror("pipe : read end"); return -1; }
		// check the packet is reasonable
		end_marker_str[end_marker_str_len] = '\0';
		if (strcmp(end_marker_str, DATA_END_MARK_STR))
			fprintf(stderr, "runtime error : the received log packet is not a reasonable one!");
		else
			_handle_log_data(chunk);
	}
	log_RECORD_pool_free();
	return 0;
}

static void _child_process_exit(int exit_id)
{
	close(pipefd[0]);
	exit(exit_id);
}

int elprof_logger_init(const char *log_filename)
{
	pid_t child_pid;
	int ret;

	if (log_filename == NULL)
		log_filename = DEF_OUT_FILENAME; 
	end_marker_str_len = strlen(DATA_END_MARK_STR);
	SECOND_PER_CLOCKS = convert_clock_time_seconds(1);
	
	if (pipe(pipefd) == -1) { perror("pipe");return -1; }
	if ((child_pid = fork()) == -1) { perror("fork");return -1; }
	if (-1 == buffer_init(BUFFER_INIT_SIZE, BUFFER_LIMIT_SIZE))
	{ fprintf(stderr, "buffer_init failed!"); return -1;}

	if (child_pid == 0)
	{
		close(pipefd[1]);
		// child process do writing log file
		ret  = _child_process_running(log_filename);
		_child_process_exit(ret);
	}
	close(pipefd[0]);
	return 0;
}
