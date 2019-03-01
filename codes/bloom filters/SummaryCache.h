#include "BloomFilter.h"

class SummaryCache
{
	int s;		// number of sets
	int m;		// number of bits used in each bloom filter
	int k;		// number of hash functions used in each bloom filter
	BloomFilter** bfs;

public:
	SummaryCache(int s, int m, int k):s(s), m(m), k(k)
	{
		bfs = new BloomFilter*[s];
		for(int i = 0; i < s; ++i)
			bfs[i] = new BloomFilter(m, 0, k);
	}
	~SummaryCache()
	{
		for(int i = 0; i < s; ++i)
			delete bfs[i];
		delete bfs;
	}

	void insert(char* key, uint32_t keylen, int setIdx)
	{
		bfs[setIdx]->insert(key, keylen);
	}

	/*
	 * return -1: not found in any set
	 * return -2: found in multiple sets
	 * return k (0<=k<s): found in the k-th set
	 */
	int query(char* key, uint32_t keylen)
	{
		int res = -1;
		for(int i = 0; i < s; ++i)
			if(bfs[i]->query(key, keylen)){
				if(res != -1)
					return -2;
				res = i;
			}
		return res;
	}
}