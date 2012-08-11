#include <stdio.h>
#include <string.h>

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
