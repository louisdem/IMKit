#ifndef MSNCONNECTION_H
#define MSNCONNECTION_H

#include "MSNManager.h"
#include "MSNHandler.h"
#include "Command.h"

#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>

#include <list>
#include <vector>

#include <libim/Helpers.h>
#include <libim/Protocol.h>
#include <libim/Constants.h>

#include <cryptlib/cryptlib.h>
#include "HTTPFormatter.h"
#include "md5.h"

typedef pair <char *, int16> ServerAddress;
typedef list<Command *> CommandQueue;
class MSNManager;

class MSNConnection : public BLooper {
	public:
						MSNConnection(const char *server, uint16 port,
							MSNManager *man);
						~MSNConnection();
						
		void			MessageReceived(BMessage *msg);
		
		status_t		Send(Command *command, queuestyle queue = qsQueue);
		status_t		ProcessCommand(Command *command);

		inline const char
						*Server(void) const { return fServer; };
		inline uint16	Port(void) const { return fPort; };

		bool			QuitRequested();
		
	private:
		int32			NetworkSend(Command *command);
		int32			ConnectTo(const char *hostname, uint16 port);
		static int32	Receiver(void *con);
		void			StartReceiver(void);
		void			StopReceiver(void);
		ServerAddress	ExtractServerDetails(char *details);
		status_t		SSLSend(const char *host, HTTPFormatter *send,
			HTTPFormatter **recv);
		void			GoOnline(void);
		void			ClearQueues(void);
			
		char			*fServer;
		uint16			fPort;
		
		CommandQueue	fOutgoing;
		CommandQueue	fWaitingOnline;
		uint32			fTrID;
		
		BMessenger		fManMsgr;
		BMessenger		*fSockMsgr;
		BMessageRunner	*fRunner;
		BMessageRunner	*fKeepAliveRunner;
		
		int16			fSock;
		
		uint8			fState;
		thread_id		fThread;
		
		MSNManager		*fManager;
		
};

#endif
