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

class AIMConnection : public BLooper {
	public:
						AIMConnection(const char *server, uint16 port,
							BMessenger manager);
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

		BMessenger		fManager;
		BMessenger		*fSockMsgr;
		BMessageRunner	*fRunner;
		BMessageRunner	*fKeepAliveRunner;
		
		int16			fSock;
		
		uint8			fState;
		thread_id		fThread;
		
		uint32			fRequestID;
};

#endif