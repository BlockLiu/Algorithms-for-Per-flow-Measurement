#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../common/BOBHash32.h"

class CuckooFilter
{
	const static int MAX_RELOCATE_TIMES = 100;
	int m;		// number of buckets in the array
				// m must be 2^k
	int w;		// number of entries in the bucket
	uint32_t** array;
	BOBHash32* hash[2];

	int kick_index, kick_bucket;

public:
	CuckooFilter(int m, int w): m(m), w(w)
	{
		array = new uint32_t*[m];
		for(int i = 0; i < m; ++i){
			array[i] = new uint32_t[w];
			memset(array[i], 0, sizeof(uint32_t) * w);
		}

		hash[0] = new BOBHash32(101);
		hash[1] = new BOBHash32[102];

		kick_bucket = 0;
		kick_index = 0;
	}
	~CuckooFilter(){
		for(int i = 0; i < m; ++i)
			delete array[i];
		delete array;
		delete hash[0];
		delete hash[1];
	}

	void insert(char* key, uint32_t keylen)
	{
		uint32_t fp = get_fp(key, keylen);
		uint32_t pos1 = hash[0]->run(key, keylen) % m;
		uint32_t pos2 = (pos1 ^ hash[0]->run((char*)&fp, sizeof(uint32_t))) % m;
		if(pos1 > pos2){
			tmp_pos = pos2;
			pos2 = pos1;
			pos1 = tmp_pos;

		}

		for(int j = 0; j < w; ++j)
			if(array[pos1][j] == fp)
				return;
		for(int j = 0; j < w; ++j)
			if(array[pos2][j] == fp)
				return;

		for(int j = 0; j < w; ++j)
			if(array[pos1][j] == 0){
				array[pos1][j] = fp;
				return;
			}
		for(int j = 0; j < w; ++j)
			if(array[pos2][j] == 0){
				array[pos2][j] = fp;
				return;
			}

		uint32_t bucket_no = kick_bucket == 0 ? pos1 : pos2;
		uint32_t kick_fp = array[bucket_no][kick_index];
		array[bucket_no][kick_index] = fp;
		kick_bucket = (kick_bucket + 1) % 2;
		kick_index = (kick_index + 1) % w;
		relocate(kick_fp, bucket_no);
	}

	bool query(char* key, uint32_t keylen)
	{
		uint32_t fp = get_fp(key, keylen);
		uint32_t pos1 = hash[0]->run(key, keylen) % m;
		uint32_t pos2 = (pos1 ^ hash[0]->run((char*)&fp, sizeof(uint32_t))) % m;

		for(int j = 0; j < w; ++j)
			if(array[pos1][j] == fp)
				return true;
		for(int j = 0; j < w; ++j)
			if(array[pos2][j] == fp)
				return true;
		return false;
	}

	void delete(char*key, uint32_t keylen)
	{
		uint32_t fp = get_fp(key, keylen);
		uint32_t pos1 = hash[0]->run(key, keylen) % m;
		uint32_t pos2 = (pos1 ^ hash[0]->run((char*)&fp, sizeof(uint32_t))) % m;

		for(int j = 0; j < w; ++j)
			if(array[pos1][j] == fp){
				array[pos1][j] = 0;
				return;
			}
		for(int j = 0; j < w; ++j)
			if(array[pos2][j] == fp){
				array[pos1][j] = 0;
				return;
			}
	}

private:
	/* calculate fingerprint */
	uint32_t get_fp(char* key, uint32_t keylen)
	{
		return hash[1]->run(key, keylen);
	}

	/* relocate finger print */
	void relocate(uint32_t fp, uint32_t bucket_no, int times = 0)
	{	
		if(times >= MAX_RELOCATE_TIMES){
			printf("relocate failed!\n");
			return;
		}

		int pos = (bucket_no ^ hash[0]->run((char*)&fp, sizeof(uint32_t))) % m;
		for(int j = 0; j < w; ++j)
			if(array[pos][j] == 0){
				array[pos][j] = fp;
				return;
			}

		uint32_t kick_fp = array[pos][kick_index];
		array[pos][kick_index] = fp;
		kick_index = (kick_index + 1) % w;
		relocate(kick_fp, pos, times++);
	}
}


















