
/*************************************************************
 *************************************************************
    FILE Buffer System
 *************************************************************
 ************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "file_buffer.h"
#include "common.h"

typedef struct _file_buffer_t
{
	int fd;
	int file_size;
	int chunk_size;

	char *memory;
	int buffer_size;  /* buffer_size must be a multiple of PAGE SIZE and chunk size*/
	int used_size;

}file_buffer_t;

static file_buffer_t s_fb;

static char * _file_mapto_mem(int fd, int mem_size, int file_offset)
{
	int result;
	char *mem;
	result = lseek(fd, mem_size - 1 + file_offset, SEEK_SET);
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
		perror("Error mapping the file");
		return NULL;
	}
	return mem;
}

static int _mem_mapto_file(char *memory, int buffer_size)
{
	if (munmap(memory, buffer_size) == -1)
	{
		perror("Error calling munmap to the file");
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

int file_buffer_init(const char *file_name, int buffer_size, int chunk_size)
{
	int page_size;
	int mask_size;
	if (buffer_size < chunk_size)
	{
		fprintf(stderr, "Error : the init size is not reasonable!");
		return -1;
	}

	s_fb.fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (s_fb.fd == -1)
	{
		perror("Error opening file for writing");
		return -1;
	}

	/* adjust buffer_size to buffer_size % PAGE_SIZE == 0 and buffer_size % chunk_size == 0 */
	page_size = sysconf(_SC_PAGE_SIZE);
	mask_size = _LCM(page_size, chunk_size);
	s_fb.buffer_size = (buffer_size / mask_size) * mask_size;

	/* mmap init memory */
	s_fb.memory = _file_mapto_mem(s_fb.fd, s_fb.buffer_size, 0);
	if (!s_fb.memory) 
	{ 
		close(s_fb.fd); 
		return -1;
	}

	/* init file buffer file size, chunk size, used size */
	s_fb.file_size = s_fb.buffer_size;
	s_fb.chunk_size  = chunk_size;
	s_fb.used_size   = 0;

	return 0;
}

char * file_buffer_get_next_chunk()
{
	if (s_fb.chunk_size + s_fb.used_size > s_fb.buffer_size)
		return NULL;
	else
	{
		char *chunk;
		chunk = s_fb.memory + s_fb.used_size;
		s_fb.used_size += s_fb.chunk_size;
		return chunk;
	}
}

void file_buffer_reset()
{
	int result;
	ASSERT(s_fb.used_size == s_fb.buffer_size, 
						"Run time error: File Buffer System size is not logical");
	DBG(printf("file size grow %x\n", s_fb.buffer_size));
	result = _mem_mapto_file(s_fb.memory, s_fb.buffer_size);

	s_fb.memory = _file_mapto_mem(s_fb.fd, 
										   s_fb.buffer_size, 
						                   s_fb.file_size);
	s_fb.file_size += s_fb.buffer_size;
	ASSERT(result != -1 && s_fb.memory, "Error calling file_buffer_reset");

	s_fb.used_size = 0;
}

void file_buffer_free()
{
	int left_size;
	if (s_fb.memory)
	{
		ASSERT(_mem_mapto_file(s_fb.memory, s_fb.buffer_size) != -1, 
			"File Buffer free failed on unmapping");
	}
	left_size = s_fb.buffer_size - s_fb.used_size;
	if (left_size > 0)
	{ 
		DBG(printf("file buffer free , left %d size bytes not written!\n", left_size));
		if (-1 == ftruncate(s_fb.fd, s_fb.file_size - left_size)) 
			perror("Error calling ftruncate to change file size");
	}

	close(s_fb.fd);
}
