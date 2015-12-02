#ifndef ARRAY_OBFS_H
#define ARRAY_OBFS_H

#include "ObfsHelper.h"

template<int v>
struct nType
{
	enum{ value = v };
};

template<typename A>
class ArrayObfs
{
private:

	int XX,YY;

	// Heavy Calc
	void  __attribute__((always_inline)) Vector ( A &a, int I, nType<0>)
	{
		Obfs_1
		while ( I-- )	a++;
	}

	// Trivial Calc
	void __attribute__((always_inline)) Vector ( A &a, int I, nType<1>)
	{
		Obfs_2
		int TEMP = (int)&Obfs_Helper;
		I += TEMP;
		a += I;
		a -= TEMP;
	}
	void  __attribute__((always_inline)) Vector ( A &a, int I, nType<2>)
	{
		Obfs_2
		Obfs_1
		unsigned int base = (unsigned int) a & 0xffff;
		unsigned int off = (unsigned int) a & ( 0xffff << 16);
		a = (A)(&((char *)off)[base]) + I;
	}
	void  __attribute__((always_inline)) Vector ( A &a, int I, nType<3>)
	{
	}
	
public:
	void __attribute__((always_inline)) Obfs ( A &a, int I )
	{
		//	K( V, N )
		//	   ^  ^
		//	   |  Offset Num
		//	   ---Vector Num
		//#ifdef K
		//#undef K
		//#define K(V,N)	Vector( a, N, nType<V>() );
		//#endif

		#include "../Key.h"

		//Vector( a, I, nType< 3 >() );
	}
};

template<typename X>
	X __attribute__((always_inline)) Get ( X a, int I )
	{
		ArrayObfs<X> x;
		x.Obfs( a, I );
		return a;
	};

#endif
