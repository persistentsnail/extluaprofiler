
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

public:
	LogReader():m_fd(-1), m_buffer(NULL) {}
	~LogReader() {}

	bool initialize(const char *rawLogFile, int bufSize);
	void finialize();

	LogChunk *doReading();
};

class LogWriter
{
	FILE *m_fileH;
public:
	LogWriter():m_fileH(NULL) {}
	~LogWriter() {}
	bool initialize(const char *fmtLogFile);
	void finialize();

	void doWriting(vector<log_RECORD *> &);
};

static int _GCD(int a, int b)
{
	if (a % b == 0)
		return b;
	return _GCD(b, a % b);
}

static int _LCM(int a, int b)
{
	return (a * b) / _GCD(a, b);
}

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

	int pageSize = sysconf(_SC_PAGE_SIZE);
	int mult = _LCM(pageSize, sizeof(log_RECORD));
	m_bufSize = (bufSize / mult) * mult;
	m_buffer = NULL;
	return true;
}

#ifdef DEBUG
static size_t s_nread = 0;
#endif

LogChunk *LogReader::doReading(void)
{
	if (m_buffer)
	{
		if (munmap(m_buffer, m_bufSize) == -1)
			perror("Error calling munmap");
		m_buffer = NULL;
	}
	
	if (m_offset < m_fileSize)
	{
		if (m_offset + m_bufSize > m_fileSize)
			m_bufSize = m_fileSize - m_offset;
		if (m_bufSize < sizeof(log_RECORD))
			return NULL;
		m_buffer = (char *)mmap(NULL, m_bufSize, PROT_READ, MAP_SHARED, m_fd, m_offset);
		if (m_buffer == MAP_FAILED)
		{
			perror("Error calling mmap");
			return 0;
		}
		m_offset += m_bufSize;

		static LogChunk lc;
		lc.chunkSize = m_bufSize;
		lc.chunk 	 = m_buffer;
		return &lc;
	}
	else
		return NULL;
}

void LogReader::finialize()
{
	if (m_buffer)
	{
		if(munmap(m_buffer, m_bufSize) == -1)
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
	if (m_fileH)
		fclose(m_fileH);
}

void LogWriter::doWriting(vector<log_RECORD *> &allRecs)
{
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
	while ((chunk = reader.doReading()) && analyzer.doAnalyzing(chunk));

	writer.doWriting(analyzer.allRecords());
	reader.finialize();
	writer.finialize();
}

int main(int argc, char *argv[])
{
	Running(argv[1], FMT_LOG_FILE); 
	return 0;
}
