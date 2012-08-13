#ifndef __BUFFER_H__
#define __BUFFER_H__

int buffer_init(int init_size, int limit_size);
void buffer_free();

char * buffer_get_whole_memory();
int buffer_get_used_size();

void buffer_reset();

char * buffer_get_next_chunk(int chunk_size);
void buffer_consume_memory_size(int used_size);

#endif
