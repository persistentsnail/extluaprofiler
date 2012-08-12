#ifndef __BUFFER_H__
#define __BUFFER_H__

int buffer_init(int init_size, int limit_size);
void buffer_free();

char * buffer_get_whole_memory();
int buffer_get_used_size();

void buffer_reset();

char * buffer_get_next_chunk(int chunk_size);
void buffer_consume_memory_size(int used_size);

//*************************************************************
//*************************************************************
// FILE Buffer System
// ************************************************************
// ************************************************************
int file_buffer_init(const char *file_name, int buffer_size);
void file_buffer_free();

char * file_buffer_get_next_chunk();


#ifdef DEBUG
#define ASSERT(e, msg) if (!e) { int line = __LINE__;			\
    fprintf(stdout,                                     \
	    "file %s line %d : assertion failed: %s\n", \
	   __FILE__,                                  \
	   line,                                  \
	   msg);                                       \
    exit(1); }
#else
#define ASSERT(e, msg)
#endif


#endif
