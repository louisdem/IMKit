#include "MSNSBConnection.h"

#include <algorithm>

MSNSBConnection::MSNSBConnection(const char *server, uint16 port, MSNManager *man)
:	MSNConnection(server,port,man)
{
}

MSNSBConnection::~MSNSBConnection()
{
}

void
MSNSBConnection::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case msnmsgPing:
			// ignore.
			break;
		
		default:
			MSNConnection::MessageReceived(msg);
			break;
	}
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
	LOG(kProtocolName, liDebug, "C %lX: Processing CAL (SB)", this);
	return B_OK;
}

status_t
MSNSBConnection::handleJOI( Command * cmd )
{
	// someone new in chat
	LOG(kProtocolName, liDebug, "C %lX: Processing JOI (SB): %s", this, cmd->Param(0));
	
	fParticipants.push_back( cmd->Param(0) );
	
	// send any pending messages
	for ( list<Command*>::iterator i=fPendingMessages.begin(); i != fPendingMessages.end(); i++ )
	{
		Send( *i );
	}
	fPendingMessages.clear();
	
	return B_OK;
}

status_t
MSNSBConnection::handleIRO( Command * cmd )
{
	// List of those already in chat
	LOG(kProtocolName, liDebug, "C %lX: Processing IRO (SB): %s", this, cmd->Param(2));
	
	fParticipants.push_back( cmd->Param(2) );
	
	return B_OK;
}

/**
	Fully connected (got list of participants), send any pending messages
*/
status_t
MSNSBConnection::handleANS( Command * cmd )
{
	// send any pending messages
	for ( list<Command*>::iterator i=fPendingMessages.begin(); i != fPendingMessages.end(); i++ )
	{
		Send( *i );
	}
	
	fPendingMessages.clear();
	
	return B_OK;
}

status_t
MSNSBConnection::handleBYE( Command * cmd )
{
	// Someone left the conversation
	LOG(kProtocolName, liDebug, "C %lX: Processing BYE (SB): %s left.", this, cmd->Param(0));
	
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
	LOG(kProtocolName, liDebug, "C %lX: Processing USR (SB)", this);
	
	GoOnline();
	
	return B_OK;
}

void
MSNSBConnection::SendMessage( Command * cmd )
{
	if ( fParticipants.size() > 0 )
	{ // someone's listening, send away
		Send(cmd);
		return;
	}
	
	// nobody there, queue the message until we get a JOI
	fPendingMessages.push_back( cmd );
}
