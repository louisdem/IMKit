#ifndef YAHOO_CONNECTION_H
#define YAHOO_CONNECTION_H

#include <map>
#include <OS.h>
#include <libyahoo2/yahoo2.h>
#include <libyahoo2/yahoo2_callbacks.h>
#include <List.h>
#include <String.h>

#include "YahooManager.h"

struct fd_conn {
	int tag;
	int fd;
	int id;
	yahoo_input_condition cond;
	void *data;
	int remove;
};

class YahooConnection
{
	public:
		YahooConnection( YahooManager *, const char *, const char * );
		~YahooConnection();
		
		void SetAway( bool );
		void LogOff();
		void Message( const char * who, const char * msg );
		// Call this to add new contacts, but only after having received the
		// buddy list so we don't double-add contacts
		void AddBuddy( const char * who );
		void RemoveBuddy( const char * who );
		
		// Stuff below this is for use by the yahoo lib interface only.
		// I could make it protected and add all them functions as friends,
		// but that'd be boring. Will do it 'later'.
		void cbStatusChanged( char * who, int stat, char * msg, int away );
		void cbGotBuddies( YList * buds );
		void cbGotIM(char *who, char *msg, long tm, int stat, int utf8);
		void cbLoginResponse(int succ, char *url);
		void cbYahooError(char *err, int fatal);
		
		void AddConnection( fd_conn * );
		void RemoveConnection( fd_conn * );
		fd_conn * ConnectionAt( int i );
		int CountConnections();
		
	private:
		friend int32 yahoo_io_thread( void * );
		
		YahooManager	* fManager;
		
		int 		fID;
		char * 		fYahooID;
		char * 		fPassword;
		
		int 		fStatus;
		
		thread_id	fThread;
		bool 		fAlive;
		bool		IsAlive();
		
		BList		fConnections;
		
		list<string>	fBuddies;
		list<string>	fBuddiesToAdd;
		bool			fGotBuddyList;
};

extern map<int, YahooConnection*> gYahooConnections;
extern int32 yahoo_io_thread( void * _data );
extern const char *  kProtocolName;

#endif
