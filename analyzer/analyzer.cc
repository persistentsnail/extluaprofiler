#include "analyzer.h"

LogAnalyzer::LogAnalyzer()
{
}

LogAnalyzer::~LogAnalyzer()
{
}

void LogAnalyzer::doAnalyzing(const LogChunk *logChunk)
{
	int nAnalyzed;
	log_RECORD *cur = (log_RECORD *)logChunk->chunk;
	for (nAnalyzed = 0; nAnalyzed < logChunk->chunkSize; 
			nAnalyzed += sizeof(log_RECORD))
	{
		const char *key = cur->source;
		_MAP_IT_TYPE dstIt = m_logRecords.find(key);
		if (dstIt == m_logRecords.end())
			m_logRecords[key] = cur;
		else
		{
			dstIt->second->call_times += cur->call_times;
			dstIt->second->local_time += cur->local_time;
			dstIt->second->total_time += cur->total_time;
		}
		cur++;
	}
	ASSERT(nAnalyzed == logChunk->chunkSize, "Runtime error: raw log file is not logical");
}

/**************************************************************
	 string FNV hash algorithm
	 from http://isthe.com/chongo/tech/comp/fnv/
 *************************************************************/
unsigned int
fnv_32a_str(const char *str, unsigned int hval)
{
    const unsigned char *s = (const unsigned char *)str;	/* unsigned string */
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

