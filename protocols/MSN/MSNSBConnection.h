#ifndef MSN_SB_CONNECTION_H
#define MSN_SB_CONNECTION_H

#include "MSNConnection.h"

#include <string>
#include <list>

class MSNSBConnection : public MSNConnection
{
	public:
		MSNSBConnection(const char *server, uint16 port, MSNManager *man );
		
		~MSNSBConnection();
		
		virtual void MessageReceived( BMessage * );
		
		bool IsGroupChat() const;
		
		bool IsSingleChatWith( const char * ) const;
		
		bool InChat( const char * ) const;
		
		/**
			Send a message to someone. If no participants have joined
			yet, store the message until they have and then send.
		*/
		void SendMessage( Command * );
		
	protected:
		list<string>	fParticipants;
		list<Command*>	fPendingMessages;
		
		virtual status_t handleCAL( Command * );
		virtual status_t handleJOI( Command * );
		virtual status_t handleIRO( Command * );
		virtual status_t handleBYE( Command * );
		virtual status_t handleUSR( Command * );
		virtual status_t handleANS( Command * );
};

#endif
