#ifndef MSNCONNECTION_H
#define MSNCONNECTION_H

#include <UTF8.h>
#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>

#include <list>
#include <vector>

#include <libim/Helpers.h>
#include <libim/Protocol.h>
#include <libim/Constants.h>

#include "MSNManager.h"
#include "MSNHandler.h"
#include "Command.h"
#include "HTTPFormatter.h"
#include "P2PHeader.h"
#include "P2PContents.h"

typedef pair <char *, int16> ServerAddress;
typedef list<Command *> CommandQueue;
class MSNManager;

class MSNConnection : public BLooper {
	public:
						MSNConnection();
						MSNConnection(const char *server, uint16 port,
							MSNManager *man);
						~MSNConnection();
						
		void			MessageReceived(BMessage *msg);
		
		void			SetTo( const char * server, uint16 port, MSNManager * man );
		
		status_t		Send(Command *command, queuestyle queue = qsQueue);
		status_t		ProcessCommand(Command *command);

		inline const char
						*Server(void) const { return fServer; };
		inline uint16	Port(void) const { return fPort; };

		bool			QuitRequested();
	
	protected:
		virtual status_t handleVER( Command * );
		virtual status_t handleNLN( Command * );
		virtual status_t handleCVR( Command * );
		virtual status_t handleRNG( Command * );
		virtual status_t handleXFR( Command * );
		virtual status_t handleCHL( Command * );
		virtual status_t handleUSR( Command * );
		virtual status_t handleMSG( Command * );
		virtual status_t handleADC( Command * );
		virtual status_t handleLST( Command * );
		virtual status_t handleQRY( Command * );
		virtual status_t handleGTC( Command * );
		virtual status_t handleBLP( Command * );
		virtual status_t handlePRP( Command * );
		virtual status_t handleCHG( Command * );
		virtual status_t handleFLN( Command * );
		virtual status_t handleSYN( Command * );
		virtual status_t handleJOI( Command * );
		virtual status_t handleCAL( Command * );
		virtual status_t handleIRO( Command * );
		virtual status_t handleANS( Command * );
		virtual status_t handleBYE( Command * );
		
		void			StartReceiver(void);
		void			StopReceiver(void);
		void			GoOnline(void);
		void			ClearQueues(void);
		
		ServerAddress	ExtractServerDetails(char *details);
		
		MSNManager		*fManager;	
		BMessenger		fManMsgr;

	private:
		int32			NetworkSend(Command *command);
		int32			ConnectTo(const char *hostname, uint16 port);
		static int32	Receiver(void *con);
		status_t		SSLSend(const char *host, HTTPFormatter *send,
			HTTPFormatter **recv);
		void			Error( const char * );
		void			Progress( const char * id, const char * msg, float );
		
		list<BString>	fContacts;
		
		char			*fServer;
		uint16			fPort;
		
		CommandQueue	fOutgoing;
		CommandQueue	fWaitingOnline;
		uint32			fTrID;
		
		BMessenger		*fSockMsgr;
		BMessageRunner	*fRunner;
		BMessageRunner	*fKeepAliveRunner;
		
		int16			fSock;
		
		uint8			fState;
		thread_id		fThread;
		

		
};

#endif
