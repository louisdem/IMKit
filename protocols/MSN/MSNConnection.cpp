#include "MSNConnection.h"

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/md5.h>

const char *kClientVer = "0x0409 win 4.10 i386 MSNMSGR 6.0.0602 MSMSGS";
const char *kProtocolsVers = "MSNP10 MSNP9 CVR0";

const int16 kDefaultPort = 1863;
const uint32 kOurCaps = ccUnknown2;

extern const char *kClientIDString = "msmsgs@msnmsgr.com";
extern const char *kClientIDCode = "Q1P7W2E4J9R8U3S5";

void remove_html(char *msg);
void PrintHex(const unsigned char* buf, size_t size);

void MD5(unsigned char *text, char * digest)
{
	MD5state_st state;
	unsigned char md5data[32];
	
	MD5_Init(&state);
	MD5_Update(&state, text, (unsigned short)strlen((const char*)text));
	MD5_Final(md5data, &state);
	
	int i;
	for (i=0; i<MD5_DIGEST_LENGTH; i++)
		sprintf(&(digest[i*2]),"%02x",md5data[i]);
}


MSNConnection::MSNConnection()
: BLooper("MSNConnection looper") {
	fState = otOffline;
	fTrID = 0;
	fManager = NULL;
	fRunner = NULL;
	fKeepAliveRunner = NULL;
	fServer = NULL;
	fSock = -1;
	fSockMsgr = NULL;
	fThread = -1;
	
	Run();
};

MSNConnection::MSNConnection(const char *server, uint16 port, MSNManager *man)
: BLooper("MSNConnection looper") {
	fState = otOffline;
	fTrID = 0;
	fManager = NULL;
	fRunner = NULL;
	fKeepAliveRunner = NULL;
	fServer = NULL;
	fSock = -1;
	fSockMsgr = NULL;
	fThread = -1;
	
	Run();
	SetTo(server,port,man);
};

void
MSNConnection::SetTo(const char *server, uint16 port, MSNManager *man)
{
	fTrID = 0;
	fManager = man;
	fManMsgr = BMessenger(fManager);
	
	fServer = strdup(server);
	fPort = port;
	
	fRunner = new BMessageRunner(BMessenger(NULL, (BLooper *)this),
		new BMessage(msnmsgPulse), 500000, -1);

	fKeepAliveRunner = new BMessageRunner(BMessenger(NULL, (BLooper *)this),
		new BMessage(msnmsgPing), 60000000, -1);
	
	fState = otConnecting;
	
	fSock = ConnectTo(fServer, fPort);
	
	StartReceiver();
}

MSNConnection::~MSNConnection(void) {
	if ( fRunner )
		delete fRunner;
	if ( fKeepAliveRunner )
		delete fKeepAliveRunner;
	
	if ( fServer )
		free(fServer);
//	snooze(1000);
	
	StopReceiver();
	
	if (fSock > 0) close(fSock);

	BMessage removeCon(msnmsgRemoveConnection), reply;
	removeCon.AddPointer("connection", this);
	
	// There's a slight risk with not having the reply here, I think.
	// At least there used to be, a race condition with the MSNManager
	// reading the connection's host and port.   /ME
	fManMsgr.SendMessage(&removeCon/*, reply*/);
};

bool MSNConnection::QuitRequested() {
	if ( fState == otOnline )
	{ // leave gracefully if connected
		Command *bye = new Command("OUT");
		bye->UseTrID(false);
		Send(bye, qsImmediate);
	}
	
	return true;
}

int32 MSNConnection::ConnectTo(const char *hostname, uint16 port) {
	if (hostname == NULL) {
		LOG(kProtocolName, liLow, "C %lX: ConnectTo() called with NULL hostname - probably"
			" not authorised to login at this time", this);
		return B_ERROR;
	};

	int32 sock = 0;

	LOG(kProtocolName, liLow, "C %lX: MSNConn::ConnectTo(\"%s\", %i)", this, hostname, port);

	struct sockaddr_in remoteAddr;
	remoteAddr.sin_family = AF_INET;

//	XXX - BONE Specific
	if (inet_aton(hostname, &remoteAddr.sin_addr) == 0) {
		struct hostent *he = gethostbyname(hostname);
		if (he) {
			remoteAddr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
		} else {
			LOG(kProtocolName, liLow, "C %lX: MSNConn::ConnectTo(%s, %i) couldn't resolve",
				this, hostname, port);
			return B_ERROR;
		};
	};
    remoteAddr.sin_port = htons(port);
	memset(&(remoteAddr.sin_zero), 0, 8);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG(kProtocolName, liMedium, "C %lX: MSNConn::ConnectTo(%s, %i): Couldn't create socket",
			this, hostname, port);	
		return B_ERROR;
	};

	if (connect(sock, (struct sockaddr *)&remoteAddr, sizeof(remoteAddr)) == -1) {
		LOG(kProtocolName, liMedium, "C %lX: MSNConn::ConnectTo: Couldn't connect to %s:%i", 
			this, hostname, port);
		return B_ERROR;
	};

	LOG(kProtocolName, liLow, "C %lX:   Connected.", this);

	return sock;
};

void MSNConnection::StartReceiver(void) {
	fSockMsgr = new BMessenger(this);
	
	fThread = spawn_thread(Receiver, "MSN socket", B_NORMAL_PRIORITY, (void *)this);
	if (fThread > B_ERROR) {
		resume_thread(fThread);
		LOG(kProtocolName, liHigh, "C %lX: Started receiver thread", this);
	} else
		LOG(kProtocolName, liHigh, "C %lX: Error creating receiver thread in MSNConnection!", this);
};

void MSNConnection::StopReceiver(void) {
	if (fSockMsgr) {
		BMessenger * old_msgr = fSockMsgr;
		fSockMsgr = new BMessenger((BHandler *)NULL);
		delete old_msgr;
	};
	
	if (fThread > 0) {
//		the following causes a EINTR error in the reader thread, which will then
//		quit in a decent way.
		suspend_thread(fThread);
		resume_thread(fThread);
		int32 res=0;
		wait_for_thread(fThread, &res);
	}
	
	fThread = -1;
};

int32 MSNConnection::Receiver(void *con) {
	LOG(kProtocolName, liLow, "C[r] %lX: Receiver init", con);

	MSNConnection *connection = reinterpret_cast<MSNConnection *>(con);

	const uint32 kSleep = 2000000;
	const char *kHost = strdup(connection->Server()); // memory leak.
	const uint16 kPort = connection->Port();
	BMessenger **kMsgr = &connection->fSockMsgr;
//	BMessenger kManMsgr = connection->fManMsgr;
	
	int32 socket = 0;
	
	if ( !(*kMsgr)->IsValid() ) {
		LOG(kProtocolName, liLow, "C[r] %lX: Messenger wasn't valid!", connection);
		return B_ERROR;
	}
	
	BMessage reply;
	status_t ret = 0;
/*
	if ((ret = (*kMsgr)->SendMessage(msnmsgGetSocket, &reply)) == B_OK) {
		if ((ret = reply.FindInt32("socket", &socket)) != B_OK) {
			LOG(kProtocolName, liLow, "C[r] %lX: Couldn't get socket: %i", connection, ret);
			return B_ERROR;
		};
	} else {
		LOG(kProtocolName, liLow, "C[r] %lX: Couldn't obtain socket: %i", connection, ret);
		return B_ERROR;
	}
*/
	socket = connection->fSock;
		
	struct fd_set read;
	struct fd_set error;
	int16 bytes = 0;
	int32 processed = 0;
	char buffer[1024];
	BString commandBuff = "";
	int32 kNewLineLen = strlen("\r\n");
	
	LOG(kProtocolName, liLow, "C[r] %lX: Starting receiver loop", connection);
	
	while ((*kMsgr)->IsValid() == true) {
		FD_ZERO(&read);
		FD_ZERO(&error);
		
		FD_SET(socket, &read);
		FD_SET(socket, &error);

		memset(buffer, 0, sizeof(buffer));

		if (select(socket + 1, &read, NULL, &error, NULL) > 0) {
			if (FD_ISSET(socket, &error)) {
				LOG(kProtocolName, liLow, "C[r] %lX: Got socket error", connection);
				snooze(kSleep);
				continue;
			};
			
			if (FD_ISSET(socket, &read)) {
				bytes = recv(socket, buffer, sizeof(buffer), 0);
				if (bytes  > 0) {
					commandBuff += buffer;
					processed += bytes;
				} else {
					if ((*kMsgr)->IsValid() == false) return B_OK;
					
					LOG(kProtocolName, liLow, "C[r] %lX: Socket got less than 0 (or 0)",
						connection);
					perror("SOCKET ERROR");
					
					BMessage msg(msnmsgCloseConnection);
					msg.AddPointer("connection", con);
					
					connection->fManMsgr.SendMessage(&msg);
					connection->fState = otOffline;
					
					return B_ERROR;
				};					
			};
		};
		
		int32 commandLen = commandBuff.FindFirst("\r\n");
		
		while (commandLen > 0) {
			BString command = "";
			int32 totalLen = commandBuff.Length();
			commandBuff.MoveInto(command, 0, commandLen + kNewLineLen);

			processed = totalLen - (commandLen + kNewLineLen);

			Command *comm = new Command("");
			comm->MakeObject(command.String());
			int32 payloadLen = 0;

			if (comm->ExpectsPayload(&payloadLen) == true) {
				LOG(kProtocolName, liDebug, "C[r] %lX: Payload of %i, have %i bytes",
					connection, payloadLen, processed);
				
				while (processed < payloadLen) {
					FD_ZERO(&read);
					FD_ZERO(&error);
					
					FD_SET(socket, &read);
					FD_SET(socket, &error);
			
					memset(buffer, 0, sizeof(buffer));
			
					if (select(socket + 1, &read, NULL, &error, NULL) > 0) {
						if (FD_ISSET(socket, &error)) {
							LOG(kProtocolName, liLow, "C[r] %lX: Got socket error", 
								connection);
							snooze(kSleep);
							continue;
						};
						
						if (FD_ISSET(socket, &read)) {
							bytes = recv(socket, buffer, payloadLen - processed, 0);
							if (bytes  > 0) {
								commandBuff += buffer;
								processed += bytes;
							} else {
								if ( (*kMsgr)->IsValid() == false) return B_OK;
		
								LOG(kProtocolName, liLow, "C[r] %lX: Socket got less than 0 (or 0)",
									connection);
								perror("SOCKET ERROR");
		
								BMessage msg(msnmsgCloseConnection);
								msg.AddPointer("connection", con);
				
								connection->fManMsgr.SendMessage(&msg);
								connection->fState = otOffline;
								
								return B_ERROR;
							};					
						};
					};
					
				};
				
//				add payload to message?
				comm->AddPayload( commandBuff.String(), payloadLen, false );
				commandBuff.Remove( 0, payloadLen );
				
				processed = commandBuff.Length();
			};
			
			BMessage dataReady(msnmsgDataReady);
			dataReady.AddPointer("command", comm);
			
			(*kMsgr)->SendMessage(&dataReady);
			
// 			this line needed to process several commands at once if they're in the
//			buffer
			commandLen = commandBuff.FindFirst("\r\n");
		};
	};
	
	return B_OK;
};

status_t MSNConnection::Send(Command *command, queuestyle queue = qsQueue) {
	status_t ret = B_ERROR;

	if (queue == qsImmediate) {
		ret = NetworkSend(command);
	};
	if (queue == qsQueue) {
		fOutgoing.push_back(command);
		ret = B_OK;
	};
	if (queue == qsOnline) {
		fWaitingOnline.push_back(command);
		ret = B_OK;
	};
	
	return ret;
};

int32 MSNConnection::NetworkSend(Command *command) {
	if (fSock > 0) {
		const char * data = command->Flatten(++fTrID);
		int32 data_size = command->FlattenedSize();
		int32 sent_data = 0;
			
		while (sent_data < data_size) {
			int32 sent = send(fSock, &data[sent_data], data_size-sent_data, 0);
			
			if (sent <= 0) {
				delete command;
				LOG(kProtocolName, liLow, "C %lX: Couldn't send packet", this);
				perror("Send Error");
				return sent;
			};
			sent_data += sent;
		};
		
		delete command;
		return sent_data;
	};
	
	return B_ERROR;
};

void MSNConnection::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case msnmsgPulse: {
			if (fOutgoing.size() == 0) return;
			
			Command *c = fOutgoing.front();
			NetworkSend(c);
			
			fOutgoing.pop_front();
		} break;

		case msnmsgPing: {
			if ((fState == otOnline) || (fState == otAway)) {
				Command *keepAlive = new Command("PNG");
				keepAlive->UseTrID(false);
				Send(keepAlive);
			};
		} break;
		
		case msnmsgGetSocket: {	
			BMessage reply(B_REPLY);
			reply.AddInt32("socket", fSock);
			msg->SendReply(&reply);
		} break;
		
		case msnmsgDataReady: {
			Command *command = NULL;
			if (msg->FindPointer(("command"), (void **)&command) == B_OK) {
				ProcessCommand(command);
				delete command;
			};
		} break;
		
		default: {
			BLooper::MessageReceived(msg);
		};
	};
};

ServerAddress MSNConnection::ExtractServerDetails(char *details) {
	char *colon = strchr(details, ':');
	uint16 port = atoi(colon + 1);
	char *server = (char *)calloc((colon - details) + 1, sizeof(char));
	strncpy(server, details, colon - details);
	server[(colon - details)] = '\0';
	
	ServerAddress p(server, port);
	return p;
};

#define CHK_NULL(x) if ((x)==NULL) return -1
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); return -1; }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); return -1; }

status_t MSNConnection::SSLSend(const char *host, HTTPFormatter *send,
	HTTPFormatter **recv) {
	int err;
	int sd;
	struct sockaddr_in sa;
	struct hostent *hp;
	SSL_CTX* ctx;
	SSL*     ssl;
	X509*    server_cert;
	char*    str;
	char     buffer [1024*1024];
	SSL_METHOD *meth;

	SSLeay_add_ssl_algorithms();
	meth = SSLv2_client_method();
	SSL_load_error_strings();
	ctx = SSL_CTX_new (meth);                        CHK_NULL(ctx);

	CHK_SSL(err);
  
	/* ----------------------------------------------- */
	/* Create a socket and connect to server using normal socket calls. */
	
	sd = socket (AF_INET, SOCK_STREAM, 0);       CHK_ERR(sd, "socket");
	
	// clear sa
	memset (&sa, '\0', sizeof(sa));
	
	// get address
	if ((hp= gethostbyname(host)) == NULL) { 
		sa.sin_addr.s_addr = inet_addr (host);   /* Server IP */
	} else {
		memcpy((char *)&sa.sin_addr,hp->h_addr,hp->h_length); /* set address */
	}
	
	sa.sin_family      = AF_INET;
	sa.sin_port        = htons     (443);    /* Server Port number */
	
	err = connect(sd, (struct sockaddr*) &sa,
		sizeof(sa));                   CHK_ERR(err, "connect");
	
	/* ----------------------------------------------- */
	/* Now we have TCP conncetion. Start SSL negotiation. */
  
	ssl = SSL_new (ctx);                         CHK_NULL(ssl);    
	SSL_set_fd (ssl, sd);
	err = SSL_connect (ssl);                     CHK_SSL(err);
    
	/* --------------------------------------------------- */
	/* DATA EXCHANGE - Send a message and receive a reply. */

	err = SSL_write (ssl, send->Flatten(), send->Length());  CHK_SSL(err);

	int received = 0;
	while ( err > 0 )
	{
		err = SSL_read (ssl, &buffer[received], sizeof(buffer) - 1 - received);
		CHK_SSL(err);
		if ( err > 0 )
			received += err;
	}
	buffer[received] = '\0';
	*recv = new HTTPFormatter(buffer, received);
	LOG(kProtocolName, liDebug, "C %lX: Got %d chars:'%s'", this, received, buffer);
	SSL_shutdown (ssl);  /* send SSL/TLS close_notify */
	
	/* Clean up. */
	
	close (sd);
	SSL_free (ssl);
	SSL_CTX_free (ctx);
	
	return received;
};

void MSNConnection::GoOnline(void) {
	CommandQueue::iterator i;

	for (i = fWaitingOnline.begin(); i != fWaitingOnline.end(); i++) {
		fOutgoing.push_back(*i);
	};
	fWaitingOnline.clear();
	
	fState = otOnline;
};

void MSNConnection::ClearQueues(void) {
	CommandQueue::iterator i;
	for (i = fOutgoing.begin(); i != fOutgoing.end(); i++) delete (*i);
	fOutgoing.clear();

	for (i = fWaitingOnline.begin(); i != fWaitingOnline.end(); i++) delete (*i);
	fWaitingOnline.clear();
};

status_t MSNConnection::ProcessCommand(Command *command) {
	if (command->Type() == "VER") {
		return handleVER( command );
	} else
	if ((command->Type() == "NLN") || (command->Type() == "ILN")) {
		return handleNLN( command );
	} else	
	if (command->Type() == "CVR") {
		return handleCVR( command );
	} else	
	if (command->Type() == "RNG") {
		return handleRNG( command );
	} else
	if (command->Type() == "XFR") {
		return handleXFR( command );
	} else
	if (command->Type() == "CHL") {
		return handleCHL( command );
	} else
	if (command->Type() == "USR") {
		return handleUSR( command );
	} else
	if (command->Type() == "MSG") {
		return handleMSG( command );
	} else
	if (command->Type() == "QNG") {
//		We shouldn't ignore this, we should use fKeepAliveRunner->SetInterval()
		return B_OK;
	} else
	if (command->Type() == "ADC") {
		return handleADC(command);
	} else
	if (command->Type() == "LST") {
		return handleLST(command);
	} else
	if (command->Type() == "QRY") {
		return handleQRY(command);
	} else
	if (command->Type() == "GTC") {
		return handleGTC(command);
	} else
	if (command->Type() == "BLP") {
		return handleBLP(command);
	} else
	if (command->Type() == "PRP") {
		return handlePRP(command);
	} else
	if (command->Type() == "CHG") {
		return handleCHG(command);
	} else
	if (command->Type() == "FLN") {
		return handleFLN(command);
	} else
	if (command->Type() == "SYN") {
		return handleSYN(command);
	} else
	if (command->Type() == "JOI") {
		return handleJOI(command);
	} else
	if (command->Type() == "CAL") {
		return handleCAL(command);
	} else
	if (command->Type() == "IRO") {
		return handleIRO(command);
	} else
	if (command->Type() == "ANS") {
		return handleANS(command);
	} else
	if (command->Type() == "BYE") {
		return handleBYE(command);
	} else {
		LOG(kProtocolName, liLow, "C %lX: Got an unsupported message \"%s\"", this, 
			command->Type().String());
		command->Debug();
		return B_ERROR;
	}
};

status_t MSNConnection::handleVER( Command * command ) {
	LOG(kProtocolName, liDebug, "C %lX: Processing VER", this);
	Command *reply = new Command("CVR");
	reply->AddParam(kClientVer);
	reply->AddParam(fManager->Passport());
		
	Send(reply);
		
	return B_OK;
}

status_t MSNConnection::handleNLN( Command * command ) {
//	XXX - We should probably look at the caps and nick	
	BString statusStr = command->Param(0);
	BString passport = command->Param(1);
	BString friendly = command->Param(2);
	BString caps = command->Param(3);
	
	online_types status = otOffline;
	
	if (statusStr == "NLN") status = otOnline;
	if (statusStr == "BSY") status = otBusy;
	if (statusStr == "IDL") status = otIdle;
	if (statusStr == "BRB") status = otBRB;
	if (statusStr == "AWY") status = otAway;
	if (statusStr == "PHN") status = otPhone;
	if (statusStr == "LUN") status = otLunch;
	
	BMessage statusChange(msnmsgStatusChanged);
	statusChange.AddString("passport", passport);
	statusChange.AddInt8("status", (int8)status);
	
	fManMsgr.SendMessage(&statusChange);
	return B_OK;
}

status_t MSNConnection::handleCVR( Command * command ) {
	LOG(kProtocolName, liDebug, "C %lX: Processing CVR", this);
	Command *reply = new Command("USR");
	reply->AddParam("TWN");	// Authentication type
	reply->AddParam("I");	// Initiate
	reply->AddParam(fManager->Passport());
						
	Send(reply);
		
	return B_OK;
}

status_t MSNConnection::handleRNG( Command * command ) {
	LOG(kProtocolName, liDebug, "C %lX: Processing RNG", this);
//		The details are actually param 1, but param 0 will be interpretted as the
//		TrID
	LOG(kProtocolName, liDebug, "C %lX: got a chat invite from %s (%s)",
		this, command->Param(4), command->Param(3));
	ServerAddress sa = ExtractServerDetails((char *)command->Param(0));
	
	BMessage newCon(msnmsgNewConnection);
	newCon.AddString("host", sa.first);
	newCon.AddInt16("port", sa.second);
	newCon.AddString("type", "RNG");
	newCon.AddString("authType", command->Param(1));
	newCon.AddString("authString", command->Param(2));
	newCon.AddString("inviterPassport", command->Param(3));
	newCon.AddString("inviterDisplayName", command->Param(4));
	
	BString temp;
	temp << command->TransactionID();
	newCon.AddString("sessionID", temp.String() );
	
	fManMsgr.SendMessage(&newCon);
	
	return B_OK;
}

status_t MSNConnection::handleXFR( Command * command ) {
	LOG(kProtocolName, liDebug, "C %lX: Processing XFR", this);
	ServerAddress sa = ExtractServerDetails((char *)command->Param(1));
	
	BMessage newCon(msnmsgNewConnection);
	newCon.AddString("host", sa.first);
	newCon.AddInt16("port", sa.second);
	newCon.AddString("type", command->Param(0));
	newCon.AddInt32("trid", command->TransactionID());
	
	if (strcmp(command->Param(0), "NS") == 0) {
		StopReceiver();
			
		ClearQueues();
			
		BMessage closeCon(msnmsgCloseConnection);
		closeCon.AddPointer("connection", this);
			
		fManMsgr.SendMessage(&closeCon);
	};
		
	if (strcmp(command->Param(0), "SB") == 0) {
		newCon.AddString("authType", command->Param(2));
		newCon.AddString("authString", command->Param(3));
	};			

	fManMsgr.SendMessage(&newCon);

	return B_OK;
}

status_t MSNConnection::handleCHL( Command * command ) {
	LOG(kProtocolName, liDebug, "C %lX: Processing CHL", this);
	BString chal = command->Param(0);
	chal << kClientIDCode;
	
	Command *reply = new Command("QRY");
	reply->AddParam(kClientIDString);

	char digest[33];
	
	MD5( (uchar*)chal.String(), digest );
	
	reply->AddPayload(digest, 32);
	
	Send(reply);

	return B_OK;
}

status_t MSNConnection::handleUSR( Command * command ) {
	LOG(kProtocolName, liDebug, "C %lX: Processing USR", this);
	if (strcmp(command->Param(0), "OK") == 0) {
		LOG(kProtocolName, liHigh, "Online!");
		
		GoOnline();
		
		BMessage statusChange(msnmsgOurStatusChanged);
		statusChange.AddInt8("status", fState);
		fManMsgr.SendMessage(&statusChange);

/*		Command *reply = new Command("CHG");
		reply->AddParam("NLN");
		BString caps = "";
		caps << kOurCaps;

		reply->AddParam(caps.String());
		Send(reply);
*/			
		Command *rea = new Command("PRP");
		rea->AddParam("MFN");
		rea->AddParam(fManager->DisplayName(), true);
		Send(rea);

		fManager->Handler()->ContactList(&fContacts);
				
		Command *syn = new Command("SYN");
		syn->AddParam("0");
		syn->AddParam("0");
		Send(syn);
						
		return B_OK;
	};

	HTTPFormatter *send = new HTTPFormatter("nexus.passport.com",
		"/rdr/pprdr.asp");
	HTTPFormatter *recv = NULL;

	int32 recvdBytes = SSLSend("nexus.passport.com", send, &recv);

	LOG(kProtocolName, liHigh, "C %lX: got %i bytes from SSL connection to %s",
		this, recvdBytes, "nexus.passport.com");

	if (recvdBytes < 0) {
		BMessage closeCon(msnmsgCloseConnection);
		closeCon.AddPointer("connection", this);

		fManMsgr.SendMessage(&closeCon);

		return B_ERROR;
	};
	
	
	BString passportURLs = recv->HeaderContents("PassportURLs");
	int32 begin = passportURLs.FindFirst("DALogin=");
	BString loginHost = "";
	if (begin != B_ERROR) {
		int32 end = passportURLs.FindFirst(",", begin);
		passportURLs.CopyInto(loginHost, begin + strlen("DALogin="),
			end - begin - strlen("DALogin="));
	} else {
		delete recv;
		delete send;
		return B_OK;
	};

	BString loginDocument = "";
	begin = loginHost.FindFirst("/");
	loginHost.MoveInto(loginDocument, begin, loginHost.Length() - begin);

//	XXX - We should connect to the host above and get redired around a bit. But
//	That's pissing me off!

	loginHost = "login.passport.com";
	loginDocument = "/login2.srf?lc=1033";
		
	delete send;
	send = new HTTPFormatter(loginHost.String(), loginDocument.String());
	BString authStr = "Passport1.4 OrgVerb=GET,OrgURL=http%3A%2F%2Fmessenger%2Emsn%2Ecom,sign-in=";
	authStr << fManager->Passport() << ",pwd=" << fManager->Password();
	authStr << "," << command->Param(2);
	authStr.ReplaceAll("@", "%40");

	send->AddHeader("Authorization", authStr.String());
	delete recv;
		
	LOG(kProtocolName, liHigh, "C %lX: got %i bytes from SSL connection to %s",
		this, SSLSend(loginHost.String(), send, &recv),
		loginHost.String());
	
	// check status for 302, redirect
	if ( recv->Status() != 200 ) {
		LOG(kProtocolName, liDebug, "C %lX: Got non-200 status: %d", this, recv->Status() );
		
		int repeatCount = 5; // max 5 redirects
		
		while ((recv->Status() == 302) && (repeatCount-- > 0)) {
			BString location = recv->HeaderContents("Location");
			
			location.ReplaceFirst("https://", "");
			int first_slash = location.FindFirst("/");
			location.CopyInto( loginHost, 0, first_slash );
			location.CopyInto( loginDocument, first_slash, location.Length() - first_slash );
			
			LOG(kProtocolName, liHigh, "C %lX: Redirected to (%s) (%s)", this, loginHost.String(), loginDocument.String() );
			
			send = new HTTPFormatter(loginHost.String(), loginDocument.String());
			BString authStr = "Passport1.4 OrgVerb=GET,OrgURL=http%3A%2F%2Fmessenger%2Emsn%2Ecom,sign-in=";
			authStr << fManager->Passport() << ",pwd=" << fManager->Password();
			authStr << "," << command->Param(2);
			authStr.ReplaceAll("@", "%40");
			
			send->AddHeader("Authorization", authStr.String());
			delete recv;
			LOG(kProtocolName, liHigh, "C %lX: got %i bytes from SSL connection to %s",
				this, SSLSend(loginHost.String(), send, &recv),
				loginHost.String());
		}
		
		if ( repeatCount == 0 ) {
			// error, too many redirects
			LOG( kProtocolName, liHigh, "C %lX: got too many redirects when loggin in", this );
			return B_ERROR;
		}
	}
	
//	We get the ticket!
	if (recv->Status() == 200) {
		BString authInfo = recv->HeaderContents("Authentication-Info");
		begin = authInfo.FindFirst("from-PP='");
		BString ticket = "";
		if (begin != B_ERROR) {
			int32 end = authInfo.FindFirst("'", begin + strlen("from-PP='") + 1);
			authInfo.CopyInto(ticket, begin + strlen("from-PP='"),
				end - (begin + strlen("from-PP='")));
		} else {
			return B_ERROR;
		};
				
		Command *reply = new Command("USR");
		reply->AddParam("TWN");
		reply->AddParam("S");
		reply->AddParam(ticket.String());
		Send(reply);
	} else {
		return B_ERROR;
	}
		
	return B_OK;
}

status_t MSNConnection::handleMSG( Command * command ) {
	LOG(kProtocolName, liDebug, "C %lX: Processing MSG", this);
	
	HTTPFormatter http(command->Payload(0), strlen(command->Payload(0)));
	
	const char * type = http.HeaderContents("Content-Type");
	
	if ( type )
	{ // we have a type, handle it.
		if ( strcmp(type, "text/plain; charset=UTF-8") == 0 ) {
			LOG(kProtocolName, liHigh, "C %lX: Got a private message [%s] from <%s>", this, http.Content(), command->Param(0) );
			fManager->fHandler->MessageFromUser( command->Param(0), http.Content() );
		} else
		if (strcmp(type, "text/x-msmsgscontrol") == 0) {
			LOG(kProtocolName, liHigh, "C %lX: User typing from <%s>", this, command->Param(0) );
			fManager->fHandler->UserIsTyping(http.HeaderContents("TypingUser"),	tnStartedTyping);
		} else
		if (strcmp(type, "text/x-msmsgsinitialemailnotification; charset=UTF-8") == 0) {
			LOG(kProtocolName, liHigh, "C %lX: HotMail number of messages in Inbox", this );
			// Ignore this. It just tells us how many emails are in the Hotmail inbox.
		} else
		if (strcmp(type, "text/x-msmsgsprofile; charset=UTF-8") == 0) {
			LOG(kProtocolName, liHigh, "C %lX: Profile message", this );
			// Maybe we should process this, it's a profile message.
		} else
		if (strcmp(type, "text/x-msmsgsinvite; charset=UTF-8") == 0) {
			LOG(kProtocolName, liDebug, "C %lX: Got an invite, we're popular!", this);
			PrintHex((uchar *)command->Payload(0), strlen(command->Payload(0)));
		} else {
			LOG(kProtocolName, liDebug, "C %lX: Got message of unknown type <%s>", this, type );
			PrintHex((uchar *)command->Payload(0), strlen(command->Payload(0)));
		}
	} else {
		LOG(kProtocolName, liDebug, "C %lX: No Content-Type in message!", this);
	}
		
	return B_OK;
}

status_t MSNConnection::handleADC(Command *command) {
	LOG(kProtocolName, liDebug, "C %lX: Processing ADC", this);

	BString listStr = command->Param(0);
	BString passport = command->Param(1, true);
	passport.ReplaceFirst("N=", "");
	BString display = "<unknown display name>";
	if ( command->Params() == 3 )
	{
		display = command->Param(2, true);
		display.ReplaceFirst("F=", "");
	}
	list_types listtype = ltReverseList;
	
	if (listStr == "RL") listtype = ltReverseList;
	
	BMessage requestAuth(msnAuthRequest);
	requestAuth.AddString("displayname", display);
	requestAuth.AddString("passport", passport);
	requestAuth.AddInt8("list", (int8)listtype);
	
	BMessenger(fManager).SendMessage(&requestAuth);
	
	return B_OK;
};

status_t MSNConnection::handleLST(Command *command) {
	LOG(kProtocolName, liDebug, "C %lX: Processing LST", this);
	
	BString passport = command->Param(0);
	passport.ReplaceFirst("N=", "");
	BString display = command->Param(1, true);
	display.ReplaceFirst("F=", "");
	
	if ( command->Params() == 4 )
	{ // this param might not be here if the contact is in no lists
		//	This is a bitmask. 1 = FL, 2 = AL, 4 = BL, 8 = RL
		int32 lists = atol(command->Param(3));
	
		LOG(kProtocolName, liDebug, "C %lX: %s (%s) is in list %i", this, passport.String(), display.String(), lists);

		if (lists == ltReverseList) {
			LOG(kProtocolName, liDebug, "C %lX: \"%s\" (%s) is only on our reverse list. Likely they "
				"added us while we were offline. Ask for authorisation", this, display.String(),
				passport.String());
			fManager->Handler()->AuthRequest(ltReverseList, passport.String(), display.String());
		};
	} else
	{
		LOG(kProtocolName, liDebug, "C %lX: %s (%s) is in no lists", this, passport.String(), display.String() );
	}
	
	return B_OK;
};

status_t MSNConnection::handleQRY(Command *command) {
	// ignore this. It's a challenge response status indicator.
	LOG(kProtocolName, liDebug, "C %lX: Processing QRY", this);
	return B_OK;
}

status_t MSNConnection::handleGTC(Command *command) {
	// ignore this. It's a setting for the client that dictates how
	// to handle people added to the RL. See protocol spec, Notification,
	// Getting details, Privacy settings.
	// Eg: GTC {TrID} GTC N
	LOG(kProtocolName, liDebug, "C %lX: Processing GTC", this);
	return B_OK;
}

status_t MSNConnection::handleBLP(Command *command) {
	// Default list for contact not on either BL or AL.
	// Eg: BLP {TrID} BLP AL
	LOG(kProtocolName, liDebug, "C %lX: Processing BLP", this);
	return B_OK;
}

status_t MSNConnection::handlePRP(Command *command) {
	// Contact phone numbers
	LOG(kProtocolName, liDebug, "C %lX: Processing PRP", this);
	return B_OK;
}

status_t MSNConnection::handleCHG(Command *command) {
	// Own status changed
	LOG(kProtocolName, liDebug, "C %lX: Processing CHG", this);
	
	BString status = command->Param(0);
	
	BMessage statusChange(msnmsgOurStatusChanged);
	
	if ( status == "NLN" ) {
		statusChange.AddInt8("status", otOnline);
	} else
	if ( status == "AWY" ) {
		statusChange.AddInt8("status", otAway);
	} else {
		LOG(kProtocolName, liDebug, "C %lX: Unknown status: %s", this, status.String() );
		statusChange.what = 0; // so we don't send the message.
	}
	
	if ( statusChange.what )
		fManMsgr.SendMessage(&statusChange);
	
	return B_OK;
}

status_t MSNConnection::handleFLN(Command *command) {
	// Contact went offline
	LOG(kProtocolName, liDebug, "C %lX: Processing FLN", this);
	
	BMessage statusChange(msnmsgStatusChanged);
	statusChange.AddString("passport", command->Param(0) );
	statusChange.AddInt8("status", otOffline);
	fManMsgr.SendMessage(&statusChange);
	
	return B_OK;
}


status_t MSNConnection::handleSYN( Command * command ) {
	LOG(kProtocolName, liDebug, "C %lX: Processing SYN", this);
	
	// process SYN here as needed
	// ...
	
	// send CHG to set our status
	Command *reply = new Command("CHG");
	reply->AddParam("NLN");
	BString caps = "";
	caps << kOurCaps;
	reply->AddParam( caps.String() );
	Send(reply);
	
	return B_OK;
}

status_t MSNConnection::handleJOI(Command *command) {
	// Someone joining conversation
	LOG(kProtocolName, liDebug, "C %lX: Processing JOI", this);
	return B_OK;
}

status_t MSNConnection::handleCAL(Command *command) {
	// Inititation response
	LOG(kProtocolName, liDebug, "C %lX: Processing CAL", this);
	return B_OK;
}

status_t MSNConnection::handleIRO(Command *command) {
	// people already in conversation
	LOG(kProtocolName, liDebug, "C %lX: Processing IRO", this);
	return B_OK;
}

status_t MSNConnection::handleANS(Command *command) {
	// Fully connected to chat session
	LOG(kProtocolName, liDebug, "C %lX: Processing ANS", this);
	return B_OK;
}

status_t MSNConnection::handleBYE(Command *command) {
	// someone left conversation
	LOG(kProtocolName, liDebug, "C %lX: Processing BYE", this);
	return B_OK;
}
