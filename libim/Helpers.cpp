#include "Helpers.h"


// Note: if you change something in this LOG,
// make sure to change the LOG below as the code
// unfortunately isn't shared. :/
void LOG(const char *message, const BMessage *msg, ...) {
	va_list varg;
	va_start(varg, msg);
	char buffer[512];
	vsprintf(buffer, message, varg);

	char timestr[64];
	time_t now = time(NULL);
	strftime(timestr,sizeof(timestr),"%F %H:%M", localtime(&now) );
	
	printf("%s: %s\n", timestr, buffer);
	if ( msg )
	{
		printf("BMessage for last message:\n");
		msg->PrintToStream();
	}
}

void LOG(const char *message, ...) {
	va_list varg;
	va_start(varg, message);
	char buffer[512];
	vsprintf(buffer, message, varg);

	char timestr[64];
	time_t now = time(NULL);
	strftime(timestr,sizeof(timestr),"%F %H:%M", localtime(&now) );
	
	printf("%s: %s\n", timestr, buffer);
}
