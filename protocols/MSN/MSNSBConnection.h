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
		
		bool IsGroupChat() const;
		
		bool IsSingleChatWith( const char * ) const;
		
		bool InChat( const char * ) const;
		
	protected:
		list<string>	fParticipants;
		
		virtual status_t handleCAL( Command * );
		virtual status_t handleJOI( Command * );
		virtual status_t handleIRO( Command * );
		virtual status_t handleBYE( Command * );
		virtual status_t handleUSR( Command * );
};

#endif
