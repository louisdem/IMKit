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

using namespace IM;

class AIMProtocol : public IM::Protocol
{
	public:
				AIMProtocol();
		virtual ~AIMProtocol();
		
		virtual status_t Init( BMessenger );
		virtual status_t Shutdown();
		
		virtual status_t Process( BMessage * );
		
		virtual const char * GetSignature();
	
		virtual BMessage GetSettingsTemplate();
		
		virtual status_t UpdateSettings( BMessage & );
		
		virtual uint32 GetEncoding();
		
	private:
		#define ICQ_THREAD_NAME "AIM Protocol"
		
		BMessenger		fMsgr;
		thread_id		fThread;
		AIMManager		*fManager;
		char			*fScreenName;
		char			*fPassword;

};

