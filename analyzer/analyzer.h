#ifndef __ANALYZER_H__
#define __ANALYZER_H__

#include <unordered_map>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "common.h"
#include "elprof_logdata.h"

using std::unordered_map;

struct Hash
{
	size_t operator()(const char *s) const
	{
		return fnv_32a_str(s, FNV1_32A_INIT); 
	}
};

struct Equal
{
	bool operator()(const char *left, const char *right) const
	{
		return !strcmp(left, right);
	}
};

struct LogChunk
{
	char *chunk;
	int chunkSize;
};

class LogAnalyzer
{
	unordered_map<const char *, log_RECORD *, Hash, Equal> m_logRecords; 
	typedef unordered_map<const char *, log_RECORD *, Hash, Equal> _MAP_TYPE;
	typedef _MAP_TYPE::iterator _MAP_IT_TYPE;

public:
	LogAnalyzer();
	~LogAnalyzer();

	void doAnalyzing(const LogChunk *logChunk);
};

#endif
