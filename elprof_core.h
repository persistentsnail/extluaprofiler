#ifndef __ELPROF_H__
#define __ELPROF_H__

#include "callstack.h"

void elprof_callhookIN(elprof_STATE *S, const char *func_name, const char *file, int linedefined);

int elprof_callhookOUT(elprof_STATE *S);

elprof_STATE *elprof_core_init(const char *out_filename);
void elprof_core_finalize(elprof_STATE *S);

#endif
