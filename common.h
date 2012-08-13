#ifndef __COMMON_H__
#define __COMMON_H__

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
