#ifndef AIMMANAGER_H
#define AIMMANAGER_H

#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>
#include <String.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>


#include <libim/Helpers.h>

#include <list>

#include "FLAP.h"
#include "TLV.h"

enum {
	AMAN_PULSE = 'ampu',
	AMAN_KEEP_ALIVE = 'amka',
	AMAN_GET_SOCKET = 'amgs',
	AMAN_FLAP_OPEN_CON = 'amoc',
	AMAN_FLAP_SNAC_DATA = 'amsd',
	AMAN_FLAP_ERROR = 'amfe',
	AMAN_FLAP_CLOSE_CON = 'amcc'
};

enum {
	AMAN_OFFLINE,
	AMAN_CONNECTING,
	AMAN_AWAY,
	AMAN_ONLINE
};

class AIMManager : public BLooper {
	public:
						AIMManager(BMessenger im_kit);
						~AIMManager(void);
						
			status_t	Send(Flap *f);
			
			void		MessageReceived(BMessage *message);
			
			status_t	MessageUser(const char *screenname, const char *message);
			status_t	AddBuddy(const char *buddy);
			int32		Buddies(void) const;

			status_t	Login(const char *server, uint16 port,
							const char *username, const char *password);
			uchar		IsConnected(void) const;
			status_t	LogOff(void);
			
	private:
			void		StartMonitor(void);
			void		StopMonitor(void);
			
		static int32	MonitorSocket(void *messenger);
			int32		ConnectTo(const char *hostname, uint16 port);
		
	
			char		*EncodePassword(const char *pass);
		list<Flap *>	fOutgoing;
		list<Flap>		fIncoming;
		list<BString>	fBuddy;
			uint16		fOutgoingSeqNum;
			uint16		fIncomingSeqNum;
			uint32		fRequestID;			
			int32		fSock;
		
			thread_id	fSockThread;
			BMessenger	*fSockMsgr;
		
		BMessageRunner	*fRunner;
		BMessageRunner	*fKeepAliveRunner;
		BMessenger		fIMKit;
		uchar			fConnectionState;
};

#endif
