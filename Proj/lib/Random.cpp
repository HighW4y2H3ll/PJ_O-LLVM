#include "../include/Random.h"
#include "3rdParty/mtwist.h"

int __attribute__((always_inline)) rand_num()
{
	mt_prng r(true);
	return r.lrand();
}

int __attribute__((always_inline)) rand_num_mod( int N )
{
	int r = (rand_num() & 0x7fffffff)%N;
	return r;
}
