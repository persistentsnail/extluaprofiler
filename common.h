#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef DEBUG
#define ASSERT(e, msg) if (!(e)) { int line = __LINE__;			\
    fprintf(stdout,                                     \
	    "file %s line %d : assertion failed: %s\n", \
	   __FILE__,                                  \
	   line,                                  \
	   msg);                                       \
    exit(1); }
#else
#define ASSERT(e, msg)
#endif


/**************************************************************
	 string FNV hash algorithm
	 from http://isthe.com/chongo/tech/comp/fnv/
 *************************************************************/
#define FNV1_32A_INIT (0x811c9dc5)
unsigned int fnv_32a_str(const char *str, unsigned int hval);

#endif
