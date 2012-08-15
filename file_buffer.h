#ifndef __FILE_BUFFER_H__
#define __FILE_BUFFER_H__

/**************************************************************
 **************************************************************
    FILE Buffer System
 **************************************************************
 *************************************************************/
int file_buffer_init(const char *file_name, int buffer_size, int chunk_size);
void file_buffer_free();

char * file_buffer_get_next_chunk();
void file_buffer_reset();


#endif
