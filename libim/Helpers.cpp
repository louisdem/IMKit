#include "Helpers.h"

VERBOSITY_LEVEL g_verbosity_level = DEBUG;

#if 0
// Use this to add unlimited buffer size to LOG()


 To allocate a sufficiently large string and print into  it
       (code correct for both glibc 2.0 and glibc 2.1):
              #include <stdio.h>
              #include <stdlib.h>
              #include <stdarg.h>
              char *
              make_message(const char *fmt, ...) {
                 /* Guess we need no more than 100 bytes. */
                 int n, size = 100;
                 char *p;
                 va_list ap;
                 if ((p = malloc (size)) == NULL)
                    return NULL;
                 while (1) {
                    /* Try to print in the allocated space. */
                    va_start(ap, fmt);
                    n = vsnprintf (p, size, fmt, ap);
                    va_end(ap);
                    /* If that worked, return the string. */
                    if (n > -1 && n < size)
                       return p;
                    /* Else try again with more space. */
                    if (n > -1)    /* glibc 2.1 */
                       size = n+1; /* precisely what is needed */
                    else           /* glibc 2.0 */
                       size *= 2;  /* twice the old size */
                    if ((p = realloc (p, size)) == NULL)
                       return NULL;
                 }
              }
#endif


// Note: if you change something in this LOG,
// make sure to change the LOG below as the code
// unfortunately isn't shared. :/
void LOG(const char * module, VERBOSITY_LEVEL level, const char *message, const BMessage *msg, ...) {
	va_list varg;
	va_start(varg, msg);
	char buffer[2048];
	vsprintf(buffer, message, varg);
	
	if ( level < g_verbosity_level )
		return;
	
	char timestr[64];
	time_t now = time(NULL);
	strftime(timestr,sizeof(timestr),"%F %H:%M", localtime(&now) );
	
	printf("%s %s: %s\n", module, timestr, buffer);
	if ( msg )
	{
		printf("BMessage for last message:\n");
		msg->PrintToStream();
	}
}

void LOG(const char * module, VERBOSITY_LEVEL level, const char *message, ...) {
	va_list varg;
	va_start(varg, message);
	char buffer[2048];
	vsprintf(buffer, message, varg);

	if ( level < g_verbosity_level )
		return;
	
	char timestr[64];
	time_t now = time(NULL);
	strftime(timestr,sizeof(timestr),"%F %H:%M", localtime(&now) );
	
	printf("%s %s: %s\n", module, timestr, buffer);
}
