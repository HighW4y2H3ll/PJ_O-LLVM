#include "../include/Random.h"
#include <stdio.h>

int *offs;

int RandSign()
{
	return rand_num_mod(2);
}

void OffGen( int idx, int len )
{
	int i = 0;
	for ( ; i < len ; i++ )
		idx -= ( offs[i] = RandSign() ? rand_num_mod(0x800) : (-rand_num_mod(0x800)) );
	offs[i] = idx;
}

void KeyGen( int len )
{
	FILE *fd = fopen("../Key.h","w");
	for ( int i = 0; i < len+1; i++ )
		fprintf( fd, "Vector( a, %d, nType<%d>() );\n", offs[i], offs[i]>0 ? rand_num_mod(3) : (rand_num_mod(2)+1) );
	fclose(fd);
}

int main( int argc, char ** argv)
{
	int obfus_num;
	int index_num;

	if ( argc < 3 )
		return -1;
	obfus_num = atoi(argv[2]);
	index_num = atoi(argv[1]);

	offs = new int[obfus_num];
	OffGen( index_num, obfus_num );
	KeyGen( obfus_num );
	delete offs;
	return 0;
}
