#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../common/BOBHash32.h"

class BloomFilter
{
	int n;		// number of elements in the set
	int m;		// number of bits in the bit array
	int w;		// number of bytes in the bit array
	int k;		// number of hash functions
	uint8_t* array;
	BOBHash32** hash;

public:
	BloomFilter(int m, int n, int k): n(n), m(m)
	{
		k = k == 0 ? int(n * 1.0 / m * log(2)) : k;

		w = m / 8 + (m % 8 == 0 ？0 : 1);
		array = new uint8_t[w];
		memset(array, 0, w);

		hash = new BOBHash32*[k];
		for(int i = 0; i < k; ++i)
			hash[i] = new BOBHash32(100 + i);
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
			set_bit(pos);
		}
	}

	bool query(char* key, uint32_t keylen)
	{
		for(int i = 0; i < k; ++i){
			int pos = hash[i]->run(key, keylen) % m;
			if(query_bit(pos) == 0)
				return false;
		}
		return true;
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