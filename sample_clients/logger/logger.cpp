#include "logger.h"

#include <Entry.h>

#include <libim/Constants.h>
#include <libim/Contact.h>

using namespace IM;

int main(void)
{
	LoggerApp app;
	
	app.Run();
	
	return 0;
}

LoggerApp::LoggerApp()
:	BApplication("application/x-vnd.m_eiman.im_logger")
{
	fFile = fopen("IM log.txt", "a+");
	
	fMan = new IM::Manager(this);
	
	fMan->StartListening();
}

LoggerApp::~LoggerApp()
{
	fclose(fFile);
	
	fMan->Lock();
	fMan->Quit();
}

void
LoggerApp::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case 'newc':
		case IM::MESSAGE:
		{
			entry_ref ref;
			
			if ( msg->FindRef("contact",&ref) != B_OK )
			{
				return;
			}
			
			Contact c(&ref);
			
			int32 im_what=-1;
			
			msg->FindInt32("im_what",&im_what);
			
			char name[512];
			char nickname[512];
			
			if ( c.GetName(name,sizeof(name)) != B_OK )
				strcpy(name,"unknown contact");
			
			if ( c.GetNickname(nickname,sizeof(nickname)) != B_OK )
				strcpy(name,"unknown nick");
			
			char timestr[64];
			time_t now = time(NULL);
			strftime(timestr,sizeof(timestr),"[%H:%M] ", localtime(&now) );
			
			switch ( im_what )
			{
				case IM::MESSAGE_SENT:
				{
					fprintf(fFile,"%s You say to %s (%s): %s\n", timestr, name, nickname, msg->FindString("message"));
					fflush(fFile);
				}	break;
				
				case IM::MESSAGE_RECEIVED:
				{
					fprintf(fFile,"%s %s (%s) says: %s\n", timestr, name, nickname, msg->FindString("message"));
					fflush(fFile);
				}	break;
				
				default:
					break;
			}
		}	break;
		default:
			BApplication::MessageReceived(msg);
	}
}
