int Obfs_Helper()
{
	return (int)(&Obfs_Helper)==0x12345;
}

#define Obfs_1	XX = 1923457;	\
                while ( Obfs_Helper() == (XX/2)*(XX/2) )	\
                        while(1)	\
                        { a += (int)a^I-0x80000; return; }

#define Obfs_2 	YY = (int)&XX;	\
		XX = (int)&Obfs_Helper;	\
		while ( (YY*YY*YY - 1) == (XX|342738) )	\
			if ( ((XX|2) << 16) == YY*XX )	\
				Obfs_Helper();		\
			else				\
				return;
