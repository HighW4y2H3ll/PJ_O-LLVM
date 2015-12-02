#ifndef RANDOM_H
#define RANDOM_H

#include <time.h>
#include <stdlib.h>

int __attribute__((always_inline)) rand_num();

int __attribute__((always_inline)) rand_num_mod( int N );

#endif
