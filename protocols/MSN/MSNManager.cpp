#include "MSNManager.h"

#include <libim/Protocol.h>
#include <libim/Constants.h>
#include <libim/Helpers.h>
#include <UTF8.h>

#include "MSNHandler.h"

void PrintHex(const unsigned char* buf, size_t size) {
	if ( g_verbosity_level != liDebug ) {
		// only print this stuff in debug mode
		return;
	}
	
	int i = 0;
	int j = 0;
	int breakpoint = 0;

	for(;i < size; i++) {
		fprintf(stdout, "%02x ", (unsigned char)buf[i]);
		breakpoint++;	

		if(!((i + 1)%16) && i) {
			fprintf(stdout, "\t\t");
			for(j = ((i+1) - 16); j < ((i+1)/16) * 16; j++)	{
				if(buf[j] < 30) {
					fprintf(stdout, ".");
				} else {
					fprintf(stdout, "%c", (unsigned char)buf[j]);
				};
			}
			fprintf(stdout, "\n");
			breakpoint = 0;
		}
	}
	
	if(breakpoint == 16) {
		fprintf(stdout, "\n");
		return;
	}

	for(; breakpoint < 16; breakpoint++) {
		fprintf(stdout, "   ");
	}
	
	fprintf(stdout, "\t\t");

	for(j = size - (size%16); j < size; j++) {
		if(buf[j] < 30) {
			fprintf(stdout, ".");
		} else {
			fprintf(stdout, "%c", (unsigned char)buf[j]);
		};
	}
	
	fprintf(stdout, "\n");
}

void remove_html( char * msg )
{
	bool is_in_tag = false;
	int copy_pos = 0;
	
	char * copy = new char[strlen(msg)+1];
	
	for ( int i=0; msg[i]; i++ )
	{
		switch ( msg[i] )
		{
			case '<':
				is_in_tag = true;
				break;
			case '>':
				is_in_tag = false;
				break;
			default:
				if ( !is_in_tag )
				{
					copy[copy_pos++] = msg[i];
				}
		}
	}
	
	copy[copy_pos] = 0;
	
	strcpy(msg, copy);
}

MSNManager::MSNManager(MSNHandler *handler)
: BLooper("MSNManager looper") {	
	fConnectionState = otOffline;
	
	fHandler = handler;
	fDisplayName = "IMKit User";
	fPassport = "";
	fPassword = "";
	fNoticeCon = NULL;
};

MSNManager::~MSNManager(void) {
	LogOff();
}

status_t MSNManager::Login(const char *server, uint16 port, const char *passport,
	const char *password, const char *displayname) {
	
	if ((passport == NULL) || (password == NULL)) {
		LOG(kProtocolName, liHigh, "MSNManager::Login: passport or password not "
			"set");
		return B_ERROR;
	}
	
	fPassport = passport;
	fPassword = password;
	fDisplayName = displayname;
	
	if (fConnectionState == otOffline) {
		if (fNoticeCon == NULL) {
			fNoticeCon = new MSNConnection(server, port, this);
			fNoticeCon->Run();
		};
		
		Command *command = new Command("VER");
		command->AddParam(kProtocolsVers);
		fNoticeCon->Send(command);

		return B_OK;
	} else {
		LOG(kProtocolName, liDebug, "MSNManager::Login: Already online");
		return B_ERROR;
	};
};

status_t MSNManager::Send(Command *command) {
//	MSNConnection *con = fConnections.front();
//	if (con != NULL) con->Send(command);
};

void MSNManager::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case msnMessageRecveived: {
			BString passport = msg->FindString("passport");
			BString message = msg->FindString("message");

			fHandler->MessageFromUser(passport.String(), message.String());
		} break;
	
		case msnmsgStatusChanged: {
			uint8 status = msg->FindInt8("status");
			BString passport = msg->FindString("passport");
			fHandler->StatusChanged(passport.String(), (online_types)status);
		} break;

		case msnmsgOurStatusChanged: {
			uint8 status = msg->FindInt8("status");
			fHandler->StatusChanged(fPassport.String(), (online_types)status);
			fConnectionState = status;
		} break;

		case msnmsgNewConnection: {
			const char *cookie;
			int32 bytes = 0;
			int16 port = 0;
			char *host = NULL;
			const char *type = NULL;

			if (msg->FindString("host", (const char **)&host) != B_OK) {
				LOG(kProtocolName, liLow, "Got a malformed new connection message"
					" (Host)");
				return;
			};
			if (msg->FindInt16("port", &port) != B_OK) {
				LOG(kProtocolName, liLow, "Got a malformed new connection message"
					" (Port)");
				return;
			};
			
			msg->FindString("type", &type);
			
			LOG(kProtocolName, liDebug, "Got a new connection to \"%s\":%i of type \"%s\"", host,
				port, type);
			
			MSNConnection *con = new MSNConnection(host, port, this);
			con->Run();

			if (strcmp(type, "NS") == 0) {
				Command *command = new Command("VER");
				command->AddParam(kProtocolsVers);
				
				con->Send(command);
				
				fNoticeCon = con;
				
				return;
			};
			if (strcmp(type, "RNG") == 0) {
				const char *auth = msg->FindString("authString");
				const char *sessionID = msg->FindString("sessionID");
				const char *inviter = msg->FindString("inviterPassport");

				Command *command = new Command("ANS");
				command->AddParam(fPassport.String());
				command->AddParam(auth);
				command->AddParam(sessionID);
				
				con->Send(command);
				
				fSwitchBoard[inviter] = con;
				
				return;
			};
			
			if (strcmp(type, "SB") == 0) {
printf("Got SB redir\n");
				const char *authString = msg->FindString("authString");

				Command *command = new Command("USR");
				command->AddParam(Passport());
				command->AddParam(authString);
				
				con->Send(command);				
			};
		} break;

		case msnmsgCloseConnection: {
			MSNConnection *con = NULL;
			msg->FindPointer("connection", (void **)&con);

			if (con != NULL) {
				LOG(kProtocolName, liLow, "Connection (%s:%i) closed", con->Server(), con->Port());
				
//				fConnections.remove(con);
				
//				fSwitchboard.remove(
				
//				con->Lock();
//				con->Quit();
				
				BMessenger(con).SendMessage(B_QUIT_REQUESTED);
//				LOG(kProtocolName, liLow, "After close we have %i connections", fConnections.size());
				
//				if (fConnections.size() == 0) {
//					fHandler->StatusChanged(fPassport.String(), otOffline);
//					fConnectionState = otOffline;
//				};
			};
		} break;

		default: {
			BLooper::MessageReceived(msg);
		};
	};
}

// -- Interface

status_t MSNManager::MessageUser(const char *passport, const char *message) {
printf("Connection state: %i\n", fConnectionState);
	if ((fConnectionState != otOffline) && (fConnectionState != otConnecting)) {
		if (fNoticeCon == NULL) return B_ERROR;

		switchboardmap::iterator it = fSwitchBoard.find(passport);
		if (it == fSwitchBoard.end()) {
			LOG(kProtocolName, liHigh, "Could not message \"%s\" - no connection established",
				passport);

			Command *sbReq = new Command("XFR");
			sbReq->AddParam("SB");	// Request a SB connection;
			fNoticeCon->Send(sbReq);

			return B_ERROR;
		};
		
		Command *msg = new Command("MSG");
		msg->AddParam("N"); // Don't ack packet
		BString format = "MIME-Version: 1.0\r\n"
			"Content-Type: text/plain; charset=UTF-8\r\n"
			"X-MMS-IM-Format: FN+Arial; EF=I; CO=0; CS=0; PF=22\r\n\r\n";
		format << message;
		
		msg->AddPayload(format.String(), format.Length());
		
		it->second->Send(msg);
		
		return B_OK;
	};
	
	return B_ERROR;
};

status_t MSNManager::AddBuddy(const char *buddy) {
	status_t ret = B_ERROR;
	return ret;
};

status_t MSNManager::AddBuddies(list <char *>buddies) {
	return B_OK;
};

int32 MSNManager::Buddies(void) const {
	return fBuddy.size();
};

uchar MSNManager::IsConnected(void) const {
	return fConnectionState;
};

status_t MSNManager::LogOff(void) {
	status_t ret = B_ERROR;

	LOG(kProtocolName, liLow, "%i connection(s) to kill", fSwitchBoard.size());
	switchboardmap::iterator it;
	
	for (it = fSwitchBoard.begin(); it != fSwitchBoard.end(); it++) {
		MSNConnection *con = (it->second);
		if (con == NULL) continue;
		LOG(kProtocolName, liLow, "Killing switchboard connection to %s:%i",
			con->Server(), con->Port());
		Command *bye = new Command("OUT");
		bye->UseTrID(false);
		con->Send(bye, qsQueue);

		con->Lock();
		con->Quit();
	};
	
	fSwitchBoard.clear();
	
	fConnectionState = otOffline;
	
	if (fNoticeCon) {
		Command *bye = new Command("OUT");
		bye->UseTrID(false);
		fNoticeCon->Send(bye, qsImmediate);
		fNoticeCon->Lock();
		fNoticeCon->Quit();
		fNoticeCon = NULL;
	};
	
	fHandler->StatusChanged(fPassport.String(), otOffline);
	ret = B_OK;
	
	return ret;
};

status_t MSNManager::RequestBuddyIcon(const char *buddy) {
	LOG(kProtocolName, liDebug, "Requesting buddy icon for \"%s\"", buddy);
	return B_OK;
};

status_t MSNManager::TypingNotification(const char *buddy, uint16 typing) {
	return B_OK;
};

status_t MSNManager::SetAway(const char *message) {
};

status_t MSNManager::SetDisplayName(const char *displayname) {
	fDisplayName = displayname;

	if (fNoticeCon) {
		Command *rea = new Command("PRP");
		rea->AddParam("MFN");
		rea->AddParam(DisplayName(), true);
	
		fNoticeCon->Send(rea);
	};

	return B_OK;
};

