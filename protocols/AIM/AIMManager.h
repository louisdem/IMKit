#ifndef AIMMANAGER_H
#define AIMMANAGER_H

#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>
#include <String.h>

#ifdef BONE
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <unistd.h>
#else
	#include <net/socket.h>
	#include <net/netdb.h>
#endif

#include <libim/Helpers.h>

#include <list>

#include "AIMConnection.h"
#include "FLAP.h"
#include "TLV.h"

enum {
	AMAN_PULSE = 'ampu',
	AMAN_KEEP_ALIVE = 'amka',
	AMAN_GET_SOCKET = 'amgs',
	AMAN_FLAP_OPEN_CON = 'amoc',
	AMAN_FLAP_SNAC_DATA = 'amsd',
	AMAN_FLAP_ERROR = 'amfe',
	AMAN_FLAP_CLOSE_CON = 'amcc',
	AMAN_NEW_CONNECTION = 'amnc',
	AMAN_CLOSED_CONNECTION = 'amcd',

	AMAN_STATUS_CHANGED = 'amsc',
	AMAN_NEW_CAPABILITIES = 'amna'
};

enum {
	AMAN_OFFLINE = 0,
	AMAN_CONNECTING = 1,
	AMAN_AWAY = 2,
	AMAN_ONLINE = 3
};

class AIMConnection;
class AIMHandler;

class AIMManager : public BLooper {
	public:
						AIMManager(AIMHandler *handler);
						~AIMManager(void);
						
			status_t	Send(Flap *f);
			
			void		MessageReceived(BMessage *message);
			
			status_t	MessageUser(const char *screenname, const char *message);
			status_t	AddBuddy(const char *buddy);
			status_t	AddBuddies(list<char *>buddies);
			int32		Buddies(void) const;

			status_t	Login(const char *server, uint16 port,
							const char *username, const char *password);
			uchar		IsConnected(void) const;
			status_t	LogOff(void);
			status_t	RequestBuddyIcon(const char *buddy);
			
			status_t	TypingNotification(const char *buddy, uint16 typing);
			
	private:	
			char		*EncodePassword(const char *pass);
		list<BString>	fBuddy;
		list<AIMConnection *>
						fConnections;	
		list<Flap *>	fWaitingSupport;
		
		BMessageRunner	*fRunner;
		BMessageRunner	*fKeepAliveRunner;
			uchar		fConnectionState;

			char		*fOurNick;

		AIMHandler		*fHandler;
};

#endif
