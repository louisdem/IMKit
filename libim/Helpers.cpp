#include "Helpers.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <Directory.h>
#include <Node.h>
#include <Entry.h>

log_importance g_verbosity_level = liDebug;

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

bool _has_checked_for_tty = false;

void
check_for_tty()
{
	if ( _has_checked_for_tty )
		return;
	
	_has_checked_for_tty = true;
	
	if ( !isatty(STDOUT_FILENO) )
	{ // redirect output to ~/im_kit.log if not run from Terminal
		close(STDOUT_FILENO);
		open("/boot/home/im_kit.log", O_WRONLY|O_CREAT|O_APPEND|O_TEXT);
		chmod("/boot/hmoe/im_kit.log", 0x600 );
	}
}


// Note: if you change something in this LOG,
// make sure to change the LOG below as the code
// unfortunately isn't shared. :/
void LOG(const char * module, log_importance level, const char *message, const BMessage *msg, ...) {
	va_list varg;
	va_start(varg, msg);
	char buffer[2048];
	vsprintf(buffer, message, varg);
	
	if ( level < g_verbosity_level )
		return;
	
	check_for_tty();
	
	char timestr[64];
	time_t now = time(NULL);
	strftime(timestr,sizeof(timestr),"%F %H:%M", localtime(&now) );
	
	printf("%s %s: %s\n", module, timestr, buffer);
	if ( msg )
	{
		printf("BMessage for last message:\n");
		msg->PrintToStream();
	}
	
	fsync(STDOUT_FILENO);
}

void LOG(const char * module, log_importance level, const char *message, ...) {
	va_list varg;
	va_start(varg, message);
	char buffer[2048];
	vsprintf(buffer, message, varg);

	if ( level < g_verbosity_level )
		return;
	
	check_for_tty();
	
	char timestr[64];
	time_t now = time(NULL);
	strftime(timestr,sizeof(timestr),"%F %H:%M", localtime(&now) );
	
	printf("%s %s: %s\n", module, timestr, buffer);
	
	fsync(STDOUT_FILENO);
}

// READ / WRITE ATTRIBUTES

status_t
im_load_attribute( const char * path, const char * attribute, BMessage * msg )
{
	char data[1024*1024];
	
	BNode node( path );
	
	if ( node.InitCheck() != B_OK )
	{
		LOG("helpers", liLow, "load_attribute: Error opening file (%s)", attribute, path);
		return B_ERROR;
	}
	
	int32 num_read = node.ReadAttr(
		attribute, B_RAW_TYPE, 0,
		data, sizeof(data)
	);
	
	if ( num_read <= 0 )
	{
		LOG("helpers", liLow, "load_attribute: Error reading (%s) from (%s)", attribute, path);
		return B_ERROR;
	}
	
	if ( msg->Unflatten(data) != B_OK )
	{
		LOG("helpers", liHigh, "ERROR: load_attribute: Error unflattening (%s) from (%s)", attribute, path);
		return B_ERROR;
	}
	
	LOG("helpers", liDebug, "Read (%s) from (%s)", attribute, path);
	
	return B_OK;
}

status_t
im_save_attribute( const char * path, const char * attribute, const BMessage * msg )
{
	// save settings
	char data[1024*1024];
	int32 data_size=msg->FlattenedSize();
	
	if ( msg->Flatten(data,data_size) != B_OK )
	{ // error flattening message
		LOG("helpers", liHigh, "ERROR: save_attribute: Error flattening (%s) message for (%s)", attribute, path);
		return B_ERROR;
	}
	
	BDirectory dir;
	dir.CreateFile(path,NULL,true);
	
	BNode node( path );
	
	if ( node.InitCheck() != B_OK )
	{
		LOG("helpers", liHigh, "ERROR: save_attribute: Error opening save file (%s):(%s)", attribute, path);
		return B_ERROR;
	}
	
	int32 num_written = node.WriteAttr(
		attribute, B_RAW_TYPE, 0,
		data, data_size
	);
	
	if ( num_written != data_size )
	{ // error saving settings
		LOG("helpers", liHigh, "ERROR: save_attribute: Error saving (%s) to (%s)", attribute, path);
		return B_ERROR;
	}
	
	LOG("helpers", liDebug, "Wrote (%s) to file: %s", attribute, path);
	
	return B_OK;
}

// LOAD SETTINGS

status_t
im_load_settings( const char * path, BMessage * msg )
{
	return im_load_attribute( path, "im_settings", msg );
}

status_t
im_load_template( const char * path, BMessage * msg )
{
	return im_load_attribute( path, "im_template", msg );
}

status_t
im_load_protocol_settings( const char * protocol, BMessage * msg )
{
	char settings_path[512];
	
	// get path to settings file
	sprintf(
		settings_path,
		"/boot/home/config/settings/im_kit/add-ons/protocols/%s",
		protocol
	);
	
	return im_load_settings( settings_path, msg );
}

status_t
im_load_protocol_template( const char * protocol, BMessage * msg )
{
	char settings_path[512];
	
	// get path to settings file
	sprintf(
		settings_path,
		"/boot/home/config/settings/im_kit/add-ons/protocols/%s",
		protocol
	);
	
	return im_load_template( settings_path, msg );
}

status_t
im_load_client_settings( const char * protocol, BMessage * msg )
{
	char settings_path[512];
	
	// get path to settings file
	sprintf(
		settings_path,
		"/boot/home/config/settings/im_kit/clients/%s",
		protocol
	);
	
	return im_load_settings( settings_path, msg );
}

status_t
im_load_client_template( const char * protocol, BMessage * msg )
{
	char settings_path[512];
	
	// get path to settings file
	sprintf(
		settings_path,
		"/boot/home/config/settings/im_kit/clients/%s",
		protocol
	);
	
	return im_load_template( settings_path, msg );
}

// SAVE SETTINGS

status_t
im_save_settings( const char * path, const BMessage * settings )
{
	return im_save_attribute( path, "im_settings", settings );
}

status_t
im_save_template( const char * path, const BMessage * settings )
{
	return im_save_attribute( path, "im_template", settings );
}

status_t
im_save_protocol_settings( const char * protocol, const BMessage * settings )
{
	char settings_path[512];
	
	sprintf(
		settings_path,
		"/boot/home/config/settings/im_kit/add-ons/protocols/%s",
		protocol
	);
	
	return im_save_settings( settings_path, settings );
}

status_t
im_save_protocol_template( const char * protocol, const BMessage * settings )
{
	char settings_path[512];
	
	sprintf(
		settings_path,
		"/boot/home/config/settings/im_kit/add-ons/protocols/%s",
		protocol
	);
	
	return im_save_template( settings_path, settings );
}

status_t
im_save_client_settings( const char * client, const BMessage * settings )
{
	char settings_path[512];
	
	sprintf(
		settings_path,
		"/boot/home/config/settings/im_kit/clients/%s",
		client
	);
	
	return im_save_settings( settings_path, settings );
}

status_t
im_save_client_template( const char * client, const BMessage * settings )
{
	char settings_path[512];
	
	sprintf(
		settings_path,
		"/boot/home/config/settings/im_kit/clients/%s",
		client
	);
	
	return im_save_template( settings_path, settings );
}

// CLIENT / PROTOCOL LIST

void
im_get_file_list( const char * path, const char * msg_field, BMessage * msg )
{
	BDirectory dir(path);
	
	entry_ref ref;
	
	while ( dir.GetNextRef(&ref) == B_OK )
	{
		msg->AddString(msg_field, ref.name );
	}
}

void
im_get_protocol_list( BMessage * list )
{
	return im_get_file_list("/boot/home/config/settings/im_kit/add-ons/protocols", "protocol", list);
}

void
im_get_client_list( BMessage * list )
{
	return im_get_file_list("/boot/home/config/settings/im_kit/clients", "client", list);
}
