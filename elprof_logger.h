#ifndef __ELPROF_LOGGER_H__
#define __ELPROF_LOGGER_H__

#include "callstack.h"

int elprof_logger_save(elprof_CALLSTACK_RECORD *savef);
int elprof_logger_init(const char *log_filename);

#endif
