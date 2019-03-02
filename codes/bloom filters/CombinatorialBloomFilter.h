#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../common/BOBHash32.h"

class CombinatorialBloomFilter
{
	int m;		// number of bits in the bit array
	int s;		// number of sets
	int w;		// number of bytes in the bit array
	int k;		// number of hash functions
	uint8_t* array;
	BOBHash32** hash;

public:
	CombinatorialBloomFilter(int m, int k, int s):m(m), k(k), s(s)
	{
		w = m / 8 + (m % 8 == 0 ？0 : 1);
		array = new uint8_t[w];
		memset(array, 0, w);

		hash = new BOBHash32*[s*k];
		for(int i = 0; i < s*k; ++i)
			hash[i] = new BOBHash32(100 + i);
	}
	~CombinatorialBloomFilter{
		delete array;
		for(int i = 0; i < s*k; ++i)
			delete hash[i];
		delete hash;
	}

	void insert(char* key, uint32_t keylen, int setIdx)
	{
		for(int i = 0; i < k; ++i){
			int pos = hash[setIdx*k + i]->run(key, keylen) & m;
			set_bit(pos);
		}
	}

	/*
	 * return -1: not found in any set
	 * return -2: found in multiple sets
	 * return k (0<=k<s): found in the k-th set
	 */
	int query(char* key, uint32_t keylen)
	{
		int setIdx = -1;
		for(int i = 0; i < s; ++i)
		{
			bool flag = true;
			for(int j = 0; j < k; ++j){
				int pos = hash[i*k + j]->run(key, keylen) % m;
				if(query_bit(pos) == 0){
					flag = false;
					break;
				}
			}
			if(flag){
				if(setIdx != -1)
					return -2;
				setIdx = i;
			}
		}
		return setIdx;
	}

private:
	int query_bit(int pos){
		int base = pos / 8;
		int offset = pos % 8;
		uint8_t mask = (uint8_t)(1 << offset);
		uint8_t res = array[base] & mask;
		return res ? 1 : 0;
	}
	void set_bit(int pos){
		int base = pos / 8;
		int offset = pos % 8;
		uint8_t mask = (uint8_t)(1 << offset);
		array[base] |= mask;
	}


}