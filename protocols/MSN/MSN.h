#ifndef MSN_H
#define MSN_H

#include <iostream>
#include <map>
#include <string>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <libim/Protocol.h>
#include <libim/Constants.h>
#include <Messenger.h>
#include <OS.h>

#include "MSNManager.h"
#include "MSNHandler.h"

using namespace IM;

class MSNHandler;

class MSNProtocol : public IM::Protocol, public MSNHandler
{
	public:
				MSNProtocol();
		virtual ~MSNProtocol();
		
		virtual status_t Init( BMessenger );
		virtual status_t Shutdown();
		
		virtual status_t Process( BMessage * );
		
		virtual const char * GetSignature();
//		virtual uint32	Capabilities();
	
		virtual BMessage GetSettingsTemplate();
		
		virtual status_t UpdateSettings( BMessage & );
		
		virtual uint32 GetEncoding();
		
			status_t	StatusChanged(const char *nick, online_types status);
			status_t	MessageFromUser(const char *passport, const char *msg);
			status_t	UserIsTyping(const char *nick, typing_notification type);
			status_t 	SSIBuddies(list<BString> buddies);

	private:
			BString 	NormalizeNick(const char *nick);
			BString 	GetScreenNick(const char *nick);
	
		BMessenger		fMsgr;
		thread_id		fThread;
		MSNManager		*fManager;
		BString			fPassport;
		BString			fPassword;
		BString			fDisplayName;
		

		map<string,BString>		fNickMap;
};

#endif
