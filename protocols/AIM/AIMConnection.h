#ifndef AIMCONNECTION_H
#define AIMCONNECTION_H

#include "AIMManager.h"

#include "AIMConstants.h"
#include "FLAP.h"
#include "TLV.h"
#include "SNAC.h"

#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>

#include <list>
#include <vector>

#include <libim/Helpers.h>
#include <libim/Protocol.h>
#include <libim/Constants.h>

typedef pair <char *, uint16> ServerAddress;
class AIMManager;

class AIMConnection : public BLooper {
	public:
						AIMConnection(const char *server, uint16 port,
							AIMManager *man, bool uberDebug = false);
						~AIMConnection();
						
		void			MessageReceived(BMessage *msg);
				
		uint8			SupportedSNACs(void) const;
		uint16			SNACAt(uint8 index) const;
		bool			Supports(const uint16 family) const;
		
		status_t		Send(Flap *flap);

		inline const char
						*Server(void) const { return fServer; };
		inline uint16	Port(void) const { return fPort; };

	private:
bool fUberDebug;
	
	
		int32			ConnectTo(const char *hostname, uint16 port);
		static int32	Receiver(void *con);
		void			StartReceiver(void);
		void			StopReceiver(void);
		ServerAddress	ExtractServerDetails(char *details);
			
		char			*fServer;
		uint16			fPort;
		
		list<Flap *>	fOutgoing;
		uint16			fOutgoingSeqNum;
		
		vector<uint16>	fSupportedSNACs;

		BMessenger		fManMsgr;
		BMessenger		*fSockMsgr;
		BMessageRunner	*fRunner;
		BMessageRunner	*fKeepAliveRunner;
		
		int16			fSock;
		
		uint8			fState;
		thread_id		fThread;
		
		uint32			fRequestID;
		
		AIMManager		*fManager;
};

#endif
