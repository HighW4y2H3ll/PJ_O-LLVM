#include <stdio.h>

char **FileLIst = NULL;
int KeyCnt = 0;

////////////////////////////////
int testFileExist();

void GetFiles( int , char ** );

///////////////////////////////
int IsTypeDefed( char * );

int IsType( char * );

int IsDef( char * );

int Calcable( char * );

void ParseFile( char * );

///////////////////////////////
// Gen File : Key_##Num.h
int GenKeyConf();

// Insert :
// 	#define KEY_CONF_##Num
void ModFile( char * );

int main( int argc, char **argv )
{

	return 0;
}
