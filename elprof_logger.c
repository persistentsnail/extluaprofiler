#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>

#include "elprof_logger.h"

#define DEF_OUT_FILENAME "elprof_lzr.out"
#define MAX_RECORDE_STRING_LEN (1 << 8)
#define BUFFER_INIT_SIZE (1 << 16)
#define BUFFER_LIMIT_SIZE (1 << 17)
#define DATA_END_MARK_STR "THIS IS END"

static int pipefd[2];
static int end_marker_str_len;

static int _child_process_write_log(FILE *logf)
{
	
}

int elprof_logger_save(elprof_CALLSTACK_RECORD *savef)
{
	char *chunk = buffer_get_next_chunk(MAX_RECORDE_STRING_LEN);
	if (!chunk)
	{
		char *whole_mem;
		int used_size;
		int data_size;
		// send log info buffer data to child process
		whole_mem = buffer_get_whole_memory();
		used_size = buffer_get_used_size();
		data_size = used_size + end_marker_str_len;
		write(pipefd[1], &data_size, sizeof(data_size));
		write(pipefd[1], whole_mem, used_size);
		write(pipefd[1], DATA_END_MARK_STR, end_marker_str_len); 
	}
}

int elprof_logger_init(const char *log_filename)
{
	pid_t child_pid;
	FILE *logf;

	if (log_filename == NULL)
		log_filename = DEF_OUT_FILENAME; 
	end_marker_str_len = strlen(DATA_END_MARK_STR);
	
	if (!(logf = fopen(log_filename, "a"))) { perror("log file");return -1; }
	fclose(logf);
	if (pipe(pipefd) == -1) { perror("pipe");return -1; }
	if ((child_pid = fork()) == -1) { perror("fork");return -1; }

	if (child_pid == 0)
	{
		// child process do writing log file
		logf = fopen(log_filename, "a");
		_child_process_write_log(logf);
		fclose(logf);
		exit(0);
	}
	buffer_init(BUFFER_INIT_SIZE, BUFFER_LIMIT_SIZE);
	return 0;
}
