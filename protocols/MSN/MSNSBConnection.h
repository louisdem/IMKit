#ifndef MSN_SB_CONNECTION_H
#define MSN_SB_CONNECTION_H

#include "MSNConnection.h"

class MSNSBConnection : public MSNConnection
{
	public:
		MSNSBConnection(const char *server, uint16 port, MSNManager *man );
		
		~MSNSBConnection();
		
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
};

#endif
