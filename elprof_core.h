#ifndef __ELPROF_H__
#define __ELPROF_H__

#include "callstack.h"

int elprof_callhookIN(void *luaS, const char *func_name, const char *file, int linedefined);

int elprof_callhookOUT(void *luaS);

int elprof_core_init(void *luaS, const char *out_filename);
void elprof_core_finalize();

#endif
