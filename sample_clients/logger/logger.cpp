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
	fMan = new IM::Manager(this);
	
	fMan->StartListening();

	BMessage msg(IM::ADD_AUTOSTART_APPSIG);
	msg.AddString("app_sig", "application/x-vnd.m_eiman.im_logger");
	
	fMan->SendMessage( &msg );


	// create ./Logs directory
	create_directory("/boot/home/Logs/IM", 0777);
/*
	fMaxFiles = 10;

	fFiles = new BFile *[fMaxFiles];
	if (fFiles == NULL) BMessenger(be_app).SendMessage(B_QUIT_REQUESTED);
	fLastFile = 0;
*/

}

LoggerApp::~LoggerApp()
{
//	delete [] fFiles;
	
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
			
			if ( im_what != IM::MESSAGE_RECEIVED && im_what != IM::MESSAGE_SENT )
				// don't create log files unless needed
				return;
			
			char name[512];
			char nickname[512];
			
			if ( c.GetName(name,sizeof(name)) != B_OK )
				strcpy(name,"unknown contact");
			
			if ( c.GetNickname(nickname,sizeof(nickname)) != B_OK )
				strcpy(name,"unknown nick");
			
			char datestamp[11];
			time_t now = time(NULL);
			strftime(datestamp, sizeof(datestamp), "%Y-%m-%d", localtime(&now));
			
			BString directory = "/boot/home/Logs/IM/";
			directory << name << " [" << nickname << "]" << "/";
			BString filename = directory;
			filename << datestamp << ".txt";
			
			BFile file(filename.String(), B_READ_WRITE | B_CREATE_FILE | B_OPEN_AT_END);
			if (file.InitCheck() != B_OK) {
				if (create_directory(directory.String(), 0777) != B_OK) return;
				
				file = BFile(filename.String(), B_READ_WRITE | B_CREATE_FILE | B_OPEN_AT_END);
				if (file.InitCheck() != B_OK) return;
			};
				
			char timestr[64];
			strftime(timestr,sizeof(timestr),"%Y-%m-%d [%H:%M] ", localtime(&now) );
			
			switch ( im_what )
			{
				case IM::MESSAGE_SENT:
				{
					BString log = "At ";
					log << timestr << "you tell " << name << " (" << nickname << 
						"): " << msg->FindString("message") << "\n";
					
					file.Write(log.String(), log.Length());
				}	break;
				
				case IM::MESSAGE_RECEIVED:
				{
					BString log = "At ";
					log << timestr << name << " (" << nickname << ") said: " <<
						msg->FindString("message") << "\n";
					
					file.Write(log.String(), log.Length());
				}	break;
				
				default:
					break;
			}
			
			file.Unset();
			
		}	break;
		default:
			BApplication::MessageReceived(msg);
	}
}

/*
void LoggerApp::SetMaxFiles(int max) {
//	Arbitrary max, anyone know a proper / realistic one?
	if ((max < 100) > (max > 0)) {
		fMaxFiles = max;
		return B_OK;
	} else {
		return B_ERROR;
	};
};

int LoggerApp::MaxFiles(void) {
	return fMaxFiles;
};
*/
