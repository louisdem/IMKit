#ifndef MSNMANAGER_H
#define MSNMANAGER_H

#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>
#include <String.h>

#ifdef BONE
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <unistd.h>
#else
	#include <net/socket.h>
	#include <net/netdb.h>
#endif
#include <sys/select.h>

#include <libim/Helpers.h>

#include <list>

enum {
	msnmsgDataReady = 'msn0',
	msnmsgNewConnection = 'msn1',
	msnmsgCloseConnection = 'msn2',
	msnmsgOurStatusChanged = 'msn3',
	msnmsgPing = 'msn4',
	msnmsgPulse = 'msn5',
	msnmsgGetSocket = 'msn6',
	msnmsgStatusChanged = 'msn7',
	msnMessageRecveived = 'msn8',
	msnAuthRequest = 'msn9',
	msnmsgRemoveConnection = 'msna'
};

enum queuestyle {
	qsImmediate,
	qsQueue,
	qsOnline
};

class MSNSBConnection;
class MSNConnection;
class MSNHandler;
class Command;

#include "Command.h"

typedef map<BString, MSNConnection *> switchboardmap;
typedef map<int32, Command *> tridmap;

// Confusing data structures, Ahoy! This is a map of TrIDs to the user the Command is targetted.
typedef map<int32, pair<BString, Command *> > waitingmsgmap;

typedef map<BString, int8> waitingauth;

typedef list<MSNConnection*> connectionlist;

class MSNManager : public BLooper {
	public:
						MSNManager(MSNHandler *handler);
						~MSNManager(void);
									
			void		MessageReceived(BMessage *message);
			
			status_t	MessageUser(const char *screenname, const char *message);
			status_t	AddBuddy(const char *buddy);
			status_t	AddBuddies(list<char *>buddies);
			int32		Buddies(void) const;

			status_t	Login(const char *server, uint16 port,
							const char *passport, const char *password,
							const char *displayname);
			uchar		IsConnected(void) const;
			status_t	LogOff(void);
			status_t	RequestBuddyIcon(const char *buddy);
			
			status_t	AuthUser(const char *passport);
			status_t	BlockUser(const char *passport);
			status_t	SetDisplayName(const char *profile);
			status_t	SetAway(bool away = true);
			status_t	TypingNotification(const char *buddy, uint16 typing);
		inline uchar	ConnectionState(void) const { return fConnectionState; };
			
			status_t	Send(Command *command);
		
		inline const char *Passport(void) const { return fPassport.String(); };
		inline const char *DisplayName(void) const { return fDisplayName.String(); };
		inline const char *Password(void) const { return fPassword.String(); };
		
		MSNHandler		*Handler(void) { return fHandler; };
	private:
		waitingmsgmap	fWaitingSBs;
		
		waitingauth		fWaitingAuth;	// For people requesting OUR auth
		waitingauth		fWantsAuth;		// For people whose auth WE want
		
		list<BString>	fBuddy;
		MSNConnection	*fNoticeCon;
		connectionlist	fConnections;	
		connectionlist	fConnectionPool;
		
		BMessageRunner	*fRunner;
		BMessageRunner	*fKeepAliveRunner;
			uchar		fConnectionState;

			BString		fPassport;
			BString		fDisplayName;
			BString		fPassword;
			BString		fAwayMsg;
	
	protected:
		friend class 	MSNConnection;
		MSNHandler		*fHandler;
};

#endif
