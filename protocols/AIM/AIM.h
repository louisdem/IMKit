#include <iostream>
#include <map>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <libim/Protocol.h>
#include <libim/Constants.h>
#include <Messenger.h>
#include <OS.h>

#include "AIMManager.h"
#include "AIMHandler.h"

using namespace IM;

class AIMHandler;

class AIMProtocol : public IM::Protocol, public AIMHandler
{
	public:
				AIMProtocol();
		virtual ~AIMProtocol();
		
		virtual status_t Init( BMessenger );
		virtual status_t Shutdown();
		
		virtual status_t Process( BMessage * );
		
		virtual const char * GetSignature();
//		virtual uint32	Capabilities();
	
		virtual BMessage GetSettingsTemplate();
		
		virtual status_t UpdateSettings( BMessage & );
		
		virtual uint32 GetEncoding();
		
			status_t	StatusChanged(const char *nick, online_types status);
			status_t	MessageFromUser(const char *nick, const char *msg);
			status_t	UserIsTyping(const char *nick, typing_notification type);
			status_t 	SSIBuddies(list<BString> buddies);

	private:
			BString 	ReNick(const char *nick);
	
		#define ICQ_THREAD_NAME "AIM Protocol"
		
		BMessenger		fMsgr;
		thread_id		fThread;
		AIMManager		*fManager;
		char			*fScreenName;
		char			*fPassword;

};

