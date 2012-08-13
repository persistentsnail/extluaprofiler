#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "elprof_logdata.h"
#include "file_buffer.h"
#include "common.h"

#define FILE_BUFFER_SIZE   (1 << 20)

enum {__h_max_entrys = 4};

typedef struct _entry_t
{
	log_RECORD record;
	unsigned int hash_val;
}entry_t;

typedef struct _bucket_t
{
	int entry_num;
	entry_t entrys[__h_max_entrys]; 
}bucket_t;

static bucket_t *s_hash;
static int nbuckets;

int log_RECORD_pool_init(int size, const char *log_filename)
{

	if (file_buffer_init(log_filename, FILE_BUFFER_SIZE, sizeof(log_RECORD)) == -1)
		return -1;
	s_hash = (bucket_t *)malloc(sizeof(bucket_t) * size);
	memset(s_hash, 0, sizeof(bucket_t) * size);
	nbuckets = size;
	return 0;
}

void log_RECORD_pool_free()
{
	int i;
	int j;
	bucket_t *p;
	char *file_buffer_chunk;
	printf("entry into free\n");
	for (i = 0; i < nbuckets; i++)
	{
		p = &s_hash[i];
		ASSERT(p->entry_num <= __h_max_entrys, "runtime error : error size entry");
		for (j = 0; j < p->entry_num; i++)
		{
			file_buffer_chunk = file_buffer_get_next_chunk();
			memcpy(file_buffer_chunk, &p->entrys[j].record, sizeof(log_RECORD));
		}
	}
	printf("will out pool free\n");
	free(s_hash);
	file_buffer_free();
}

/**************************************************************
	 string FNV hash algorithm
	 from http://isthe.com/chongo/tech/comp/fnv/
 *************************************************************/
#define FNV1_32A_INIT (0x811c9dc5)
static unsigned int s_hval = FNV1_32A_INIT;
static unsigned int
fnv_32a_str(char *str, unsigned int hval)
{
    unsigned char *s = (unsigned char *)str;	/* unsigned string */
    /*
     * FNV-1a hash each octet in the buffer
     */
    while (*s) {
	/* xor the bottom with the current octet */
	hval ^= (unsigned int)*s++;
	/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
	hval *= FNV_32_PRIME;
#else
	hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
    }
    /* return our new hash value */
    return hval;
}

void log_RECORD_pool_add(char *source, float local_time, float total_time)
{
	int i;
	unsigned int hash_val;
	bucket_t *bucket;
	entry_t *dst_entry;
	char *file_buffer_chunk;

	/* hash source string , get dest entry by hash key */
	hash_val = fnv_32a_str(source, s_hval);
	bucket = &s_hash[hash_val % nbuckets];

	dst_entry = NULL;
	for (i = 0; i < bucket->entry_num; i++)
	{
		if (hash_val == bucket->entrys[i].hash_val &&
			!strcmp(source, bucket->entrys[i].record.source))
		{
			dst_entry = &bucket->entrys[i];
			dst_entry->record.call_times++;
			break;
		}
	}

	if (!dst_entry)
	{
		if (bucket->entry_num < __h_max_entrys)
			dst_entry = &bucket->entrys[bucket->entry_num++];
		else
		{
			dst_entry = &bucket->entrys[0];
			for (i = 1; i < bucket->entry_num; i++)
			{
				if (bucket->entrys[i].record.call_times < dst_entry->record.call_times)
					dst_entry = &bucket->entrys[i];
			}
			/* dump the less call times record to file buffer */
			file_buffer_chunk = file_buffer_get_next_chunk();
			memcpy(file_buffer_chunk, &dst_entry->record, sizeof(dst_entry->record));
		}
		strcpy(dst_entry->record.source, source);
		dst_entry->record.local_time = local_time;
		dst_entry->record.total_time = total_time;
		dst_entry->record.call_times = 1;
		dst_entry->hash_val = hash_val;
	}
}
