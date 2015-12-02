#include <stdio.h>
#include "lib/ArrayObfs.h"
/*
#include "MetaRandom.h"

#define RandAdd( A, I ) { unsigned int TEMP = andrivet::ADVobfuscator::MetaRandom<__Counter__, 0x8000000>::value; A += TEMP; A += i; A -= TEMP; }
#define Inc( A, I )	{ unsigned int i = I; while ( i-- ) A++; }
#define Split( A, I ) { unsigned int base = (unsigned int)A & 0xffff; \
			unsigned int off = (unsigned int)A >> 16; \
			A = &((char *)off[base]); \
			A += I; }
	


void __attribute__((always_inline)) Obfs_1(void *a, unsigned int i = 1)
{char *b = (char*)a; printf("Inc==>"); Inc( b, i); printf("%d\n", b[0]); }

void __attribute__((always_inline)) Obfs_2(void *a, unsigned int i = 1)
{char *b = (char*)a; printf("RandAdd==>"); RandAdd( b, i); printf("%d\n", b[0]); }

void __attribute__((always_inline)) Obfs_3(void *a, unsigned int i = 1)
{char *b = (char*)a; printf("Split==>"); Split( b, i); printf("%d\n", b[0]); }

#define Dispatch( A, I, R ) \
	Obfs_##R( A, I );


*/
int fun(int i)
{
	return i+1;
}

int main()
{
	char a[10] = {'a','b','c','d','e','f','g','o','p','q'};
	int b[10] = {1,2,3,4,5,6,7,8,9,0};
	printf("%d\n", Get(b, fun(3))[0]);
	return 0;
}
