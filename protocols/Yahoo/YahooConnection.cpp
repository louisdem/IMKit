#include "YahooConnection.h"

#include <stdio.h>
#include <algorithm>

// We include this for the ONLINE_TEXT, AWAY_TEXT and OFFLINE_TEXT defines
#include <libim/Constants.h>

map<int, YahooConnection*> gYahooConnections;


YahooConnection::YahooConnection( YahooManager * mgr, const char * yahooID, const char * pass )
:	fManager( mgr ),
	fID(-1),
	fYahooID( strdup(yahooID) ),
	fPassword( strdup(pass) ),
	fStatus( YAHOO_STATUS_OFFLINE ),
	fAlive(true),
	fGotBuddyList(false)
{
	fThread = spawn_thread(
		yahoo_io_thread,
		"Yahoo IO",
		B_NORMAL_PRIORITY,
		this
	);
	
	resume_thread( fThread );
}

YahooConnection::~YahooConnection()
{
	LogOff();
	
	fAlive = false;
	int32 thread_res=0;
	
	wait_for_thread( fThread, &thread_res );
	
	free( fYahooID );
	free( fPassword );
}

void
YahooConnection::SetAway( bool away )
{
	if ( fStatus == YAHOO_STATUS_OFFLINE )
	{
		fManager->Error( "Calling SetAway() when offline", NULL );
		return;
	}
	
	if ( away )
	{
		yahoo_set_away( fID, YAHOO_STATUS_BUSY, NULL, 1 );
		fManager->SetAway(true);
	} else 
	{
		yahoo_set_away( fID, YAHOO_STATUS_AVAILABLE, NULL, 1 );
		fManager->SetAway(false);
	}
}

void
YahooConnection::LogOff()
{
	if ( fID < 0 )
		return;
	
	yahoo_logoff(fID);
	
	gYahooConnections[fID] = NULL;
	
	fStatus = YAHOO_STATUS_OFFLINE;
	fID = -1;
	
	fManager->LoggedOut();
	
	// owner should delete us now to stop the thread and clean up
}

void
YahooConnection::Message( const char * who, const char * msg )
{
	if ( fStatus == YAHOO_STATUS_OFFLINE )
	{
		fManager->Error( "Can't send message, not connected", who );
		return;
	}
	
	yahoo_send_im( 
		fID, 
		NULL /* default identity */,
		who,
		msg,
		1 /* it's utf-8 */
	);
	
	fManager->MessageSent( who, msg );
}

void
YahooConnection::AddBuddy( const char * who )
{
	if ( !fGotBuddyList )
	{
		fBuddiesToAdd.push_back( who );
		return;
	}
	
	list<string>::iterator iter = find( fBuddies.begin(), fBuddies.end(), who );
	
	if ( iter == fBuddies.end() )
	{ // not in list, adding
		yahoo_add_buddy( fID, who, "BeOS Yahoo", "Adding you from BeOS" );
		fBuddies.push_back( who );
		printf("Yahoo: Added buddy\n");
	}
}

void
YahooConnection::RemoveBuddy( const char * who )
{
	if ( fStatus == YAHOO_STATUS_OFFLINE )
	{
		fManager->Error("Not connected when calling YahooConnection::RemoveBuddy()", NULL);
		return;
	}
	
	yahoo_remove_buddy( fID, who, "BeOS Yahoo" );
}

void
YahooConnection::cbStatusChanged( char * who, int stat, char * msg, int away )
{
	const char * status=NULL;
	
	switch ( stat )
	{
		case YAHOO_STATUS_AVAILABLE:
		case YAHOO_STATUS_IDLE:
			status = ONLINE_TEXT;
			break;
		
		case YAHOO_STATUS_BRB:
		case YAHOO_STATUS_BUSY:
		case YAHOO_STATUS_NOTATHOME:
		case YAHOO_STATUS_NOTATDESK:
		case YAHOO_STATUS_NOTINOFFICE:
		case YAHOO_STATUS_ONPHONE:
		case YAHOO_STATUS_ONVACATION:
		case YAHOO_STATUS_OUTTOLUNCH:
		case YAHOO_STATUS_STEPPEDOUT:
		case YAHOO_STATUS_CUSTOM:
			status = AWAY_TEXT;
			break;

		case YAHOO_STATUS_OFFLINE:
		case YAHOO_STATUS_INVISIBLE:
			status = OFFLINE_TEXT;
			break;
		
		default:
			break;
	}
	
	if ( status )
		fManager->BuddyStatusChanged( who, status );
}

void
YahooConnection::cbGotBuddies( YList * buds )
{
	fGotBuddyList = true;
	
	// copy from buds to buddies...
	for(; buds; buds = buds->next) {
		struct yahoo_buddy *bud = (struct yahoo_buddy *)buds->data;
		
		fBuddies.push_back( bud->id );
	}
	
	// add waiting buddies
	for ( list<string>::iterator iter=fBuddiesToAdd.begin(); iter != fBuddiesToAdd.end(); iter++ )
	{
		AddBuddy( (*iter).c_str() );
	}
	fBuddiesToAdd.clear();
	
	// Tell the manager!
	fManager->GotBuddyList( fBuddies );
}

void
YahooConnection::cbGotIM(char *who, char *msg, long tm, int stat, int utf8)
{
	fManager->GotMessage( who, msg );
}

void
YahooConnection::cbLoginResponse(int succ, char *url)
{
	if(succ == YAHOO_LOGIN_OK) {
		fStatus = yahoo_current_status(fID);
		fManager->LoggedIn();
		return;
	} else if(succ == YAHOO_LOGIN_UNAME) {
		fManager->Error("Could not log into Yahoo service - username not recognised.  Please verify that your username is correctly typed.", NULL);
	} else if(succ == YAHOO_LOGIN_PASSWD) {
		fManager->Error("Could not log into Yahoo service - password incorrect.  Please verify that your password is correctly typed.", NULL);
	} else if(succ == YAHOO_LOGIN_LOCK) {
		fManager->Error("Could not log into Yahoo service.  Your account has been locked.\nVisit [url] to reactivate it.\n", NULL);
	} else if(succ == YAHOO_LOGIN_DUPL) {
		fManager->Error("You have been logged out of the yahoo service, possibly due to a duplicate login.", NULL);
	} else if(succ == YAHOO_LOGIN_SOCK) {
		fManager->Error("The server closed the socket.", NULL);
	} else {
		fManager->Error("Could not log in, unknown reason.", NULL);
	}
	
	fStatus = YAHOO_STATUS_OFFLINE;
	
	LogOff();
}

void
YahooConnection::cbYahooError(char *err, int fatal)
{
	fManager->Error( err, NULL );
	
	if ( fatal )
	{
		LogOff();
	}
}

/*
void
YahooConnection::()
{
}

*/

void
YahooConnection::AddConnection( fd_conn * c )
{
	fConnections.AddItem(c);
}

void
YahooConnection::RemoveConnection( fd_conn *c )
{
	fConnections.RemoveItem(c);
}

fd_conn *
YahooConnection::ConnectionAt( int i )
{
	return (fd_conn*)fConnections.ItemAt(i);
}

int
YahooConnection::CountConnections()
{
	return fConnections.CountItems();
}

bool
YahooConnection::IsAlive()
{
	return fAlive;
}
