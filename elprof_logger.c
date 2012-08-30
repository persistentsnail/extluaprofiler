#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

#include "elprof_logger.h"
#include "buffer.h"
#include "clocks.h"
#include "elprof_logdata.h"
#include "common.h"

#define DEF_OUT_FILENAME "elprof_xxx.out"
#define MAX_FILE_DEF_NAME (1 << 7)
#define MAX_FUNC_NAME (1 << 6)
#define MAX_RAW_LOG_RECORD (1 << 8)
#define BUFFER_INIT_SIZE (1 << 16)
#define BUFFER_LIMIT_SIZE (1 << 17)
#define DATA_END_MARK_STR "THIS IS END"

#define LOG_RECORD_POOL_SIZE (1 << 12)


static int pipefd[2];
static pid_t child_pid;
static int end_marker_str_len;
static int s_logger_running;


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
	if (data_size <= 0)
		return 0;
	
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

void _terminate_logger()
{
	buffer_free();
	close(pipefd[1]);
	s_logger_running = 0;
}

int elprof_logger_save(elprof_CALLSTACK_RECORD *savef)
{
	clock_t delay_time, beg_time_marker, end_time_marker;
	char *chunk;
	int size;
	int len;
	const char *loop;

	if (!s_logger_running)
		return -1;
	
	chunk = buffer_get_next_chunk(MAX_RAW_LOG_RECORD);
	delay_time = 0;
	if (!chunk)
	{
		beg_time_marker = get_now_time();
		if (-1 == _send_log_data())
		{
			_terminate_logger();
			return -1;
		}
		end_time_marker = get_now_time();
		delay_time = end_time_marker - beg_time_marker;

		/* reset buffer */
		buffer_reset();
		/* retry */
		chunk = buffer_get_next_chunk(MAX_RAW_LOG_RECORD);
		ASSERT(chunk, "buffer size is too small!");
	}

	/* save log info to buffer */
	size = 0;

	*(clock_t *)chunk = savef->local_time;
	size += sizeof(clock_t);
	chunk += sizeof(clock_t);

	*(clock_t *)chunk = savef->total_time;
	size += sizeof(clock_t);
	chunk += sizeof(clock_t);

	*(int *)chunk = savef->line_defined;
	size += sizeof(int);
	chunk += sizeof(int);

	len = 0;
	if ((loop = savef->file_defined))
	{
		while ((*chunk++ = *loop++) && (++len < MAX_FILE_DEF_NAME)); 
		if (len == MAX_FILE_DEF_NAME)
			*(chunk - 1) = '\0';
		size += (len < MAX_FILE_DEF_NAME ? len + 1 : len);
	}
	else
	{
		*chunk++ = '\0';
		size++;
	}
	
	len = 0;
	if ((loop = savef->function_name))
	{
		while ((*chunk++ = *loop++) && (++len < MAX_FUNC_NAME));
		if (len == MAX_FUNC_NAME)
			*(chunk - 1) = '\0';
		size += (len < MAX_FUNC_NAME ? len + 1 : len);
	}
	else
	{
		*chunk++ = '\0';
		size++;
	}

	buffer_consume_memory_size(size);
	return delay_time;
}


static int _handle_log_data(char *log_data, int size)
{
	float local_time;
	float total_time;
	int line_defined;
	char *file_defined;
	char *function_name;
	int offset = 0;
	char *saved;

	while (size > 0)
	{
		saved = log_data;

		local_time = convert_clock_time_seconds(*(clock_t *)log_data);
		log_data += sizeof(clock_t);
		total_time = convert_clock_time_seconds(*(clock_t *)log_data);
		log_data += sizeof(clock_t);
		line_defined = *(int *)log_data;
		log_data += sizeof(int);

		offset = strlen(log_data) + 1;
		ASSERT(offset <= MAX_FILE_DEF_NAME, "file name Overflow!");
		file_defined = log_data;
		log_data += offset;

		offset = strlen(log_data) + 1;
		ASSERT(offset <= MAX_FUNC_NAME, "func name Overflow!");
		function_name = log_data;
		log_data += offset;

		log_RECORD_pool_add(file_defined, function_name, line_defined, local_time, total_time);

		size -= (log_data - saved);
	}
	ASSERT(size == 0, "_handle_log_data Runtime Error :  size is not multiply of raw log RECORD!")
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
		fprintf(stderr, "child process exit abnormally!\n");
	else
		fprintf(stderr, "child process exit normally!\n");

	close(pipefd[0]);
	buffer_free();
	exit(exit_id);
}


int elprof_logger_init(const char *log_filename)
{
	int ret;
	struct sigaction act;

	if (log_filename == NULL)
		log_filename = DEF_OUT_FILENAME; 
	end_marker_str_len = strlen(DATA_END_MARK_STR);
	
	if (pipe(pipefd) == -1) { perror("pipe");return -1; }
	
	if ((child_pid = fork()) == -1) { perror("fork");return -1; }
	
	if (-1 == buffer_init(BUFFER_INIT_SIZE, BUFFER_LIMIT_SIZE))
	{ 
		fprintf(stderr, "buffer_init failed!"); 
		return -1;
	}

	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;
	sigemptyset(&act.sa_mask);
	sigaction(SIGPIPE, &act, NULL);

	if (child_pid == 0)
	{
		char filename[256];
		snprintf(filename, sizeof(filename), "%s-%d", log_filename, getpid());
		close(pipefd[1]);
		/* child process do writing log file */
		ret  = _child_process_running(filename);
		_child_process_exit(ret);
	}
	close(pipefd[0]);
	ASSERT(MAX_RAW_LOG_RECORD > MAX_FUNC_NAME + MAX_FILE_DEF_NAME + 
		sizeof(int) + 2 * sizeof(clock_t), "Total Max Raw Log Record is too small!");
	s_logger_running = 1;
	
	return 0;
}

void elprof_logger_stop(void)
{
	int status;

	if (s_logger_running)
	{
		_send_log_data();
		_terminate_logger();
	}
	waitpid(child_pid, &status, 0);
	DBG(fprintf(stderr, "logger has stopped!\n"));
}

