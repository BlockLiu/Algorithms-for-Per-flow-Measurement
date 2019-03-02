#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../common/BOBHash32.h"


class CodedBloomFilter
{
	int s;		// number of sets
	int n;		// number of bloom filters
	int m;		// number of bits used in each bloom filter
	int k;		// number of hash functions used in each bloom filter
	BloomFilter** bfs;

public:
	CodedBloomFilter(int s, int m, int k):s(s), m(m), k(k)
	{
		n = 0;
		int tmpS = s;
		while(tmpS != 0){
			n++;
			tmpS >>= 1;
		}

		bfs = new BloomFilter*[n];
		for(int i = 0; i < n; ++i)
			bfs[i] = new BloomFilter(m, 0, k);
	}
	~CodedBloomFilter()
	{
		for(int i = 0; i < n; ++i)
			delete bfs[i];
		delete bfs;
	}

	void insert(char* key, uint32_t keylen, int setIdx)
	{
		for(int i = 0; i < n; ++i){
			if(setIdx & 1 == 1)
				bfs[i]->insert(key, keylen);
			setIdx >>= 1;
		}
	}

	/*
	 * return -1: not found in any set
	 * return -2: found in multiple sets
	 * return k (0<=k<s): found in the k-th set
	 */
	int query(char* key, uint32_t keylen)
	{
		int setId = 0;
		for(int i = 0; i < n; ++i)
			if(bfs[i]->query(key, keylen))
				setId += (1 << i);
		if(setId == 0)
			return -1;
		if(setId > s)
			return -2;
		return setId;
	}
}