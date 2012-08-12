#include <stdio.h>
#include <string.h>

// file buffer include file
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
//
#include "buffer.h"

typedef struct buffer_t
{
	char *memory;
	int used_size;
	int total_size;

	int init_size;
	int limit_size;
} buffer_t;

static buffer_t s_buffer;

int buffer_init(int init_size, int limit_size)
{
	s_buffer.memory = (char *)malloc(init_size);
	if (!s_buffer.memory) return -1;
	s_buffer.used_size = 0;
	s_buffer.total_size = init_size;
	s_buffer.init_size = init_size;
	s_buffer.limit_size = limit_size;
	return 0;
}

char * buffer_get_next_chunk(int chunk_size)
{
	if (chunk_size + s_buffer.used_size > s_buffer.total_size)
	{
		if (s_buffer.total_size < s_buffer.limit_size)
		{
			s_buffer.memory = (char *)realloc(s_buffer.memory, s_buffer.limit_size);
			s_buffer.total_size = s_buffer.limit_size;
			return buffer_get_next_chunk(chunk_size);
		}
		return NULL;
	}
	else
		return s_buffer.memory + s_buffer.used_size;
}

void buffer_consume_memory_size(int used_size)
{
	s_buffer.used_size += used_size;
}

void buffer_free()
{
	free(s_buffer.memory);
}

char * buffer_get_whole_memory()
{
	return s_buffer.memory;
}

int buffer_get_used_size()
{
	return s_buffer.used_size;
}

void buffer_reset()
{
	s_buffer.used_size = 0;
}


//*************************************************************
//*************************************************************
// FILE Buffer System
// ************************************************************
// ************************************************************
typedef struct _file_buffer_t
{
	int fd;
	int file_size;

	char *memory;
	int buffer_size;  // buffer_size must be a multiple of PAGE SIZE
	int chunk_size;

	int used_size;

}file_buffer_t;

file_buffer_t s_file_buffer;

static char * _file_mapto_mem(int fd, int mem_size, int file_offset)
{
	int result;
	char *mem;
	result = lseek(fd, mem_size - 1, SEEK_CUR);
	if (result == -1)
	{
		perror("Error calling lseek to stretch the file");
		return NULL;
	}
	result = write(fd, "", 1);
	if (result == -1)
	{
		perror("Error write the last byte of the file");
		return NULL;
	}
	mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, file_offset);
	if (mem == MAP_FAILED)
	{
		perror("Error mapping the file")
		return NULL;
	}
	return mem;
}

static int _mem_mapto_file(char *memory, int buffer_size)
{
	if (munmap(memory, buffer_size) == -1)
	{
		perror("Error calling munmap to the file")
		return -1;
	}
	return 0;
}


static int _GCD(int a, int b)
{
	if (a % b == 0)
		return b;
	return _GCD(b, a % b);
}

static int _LCM(int a, int b)
{
	return (a * b) / _GCD(a, b);
}

int file_buffer_int(const char *file_name, int buffer_size, int chunk_size)
{
	int page_size;
	int mask_size;
	s_file_buffer.fd = open(file_name, O_RDWR | O_CREATE | O_TRUNC, S_IRUSR | S_IWUSR);
	if (s_file_buffer.fd == -1)
	{
		perror("Error opening file for writing");
		return -1;
	}
	s_file_buffer.used_size   = 0;
	s_file_buffer.chunk_size  = chunk_size;

	// adjust buffer_size to buffer_size % PAGE_SIZE == 0 and buffer_size % chunk_size == 0
	page_size = sysconf(_SC_PAGE_SIZE);
	mask_size = _LCM(page_size, chunk_size) - 1;
	s_file_buffer.buffer_size = buffer_size & ~mask_size;

	s_file_buffer.memory = _file_mapto_mem(fd, s_file_buffer.buffer_size, 0);
	if (!s_file_buffer.memory) { close(s_file_buffer.fd); return -1;}
	s_file_buffer.file_size = s_file_buffer.buffer_size;
	return 0;
}

char * file_buffer_get_next_chunk()
{
	if (s_file_buffer.chunk_size + s_file_buffer.used_size > s_file_buffer.buffer_size)
	{
		int result;
		ASSERT(s_file_buffer.used_size == s_file_buffer.buffer_size, "run time error: file buffer system size is not logical");
		result = _mem_mapto_file(s_file_buffer.memory, s_file_buffer.buffer_size);
		if (result == -1) return NULL;
		s_file_buffer.memory = _file_mapto_mem(s_file_buffer.fd, s_file_buffer.buffer_size, s_file_buffer.file_size);
		if (!s_file_buffer.memory) { return NULL; }
		s_file_buffer.file_size += s_file_buffer.buffer_size;
		s_file_buffer.used_size = chunk_size;
		return s_file_buffer.memory;
	}
	else
	{
		char *chunk;
		chunk = s_file_buffer.memory + s_file_buffer.used_size;
		s_file_buffer.used_size += s_file_buffer.chunk_size;
		return chunk;
	}
}

void file_buffer_free()
{
	int left_size;
	if (s_file_buffer.memory)
		ASSERT(_mem_mapto_file(s_file_buffer.memory, s_file_buffer.buffer_size) != -1, "file buffer free failed on unmapping");
	left_size = s_file_buffer.buffer_size - s_file_buffer.used_size;
	if (left_size > 0)
	{ if (-1 == ftruncate(fd, s_file_buffer.file_size - left_size)) perror("Error calling ftruncate to change file size"); }
	close(s_file_bufer.fd);
}
