#include "MSNSBConnection.h"

#include <algorithm>

MSNSBConnection::MSNSBConnection(const char *server, uint16 port, MSNManager *man)
:	MSNConnection(server,port,man)
{
}

MSNSBConnection::~MSNSBConnection()
{
}

bool
MSNSBConnection::IsGroupChat() const
{
	return fParticipants.size() > 1;
}

bool
MSNSBConnection::IsSingleChatWith( const char * who ) const
{
	return fParticipants.size() == 1 && *fParticipants.begin() == who;
}

bool
MSNSBConnection::InChat( const char * who ) const
{
	list<string>::const_iterator i;
	
	for ( i = fParticipants.begin(); i != fParticipants.end(); i++ )
	{
		if ( *i == who )
		{
			break;
		}
	}
	
	return i != fParticipants.end();
}

status_t
MSNSBConnection::handleCAL( Command * cmd )
{
	// Invite someone (response)
	LOG(kProtocolName, liDebug, "Processing CAL (SB)");
	return B_OK;
}

status_t
MSNSBConnection::handleJOI( Command * cmd )
{
	// someone new in chat
	LOG(kProtocolName, liDebug, "Processing JOI (SB): %s", cmd->Param(0));
	
	fParticipants.push_back( cmd->Param(0) );
	
	return B_OK;
}

status_t
MSNSBConnection::handleIRO( Command * cmd )
{
	// List of those already in chat
	LOG(kProtocolName, liDebug, "Processing IRO (SB): %s", cmd->Param(2));

	fParticipants.push_back( cmd->Param(2) );
	
	return B_OK;
}

status_t
MSNSBConnection::handleBYE( Command * cmd )
{
	// Someone left the conversation
	LOG(kProtocolName, liDebug, "Processing BYE (SB): %s left.", cmd->Param(0));
	
	fParticipants.remove( cmd->Param(0) );
	
	if ( fParticipants.size() == 0 )
	{ // last one left, leave too.
		Command * reply = new Command("OUT");
		reply->UseTrID(false);
		
		Send( reply, qsImmediate );
		
		BMessage closeCon(msnmsgCloseConnection);
		closeCon.AddPointer("connection", this);
		
		fManMsgr.SendMessage(&closeCon);
	}
	
	return B_OK;
}

status_t
MSNSBConnection::handleUSR( Command * cmd  )
{
	// Just send any pending messages here.
	LOG(kProtocolName, liDebug, "Processing USR (SB)");
	
	GoOnline();
	
	return B_OK;
}
