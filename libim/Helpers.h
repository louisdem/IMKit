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

enum VERBOSITY_LEVEL {
	DEBUG = 0,		// debug
	HIGH = 1,		// lots and lots of messages
	MEDIUM = 2,		// enough to follow what's happening
	LOW = 3,		// general idea
	QUIET = 100		// quiet. Don't say a word.
};

extern void LOG( const char * module, VERBOSITY_LEVEL level, const char * msg, const BMessage *, ...);
extern void LOG( const char * module, VERBOSITY_LEVEL level, const char * msg, ...);

// if level < g_verbosity_level then LOG() doesn't print the msg
extern VERBOSITY_LEVEL g_verbosity_level;

#endif
