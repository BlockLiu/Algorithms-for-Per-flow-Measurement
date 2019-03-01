#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../common/BOBHash32.h"

class BloomFilter
{
	int n;		// number of elements in the set
	int m;		// number of counters in the array
	int k;		// number of hash functions
	uint32_t* array;
	BOBHash32** hash;

public:
	BloomFilter(int m, int n, int k): n(n), m(m), k(k)
	{
		array = new uint32_t[m];
		memset(array, 0, m * sizeof(uint32_t));

		hash = new BOBHash32*[k];
		for(int i = 0; i < k; ++i)
			hash[i] = new BOBHash32(100 + i);
	}
	BloomFilter(int m, int n)
	{
		k = int(n * 1.0 / m * log(2));
		BloomFilter(m, n, k);
	}
	~BloomFilter(){
		delete array;
		for(int i = 0; i < k; ++i)
			delete hash[i];
		delete hash;
	}

	void insert(char* key, uint32_t keylen)
	{
		for(int i = 0; i < k; ++i){
			int pos = hash[i]->run(key, keylen) % m;
			array[pos] += 1;
		}
	}

	int query(char* key, uint32_t keylen)
	{
		int res = 0x3FFFFFFF;
		for(int i = 0; i < k; ++i){
			int pos = hash[i]->run(key, keylen) % m;
			res = res > array[pos] ? array[pos] : res;
		}
		return res;
	}
}