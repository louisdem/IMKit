#ifndef HELPERS_H
#define HELPERS_H

#include <stdio.h>
#include <String.h>
#include <Message.h>

class Tracer
{
	private:
		BString str;
		
	public:
		Tracer( const char * _str ) 
		: str(_str) 
		{ 
			printf("%s\n",str.String()); 
		};
		~Tracer() 
		{ 
			printf("-%s\n",str.String() );
		};
};

extern void LOG( const char *, const BMessage *, ...);
extern void LOG( const char *, ...);

#endif
