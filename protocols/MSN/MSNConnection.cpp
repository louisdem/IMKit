#include "MSNConnection.h"

#include <UTF8.h>

#include <DataIO.h>
#include <sys/select.h>

const char *kClientVer = "0x0409 win 4.10 i386 MSNMSGR 6.0.0602 MSMSGS";
const char *kProtocolsVers = "MSNP10 MSNP9 CVR0";

const int16 kDefaultPort = 1863;
const uint32 kOurCaps = ccUnknown2;

extern const char *kClientIDString = "msmsgs@msnmsgr.com";
extern const char *kClientIDCode = "Q1P7W2E4J9R8U3S5";

void remove_html(char *msg);
void PrintHex(const unsigned char* buf, size_t size);

MSNConnection::MSNConnection(const char *server, uint16 port, MSNManager *man)
: BLooper("MSNConnection looper") {
	fTrID = 0;
	fManager = man;
	fManMsgr = BMessenger(fManager);
	
	uint8 serverLen = strlen(server);
	fServer = (char *)calloc(serverLen + 1, sizeof(char));
	memcpy(fServer, server, serverLen);
	fServer[serverLen] = '\0';
	fPort = port;
	
	fRunner = new BMessageRunner(BMessenger(NULL, (BLooper *)this),
//		new BMessage(msnmsgPulse), 100000, -1);
		new BMessage(msnmsgPulse), 500000, -1);
//		new BMessage(msnmsgPulse), 1000000, -1);

	fKeepAliveRunner = new BMessageRunner(BMessenger(NULL, (BLooper *)this),
		new BMessage(msnmsgPing), 60000000, -1);
	
	fState = otConnecting;
	
	fSock = ConnectTo(fServer, fPort);
	StartReceiver();
};

MSNConnection::~MSNConnection(void) {
	delete fRunner;
	delete fKeepAliveRunner;
	
	free(fServer);
	snooze(1000);
	
	StopReceiver();
	
	if (fSock > 0) close(fSock);
};

bool MSNConnection::QuitRequested() {
	return true;
}

int32 MSNConnection::ConnectTo(const char *hostname, uint16 port) {
	if (hostname == NULL) {
		LOG(kProtocolName, liLow, "ConnectTo() called with NULL hostname - probably"
			" not authorised to login at this time");
		return B_ERROR;
	};

	int32 sock = 0;

	LOG(kProtocolName, liLow, "MSNConn::ConnectTo(\"%s\", %i)", hostname, port);

	struct sockaddr_in remoteAddr;
	remoteAddr.sin_family = AF_INET;

//	XXX - BONE Specific
	if (inet_aton(hostname, &remoteAddr.sin_addr) == 0) {
		struct hostent *he = gethostbyname(hostname);
		if (he) {
			remoteAddr.sin_addr = *((struct in_addr *)he->h_addr_list[0]);
		} else {
			LOG(kProtocolName, liLow, "MSNConn::ConnectTo(%s, %i) couldn't resolve",
				hostname, port);
			return B_ERROR;
		};
	};
    remoteAddr.sin_port = htons(port);
	memset(&(remoteAddr.sin_zero), 0, 8);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG(kProtocolName, liMedium, "MSNConn::ConnectTo(%s, %i): Couldn't create socket",
			hostname, port);	
		return B_ERROR;
	};

	if (connect(sock, (struct sockaddr *)&remoteAddr, sizeof(remoteAddr)) == -1) {
		LOG(kProtocolName, liMedium, "MSNConn::ConnectTo: Couldn't connect to %s:%i", hostname, port);
		return B_ERROR;
	};

	return sock;
};

void MSNConnection::StartReceiver(void) {
	fSockMsgr = new BMessenger(NULL, (BLooper *)this);

	fThread = spawn_thread(Receiver, "MSN socket", B_NORMAL_PRIORITY, (void *)this);
	if (fThread > B_ERROR) resume_thread(fThread);
	
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
	}

	fThread = -1;
};

int32 MSNConnection::Receiver(void *con) {
	MSNConnection *connection = reinterpret_cast<MSNConnection *>(con);

	const uint32 kSleep = 2000000;
	const char *kHost = connection->Server();
	const uint16 kPort = connection->Port();
	const BMessenger *kMsgr = connection->fSockMsgr;
	
	int32 socket = 0;

	if ( !connection->fSockMsgr->IsValid() ) {
		LOG(kProtocolName, liLow, "%s:%i: Messenger wasn't valid!", kHost, kPort);
		return B_ERROR;
	}

	BMessage reply;
	status_t ret = 0;

	if ((ret = connection->fSockMsgr->SendMessage(msnmsgGetSocket, &reply)) == B_OK) {
		if ((ret = reply.FindInt32("socket", &socket)) != B_OK) {
			LOG(kProtocolName, liLow, "%s:%i: Couldn't get socket: %i", kHost, kPort, ret);
			return B_ERROR;
		};
	} else {
		LOG(kProtocolName, liLow, "%s:%i: Couldn't obtain socket: %i", kHost, kPort, ret);
		return B_ERROR;
	}
	
	struct fd_set read;
	struct fd_set error;
	int16 bytes = 0;
	int32 processed = 0;
	char buffer[1024];
	BString commandBuff = "";
	int32 kNewLineLen = strlen("\r\n");
	
	while (kMsgr->IsValid() == true) {
		FD_ZERO(&read);
		FD_ZERO(&error);
		
		FD_SET(socket, &read);
		FD_SET(socket, &error);

		memset(buffer, 0, sizeof(buffer));

		if (select(socket + 1, &read, NULL, &error, NULL) > 0) {
			if (FD_ISSET(socket, &error)) {
				LOG(kProtocolName, liLow, "%s:%i: Got socket error", kHost,
					kPort);
				snooze(kSleep);
				continue;
			};
			
			if (FD_ISSET(socket, &read)) {
				bytes = recv(socket, buffer, sizeof(buffer), 0);
				if (bytes  > 0) {
					commandBuff += buffer;
					processed += bytes;
				} else {
					if (kMsgr->IsValid() == false) return B_OK;
					
					LOG(kProtocolName, liLow, "%s:%i: Socket got less than 0 (or 0)",
						kHost, kPort);
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
				LOG(kProtocolName, liDebug, "%s:%i: Payload of %i, have %i bytes",
					kHost, kPort, payloadLen, processed);
				
				while (processed < payloadLen) {
					FD_ZERO(&read);
					FD_ZERO(&error);
					
					FD_SET(socket, &read);
					FD_SET(socket, &error);
			
					memset(buffer, 0, sizeof(buffer));
			
					if (select(socket + 1, &read, NULL, &error, NULL) > 0) {
						if (FD_ISSET(socket, &error)) {
							LOG(kProtocolName, liLow, "%s:%i: Got socket error", kHost,
								kPort);
							snooze(kSleep);
							continue;
						};
						
						if (FD_ISSET(socket, &read)) {
							bytes = recv(socket, buffer, payloadLen - processed, 0);
							if (bytes  > 0) {
								commandBuff += buffer;
								processed += bytes;
							} else {
								if (kMsgr->IsValid() == false) return B_OK;
		
								LOG(kProtocolName, liLow, "%s:%i: Socket got less than 0 (or 0)",
									kHost, kPort);
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
			
			kMsgr->SendMessage(&dataReady);
			
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
		
//		LOG(kProtocolName, liDebug, "%s:%i: Sending; ", Server(), Port());
//		PrintHex((uchar *)data, data_size);
		
		LOG(kProtocolName, liDebug, "%s:%i: Sending %ld bytes of data", Server(),
			Port(), data_size);
		
		while (sent_data < data_size) {
			int32 sent = send(fSock, &data[sent_data], data_size-sent_data, 0);
			
			if (sent <= 0) {
				delete command;
				LOG(kProtocolName, liLow, "%s:%i: Couldn't send packet", Server(),
					Port());
				perror("Send Error");
				return sent;
			};
			sent_data += sent;
		};
		
		LOG(kProtocolName, liDebug, "%s:%i: Sent %ld bytes of data", Server(), Port(),
			data_size);
		
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

status_t MSNConnection::SSLSend(const char *host, HTTPFormatter *send,
	HTTPFormatter **recv) {

	CRYPT_SESSION session;
	int status = 0;
	int8 retryCount = 0;
	const int8 maxRetries = 100;
	const int8 timeout = 100;	// Seconds
	*recv = NULL;
	
	status = cryptCreateSession(&session, CRYPT_UNUSED, CRYPT_SESSION_SSL);
	status = cryptSetAttribute(session, CRYPT_ATTRIBUTE_BUFFERSIZE, 65536L);
	status = cryptSetAttribute(session, CRYPT_SESSINFO_VERSION, 1);
	status = cryptSetAttributeString(session, CRYPT_SESSINFO_SERVER_NAME,
		host, strlen(host));
	status = cryptSetAttribute(session, CRYPT_OPTION_NET_TIMEOUT, timeout);

	bool connected = false;

	while (connected == false) {
		status = cryptSetAttribute(session, CRYPT_SESSINFO_ACTIVE, 1);
		if (cryptStatusError(status)) {
			retryCount++;
			LOG(kProtocolName, liMedium, "%s:%i: Could not initiate SSL "
				"connection to %s (%i) %i / %i", fServer, fPort, host, status,
				retryCount, maxRetries);

			if (retryCount >= maxRetries) {
				LOG(kProtocolName, liMedium, "%s:%i: Giving up making SSL "
					"connection to %s", fServer, fPort, host);
				cryptDestroySession(session);
				return B_ERROR;
			};
		} else {
			connected = true;
		};
	};

	int sentData = 0;
	
	cryptPushData(session, send->Flatten(), send->Length(), &sentData);
	LOG(kProtocolName, liHigh, "%s:%i: Sent %i bytes to %s over SSL", fServer,
		fPort, sentData, host);

	status = cryptFlushData(session);
	
	char buffer[1024*1024];
	int received = 0;

	status = cryptPopData(session, buffer, sizeof(buffer), &received);
	if (received > 0) {
		buffer[received] = 0;
	} else {
		cryptDestroySession(session);
		return received;
	};
	
	cryptDestroySession(session);
	
	*recv = new HTTPFormatter(buffer, received);
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
		LOG("MSN", liDebug, "Processing VER");
		Command *reply = new Command("CVR");
		reply->AddParam(kClientVer);
		reply->AddParam(fManager->Passport());
		
		Send(reply);
		
		delete command;
		return B_OK;
	};
	
	if ((command->Type() == "NLN") || (command->Type() == "ILN")) {
//		XXX - We should probably look at the caps and nick	
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

		delete command;
		return B_OK;
	};
	
	if (command->Type() == "CVR") {
		LOG("MSN", liDebug, "Processing CVR");
		Command *reply = new Command("USR");
		reply->AddParam("TWN");	// Authentication type
		reply->AddParam("I");	// Initiate
		reply->AddParam(fManager->Passport());
						
		Send(reply);
		
		delete command;
		return B_OK;
	};
	
	if (command->Type() == "RNG") {
		LOG("MSN", liDebug, "Processing RNG");
//		The details are actually param 1, but param 0 will be interpretted as the
//		TrID
		LOG(kProtocolName, liDebug, "%s:%i got a chat invite from %s (%s)",
			fServer, fPort, command->Param(4), command->Param(3));
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
		newCon.AddString("sessionID", temp);
		
		fManMsgr.SendMessage(&newCon);

		delete command;
		return B_OK;
	};
	
	if (command->Type() == "XFR") {
		LOG("MSN", liDebug, "Processing XFR");
		ServerAddress sa = ExtractServerDetails((char *)command->Param(1));

		BMessage newCon(msnmsgNewConnection);
		newCon.AddString("host", sa.first);
		newCon.AddInt16("port", sa.second);
		newCon.AddString("type", command->Param(0));
		
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

		delete command;
		return B_OK;
	};
	
//	Server Challenge
	if (command->Type() == "CHL") {
		LOG("MSN", liDebug, "Processing CHL");
		BString chal = command->Param(0);

		Command *reply = new Command("QRY");
		reply->AddParam(kClientIDString);

		MD5 *md5 = new MD5();
		md5->update((uchar *)chal.String(), (uint16)chal.Length());
		md5->update((uchar *)kClientIDCode, (uint16)strlen(kClientIDCode));
		md5->finalize();

		reply->AddPayload(md5->hex_digest(), 32);

		Send(reply);
	
		delete md5;	
		delete command;	
		return B_OK;
	};
	
	if (command->Type() == "USR") {
		LOG("MSN", liDebug, "Processing USR");
		if (strcmp(command->Param(0), "OK") == 0) {
			LOG(kProtocolName, liHigh, "Online!");

			GoOnline();

			BMessage statusChange(msnmsgOurStatusChanged);
			statusChange.AddInt8("status", fState);
			fManMsgr.SendMessage(&statusChange);

			Command *reply = new Command("CHG");
			reply->AddParam("NLN");
			BString caps = "";
			caps << kOurCaps;

			reply->AddParam(caps.String());
			Send(reply);
			
			Command *rea = new Command("PRP");
			rea->AddParam("MFN");
			rea->AddParam(fManager->DisplayName(), true);
			Send(rea);

			Command *syn = new Command("SYN");
			syn->AddParam("0");
			syn->AddParam("0");
			Send(syn);
						
			delete command;
			return B_OK;
		};

		HTTPFormatter *send = new HTTPFormatter("nexus.passport.com",
			"/rdr/pprdr.asp");
		HTTPFormatter *recv = NULL;

		int32 recvdBytes = SSLSend("nexus.passport.com", send, &recv);

		LOG(kProtocolName, liHigh, "%s:%i got %i bytes from SSL connection to %s",
			fServer, fPort, recvdBytes, "nexus.passport.com");
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

//		XXX - We should connect to the host above and get redired around a bit. But
//		That's pissing me off!

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
		
		LOG(kProtocolName, liHigh, "%s:%i got %i bytes from SSL connection to %s",
			fServer, fPort, SSLSend(loginHost.String(), send, &recv),
			loginHost.String());
//
////		We got redirected.
//		if (recv->Status() == 302) {
//			loginDocument = "";
//			loginHost = recv->HeaderContents("Location");
//			begin = 0;
//		
//			begin = loginHost.FindFirst("//");
//			if (begin != B_ERROR) loginHost.Remove(0, begin + strlen("//"));
//			
//			begin = loginHost.FindFirst("/");
//			if (begin != B_ERROR) {
//				loginHost.MoveInto(loginDocument, begin, begin - loginHost.Length());
//				loginHost.Remove(begin, begin - loginHost.Length());
//			} else {
//				printf("Erk!\n");
//				exit(0);
//			};
//			
//			send = new HTTPFormatter(loginHost.String(), loginDocument.String());
//			BString authStr = "Passport1.4 OrgVerb=GET,OrgURL=http%3A%2F%2Fmessenger%2Emsn%2Ecom,sign-in=";
//			authStr << fManager->Passport() << ",pwd=" << fManager->Password();
//			authStr << "," << command->Param(2);
//
//			delete recv;
//
//			LOG(kProtocolName, liHigh, "%s:%i got %i bytes from SSL connection to %s",
//				fServer, fPort, SSLSend(loginHost.String(), send, &recv),
//				loginHost.String());
//		};
////		We got redirected. Again.
//		if (recv->Status() == 302) {
//			loginDocument = "";
//			loginHost = recv->HeaderContents("Location");
//			begin = 0;
//
//			begin = loginHost.FindFirst("//");
//			if (begin != B_ERROR) loginHost.Remove(0, begin + strlen("//"));
//			
//			begin = loginHost.FindFirst("/");
//			if (begin != B_ERROR) {
//				loginHost.MoveInto(loginDocument, begin, begin - loginHost.Length());
//				loginHost.Remove(begin, begin - loginHost.Length());
//			} else {
//				printf("Erk!\n");
//				exit(0);
//			};
//
//			
//			send = new HTTPFormatter(loginHost.String(), loginDocument.String());
//			BString authStr = "Passport1.4 OrgVerb=GET,OrgURL=http%3A%2F%2Fmessenger%2Emsn%2Ecom,sign-in=";
//			authStr << fManager->Passport() << ",pwd=" << fManager->Password();
//			authStr << "," << command->Param(2);
//
//			LOG(kProtocolName, liHigh, "%s:%i got %i bytes from SSL connection to %s",
//				fServer, fPort, SSLSend(loginHost.String(), send, &recv),
//				loginHost.String());
//		};
////	We get the ticket!
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
			
			delete command;
		};
		
		return B_OK;
	};
	
	if (command->Type() == "MSG") {
/*		LOG(kProtocolName, liDebug, "Processing MSG: command->Payload(0), [%s]");
	
		BString sender = command->Param(0);

		if (sender != "Hotmail") {
			BMessage msgRecvd(msnMessageRecveived);
			msgRecvd.AddString("passport", sender.String());
			msgRecvd.AddString("message", command->Payload(0));
			
			fManMsgr.SendMessage(&msgRecvd);
		};
		
		delete command;
		return B_OK;*/
		
		HTTPFormatter http(command->Payload(0), strlen(command->Payload(0)));
		
		const char * type = http.HeaderContents("Content-Type");
		
		if ( type )
		{ // we have a type, handle it.
			if ( strcmp(type, "text/plain; charset=UTF-8") == 0 ) {
				LOG("MSN", liHigh, "Got a private message [%s] from <%s>\n", http.Content(), command->Param(0) );
				BMessage immsg(IM::MESSAGE);
				immsg.AddString("protocol", "MSN");
				immsg.AddInt32("im_what", IM::MESSAGE_RECEIVED);
				immsg.AddString("id", command->Param(0) );
				immsg.AddString("message", http.Content());
				fManMsgr.SendMessage(&immsg);
			} else {
				LOG("MSN", liDebug, "Got message of unknown type <%s>\n", type );
			}
		} else {
			LOG("ICQ", liDebug, "No Content-Type in message!\n");
		}
		
		delete command;
		return B_OK;
	};
	
//	Unsupported

	LOG(kProtocolName, liLow, "%s:%i got an unsupported message \"%s\"", fServer,
		fPort, command->Type().String());
	PrintHex((uchar *)command->Flatten(0), command->FlattenedSize());
	
	delete command;
	return B_ERROR;
};
