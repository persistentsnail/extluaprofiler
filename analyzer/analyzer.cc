#include "analyzer.h"


LogAnalyzer::LogAnalyzer()
{
	memset(&m_eof, -1, sizeof(m_eof));
}

LogAnalyzer::~LogAnalyzer()
{
	vector<log_RECORD *>::iterator itRec;
	for (itRec = m_allRecs.begin(); itRec != m_allRecs.end(); itRec++)
		delete *itRec;
}

bool LogAnalyzer::doAnalyzing(const LogChunk *logChunk)
{
	int nLeft = logChunk->chunkSize;
	log_RECORD *cur = (log_RECORD *)logChunk->chunk;
	while (nLeft >= sizeof(log_RECORD))
	{
		const char *key = cur->source;
		if (cur->source[0] == 0)
		{
			DBG(printf("empty record source stop ananlyzing!\n"));
			return false;
		}

		_MAP_IT_TYPE dstIt = m_hashRecs.find(key);
		if (dstIt == m_hashRecs.end())
		{
			log_RECORD *one = new log_RECORD;
			*one = *cur;
			m_allRecs.push_back(one);
			m_hashRecs[key] = one;
		}
		else
		{
			dstIt->second->call_times += cur->call_times;
			dstIt->second->local_time += cur->local_time;
			dstIt->second->total_time += cur->total_time;
		}
		cur++;
		nLeft -= sizeof(log_RECORD);
	}
#ifdef DEBUG
	if (nLeft > 0)
		printf("log Records data is not complete!\n");
#endif
	return true;
}

vector<log_RECORD *> & LogAnalyzer::allRecords()
{
	return m_allRecs;
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

