#include "Helpers.h"

void
LOG( const char * message, const BMessage * msg )
{
	char timestr[64];
	time_t now = time(NULL);
	strftime(timestr,sizeof(timestr),"%F %H:%M", localtime(&now) );
	
	printf("%s: %s\n", timestr, message);
	if ( msg )
	{
		printf("BMessage for last message:\n");
		msg->PrintToStream();
	}
}

void
LOG( const char * message )
{
	LOG(message,NULL);
}

