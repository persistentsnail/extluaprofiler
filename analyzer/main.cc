
#include <iostream>
#include <vector>
#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "analyzer.h"

#define INIT_BUF_SIZE (1 << 20)
#define FMT_LOG_FILE "profiler.out"

using std::vector;


class LogReader
{
	int m_fd;
	int m_offset;
	int m_fileSize;

	char *m_buffer;
	int m_bufSize;

	vector<LogChunk> m_logChunks;
public:
	LogReader():m_fd(-1) {}
	~LogReader() {}

	bool initialize(const char *rawLogFile, int bufSize);
	void finialize();

	LogChunk *doReading();
	const vector<LogChunk> &logChunksInst() const;
};

class LogWriter
{
	FILE *m_fileH;
public:
	LogWriter():m_fileH(NULL) {}
	~LogWriter() {}
	bool initialize(const char *fmtLogFile);
	void finialize();

	void doWriting(const vector<LogChunk> &rawLogChunks);
};

bool LogReader::initialize(const char *rawLogFile, int bufSize)
{
	m_fd = open(rawLogFile, O_RDONLY);
	if (m_fd == -1)
	{
		perror("Error opening raw log file");
		return false;
	}
	struct stat stat_;
	if (fstat(m_fd, &stat_) == -1)
	{
		perror("Error calling fstat");
		close(m_fd);
		return false;
	}
	m_fileSize = stat_.st_size;
	m_offset = 0;

	int maskSize = sysconf(_SC_PAGE_SIZE) - 1;
	m_bufSize = bufSize & ~maskSize;
	return true;
}

#ifdef DEBUG
static size_t s_nread = 0;
#endif

LogChunk *LogReader::doReading(void)
{
	if (m_offset < m_fileSize)
	{
		if (m_offset + m_bufSize > m_fileSize)
			m_bufSize = m_fileSize - m_offset;
		m_buffer = (char *)mmap(NULL, m_bufSize, PROT_READ, MAP_SHARED, m_fd, m_offset);
		if (m_buffer == MAP_FAILED)
		{
			perror("Error calling mmap");
			return 0;
		}
		m_offset += m_bufSize;

		LogChunk lc;
		lc.chunkSize = m_bufSize;
		lc.chunk 	 = m_buffer;
		m_logChunks.push_back(lc);
		#ifdef DEBUG
		s_nread += m_bufSize;
		#endif
		return &m_logChunks.back();
	}
	else
		return NULL;
}

const vector<LogChunk> &LogReader::logChunksInst() const
{
	return m_logChunks;
}

void LogReader::finialize()
{
	vector<LogChunk>::iterator it;
	for (it = m_logChunks.begin(); it != m_logChunks.end(); it++)
	{
		if (munmap(it->chunk, it->chunkSize) == -1)
			perror("Error calling munmap");
	}
	if (m_fd != -1)
		close(m_fd);
}

typedef log_RECORD *PLOG_RECORD;
struct CallTimesComp
{
	bool operator ()(const PLOG_RECORD &left, const PLOG_RECORD &right) const
	{
		return left->call_times > right->call_times;
	}
};

struct LocalTimeComp
{
	bool operator ()(const PLOG_RECORD &left, const PLOG_RECORD &right) const
	{
		return left->local_time > right->local_time;
	}
};

struct TotalTimeComp
{	
	bool operator ()(const PLOG_RECORD &left, const PLOG_RECORD &right) const
	{
		return left->total_time > right->total_time;
	}
};

bool LogWriter::initialize(const char *fmtLogFile)
{
	m_fileH = fopen(fmtLogFile, "w");
	if (m_fileH == NULL)
	{	
		fprintf(stderr, "Error calling open format log file");
		return false;
	}
	return true;
}

void LogWriter::finialize()
{
	fclose(m_fileH);
}

void LogWriter::doWriting(const vector<LogChunk> &rawLogChunks)
{
	vector<log_RECORD *> allRecs;
	vector<LogChunk>::const_iterator chunkIt;
	for (chunkIt = rawLogChunks.begin(); chunkIt != rawLogChunks.end(); chunkIt++)
	{
		int nAnalyzed;
		log_RECORD *cur = (log_RECORD *)chunkIt->chunk;
		for (nAnalyzed = 0; nAnalyzed < chunkIt->chunkSize; 
				nAnalyzed += sizeof(log_RECORD))
		{
			allRecs.push_back(cur);
			cur++;
		}
	}
	std::sort(allRecs.begin(), allRecs.end(), LocalTimeComp());
	vector<PLOG_RECORD>::iterator recordIt;
	for (recordIt = allRecs.begin(); recordIt != allRecs.end(); recordIt++)
	{
		fprintf(m_fileH, "%s\t:\t%f\t%f\t%d\n", (*recordIt)->source, (*recordIt)->local_time,(*recordIt)->total_time, (*recordIt)->call_times);
	}
}

int Running(const char *rawLogFile, const char *fmtLogFile)
{
	LogReader reader;
	LogAnalyzer analyzer;
	LogWriter writer;

	if(!reader.initialize(rawLogFile, INIT_BUF_SIZE))
		return -1;
	if(!writer.initialize(fmtLogFile))
		return -1;
	LogChunk *chunk = NULL;
	while (chunk = reader.doReading())
		analyzer.doAnalyzing(chunk);

	writer.doWriting(reader.logChunksInst());
	reader.finialize();
	writer.finialize();
}

int main(int argc, char *argv[])
{
	Running(argv[1], FMT_LOG_FILE); 
	return 0;
}
