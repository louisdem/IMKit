#include "AIMConnection.h"

#include <UTF8.h>

void remove_html(char *msg);
void PrintHex(const unsigned char* buf, size_t size);
const char kProtocolName[] = "AIM";

AIMConnection::AIMConnection(const char *server, uint16 port, AIMManager *man,
	bool uberDebug = false) {
	
fUberDebug = uberDebug;
	fManager = man;
	fManMsgr = BMessenger(fManager);
	
	uint8 serverLen = strlen(server);
	fServer = (char *)calloc(serverLen + 1, sizeof(char));
	memcpy(fServer, server, serverLen);
	fServer[serverLen] = '\0';
	fPort = port;
	
if (fUberDebug) {
	printf("\n\n\n\nNew connection %s:%i\n", fServer, fPort);
};
	fRunner = new BMessageRunner(BMessenger(NULL, (BLooper *)this),
		new BMessage(AMAN_PULSE), 1000000, -1);	

	fKeepAliveRunner = new BMessageRunner(BMessenger(NULL, (BLooper *)this),
		new BMessage(AMAN_KEEP_ALIVE), 30000000, -1);
	
	fState = AMAN_CONNECTING;
	
	fSock = ConnectTo(fServer, fPort);
	StartReceiver();
	
};

AIMConnection::~AIMConnection(void) {
	free(fServer);
	snooze(1000);
	
	StopReceiver();
	
	if (fThread > 0) kill_thread(fThread);
};

int32 AIMConnection::ConnectTo(const char *hostname, uint16 port) {
	if (hostname == NULL) {
		LOG("AIM", liLow, "ConnectTo() called with NULL hostname - probably"
			" not authorised to login at this time");
		return B_ERROR;
	};

	struct hostent *he;
	struct sockaddr_in their_addr;
	int32 sock = 0;

	LOG("AIM", liLow, "AIMConn::ConnectTo(%s, %i)", hostname, port);

	if ((he = gethostbyname(hostname)) == NULL) {
		LOG("AIM", liMedium, "AIMConn::ConnectTo: Couldn't get Server name (%s)", hostname);
		return B_ERROR;
	};

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG("AIM", liMedium, "AIMConn::ConnectTo(%s, %i): Couldn't create socket",
			hostname, port);	
		return B_ERROR;
	};

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), 0, 8);

	if (connect(sock, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
		LOG("AIM", liMedium, "AIMConn::ConnectTo: Couldn't connect to %s:%i", hostname, port);
		return B_ERROR;
	};
	
	return sock;
};

void AIMConnection::StartReceiver(void) {
	fSockMsgr = new BMessenger(NULL, (BLooper *)this);

	fThread = spawn_thread(Receiver, "DSfgsfdg", B_NORMAL_PRIORITY,
		(void *)this);
	if (fThread > B_ERROR) resume_thread(fThread);
	
};

void AIMConnection::StopReceiver(void) {
	if (fSockMsgr) {
		BMessenger * old_msgr = fSockMsgr;
		fSockMsgr = new BMessenger((BHandler *)NULL);
		delete old_msgr;
	}
	
//	fThread = 0;

};

int32 AIMConnection::Receiver(void *con) {
	AIMConnection *connection = reinterpret_cast<AIMConnection *>(con);

	const uint8 kFLAPHeaderLen = 6;
	const uint32 kSleep = 2000000;
	const char *kHost = connection->Server();
	const char kPort = connection->Port();
	const BMessenger *kMsgr = connection->fSockMsgr;
	
	int32 socket = 0;

	if ( !connection->fSockMsgr->IsValid() ) {
		LOG("AIM", liLow, "%s:%i: Messenger wasn't valid!", kHost, kPort);
		return B_ERROR;
	}

	BMessage reply;

	status_t ret = 0;

	if ((ret = connection->fSockMsgr->SendMessage(AMAN_GET_SOCKET, &reply)) == B_OK) 
	{
		if ((ret = reply.FindInt32("socket", &socket)) != B_OK) 
		{
			LOG("AIM", liLow, "%s:%i: Couldn't get socket: %i", kHost, kPort, ret);
			return B_ERROR;
		}
	} else 
	{
		LOG("AIM", liLow, "%s:%i: Couldn't obtain socket: %i", kHost, kPort, ret);
		return B_ERROR;
	}
	
	struct fd_set read;
	struct fd_set error;
	int16 bytes = 0;
	int32 processed = 0;
	uint16 flapLen = 0;
	uchar flapHeader[kFLAPHeaderLen];
	
	while (kMsgr->IsValid() == true) {
		bytes = 0;
		processed = 0;
						
		while (processed < kFLAPHeaderLen) {
			FD_ZERO(&read);
			FD_ZERO(&error);
			
			FD_SET(socket, &read);
			FD_SET(socket, &error);

			if (select(socket + 1, &read, NULL, &error, NULL) > 0) {
				if (FD_ISSET(socket, &error)) {
					LOG(kProtocolName, liLow, "%s:%i: Got socket error", kHost, kPort);
					snooze(kSleep);
					continue;
				};
				if (FD_ISSET(socket, &read)) {
					if ((bytes = recv(socket, (void *)(flapHeader + processed),
						kFLAPHeaderLen - processed, 0)) > 0) {
						processed += bytes;
					} else {
						if (bytes == 0) {
							snooze(kSleep);
							continue;
						} else {
							if (kMsgr->IsValid() == false) return B_OK;

							LOG(kProtocolName, liLow, "%s:%i: Socket got less than 0",
								kHost, kPort);
							perror("SOCKET ERROR");
							
							BMessage msg(AMAN_CLOSED_CONNECTION);
							msg.AddPointer("connection", con);
			
							connection->fManMsgr.SendMessage(&msg);
							connection->fState = AMAN_OFFLINE;
							
							return B_ERROR;							
						};
					};					
				};
			};
		};

		PrintHex(flapHeader, kFLAPHeaderLen);
		uint8 channel = 0;
		uint16 seqNum = 0;
		uint16 flapLen = 0;
		uchar *flapContents;
		
		if (flapHeader[0] != COMMAND_START) {
			LOG(kProtocolName, liHigh, "%s:%i: Packet header doesn't start with 0x2a "
				" - discarding!", kHost, kPort);
			continue;
		};
	
		channel = flapHeader[1];
		seqNum = (flapHeader[2] << 8) + flapHeader[3];
		flapLen = (flapHeader[4] << 8) + flapHeader[5];
		
		flapContents = (uchar *)calloc(flapLen, sizeof(char));
		
//		printf("FLAP is %i (0x%04x) bytes\n", flapLen, flapLen);
	
		processed = 0;
		bytes = 0;
		
		while (processed < flapLen) {
			FD_ZERO(&read);
			FD_ZERO(&error);
			
			FD_SET(socket, &read);
			FD_SET(socket, &error);

			if (select(socket + 1, &read, NULL, &error, NULL) > 0) {
				if (FD_ISSET(socket, &read)) {
					if ((bytes = recv(socket, (void *)(flapContents + processed),
						flapLen - processed, 0)) > 0) {
						processed += bytes;
					} else {
						if (bytes == 0) {
							snooze(kSleep);
							continue;
						} else {
							if (kMsgr->IsValid() == false) return B_OK;
						
							LOG("AIM", liLow, "%s:%i. Got socket error:",
								connection->Server(), connection->Port());
							perror("SOCKET ERROR");
							
							BMessage msg(AMAN_CLOSED_CONNECTION);
							msg.AddPointer("connection", con);
			
							connection->fManMsgr.SendMessage(&msg);
							connection->fState = AMAN_OFFLINE;
							
							return B_ERROR;							
						};
					};
				};
			};
		};
		
		BMessage dataReady;
		
		switch (channel) {
			case OPEN_CONNECTION: {
				dataReady.what = AMAN_FLAP_OPEN_CON;
			} break;
			case SNAC_DATA: {
				dataReady.what = AMAN_FLAP_SNAC_DATA;
			} break;
			case FLAP_ERROR: {
				dataReady.what = AMAN_FLAP_ERROR;
			} break;
			case CLOSE_CONNECTION: {
				dataReady.what = AMAN_FLAP_CLOSE_CON;
			} break;
			default: {
				LOG(kProtocolName, liHigh, "%s:%i Got an unsupported FLAP channel",
					kHost, kPort);
				continue;
			};
		}

		dataReady.AddInt8("channel", channel);
		dataReady.AddInt16("seqNum", seqNum);
		dataReady.AddInt16("flapLen", flapLen);
		dataReady.AddData("data", B_RAW_TYPE, flapContents, flapLen);

if (connection->fUberDebug) {
	LOG(kProtocolName, liLow, "%s:%i\n", kHost, kPort);
	dataReady.PrintToStream();
}

		kMsgr->SendMessage(&dataReady);

		free(flapContents);
	}

//	delete msgr;

	return B_OK;
};

status_t AIMConnection::Send(Flap *f) {
	fOutgoing.push_back(f);
}

void AIMConnection::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case AMAN_PULSE: {
			if (fSock > 0) {
				if (fOutgoing.size() == 0) return;
				Flap *f = fOutgoing.front();
				if (f->Channel() == SNAC_DATA) {
if (fUberDebug) printf("---- Sending SNAC!!!\n\n");
					LOG("AIM", liLow, "Connection %s:%i 0x%04x / 0x%04x", Server(),
						Port(), f->SNACAt()->Family(), f->SNACAt()->SubType());
				};
				const char * data = f->Flatten(++fOutgoingSeqNum);
				int32 data_size = f->FlattenedSize();
				int32 sent_data = 0;
				
				LOG("AIM", liDebug, "Sending %ld bytes of data", data_size);
				
				while ( sent_data < data_size )
				{
					int32 sent = send(fSock, &data[sent_data], data_size-sent_data, 0);
					
					if ( sent < 0 ) 
					{
						delete f;
						LOG("AIM", liLow, "AIMConn::MessageReceived: Couldn't send packet");
						perror("Socket error");
						return;
					}
					
					if ( sent == 0 )
					{
						LOG("AIM", liHigh, "send() returned 0, is this bad?");
						snooze(1*1000*1000);
					}
					
					sent_data += sent;
				}
				
				LOG("AIM", liDebug, "Sent %ld bytes of data", data_size);
				
				fOutgoing.pop_front();
				delete f;
				
			};
		} break;
		
		case AMAN_KEEP_ALIVE: {
			if ((fState == AMAN_ONLINE) || (fState == AMAN_AWAY)) {
				Flap *keepAlive = new Flap(KEEP_ALIVE);
				Send(keepAlive);
			};
		} break;
		
		case AMAN_GET_SOCKET: {	
			BMessage reply(B_REPLY);
			reply.AddInt32("socket", fSock);
			msg->SendReply(&reply);
		} break;
		
		case AMAN_FLAP_OPEN_CON: {
//			We don't do anything with this currently
			const uchar *data;
			int32 bytes = 0;
			msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);
			LOG(kProtocolName, liLow, "%s:%i: Got FLAP_OPEN_CON packet", Server(),
				Port());
		} break;
		
		case AMAN_FLAP_SNAC_DATA: {
			uint16 seqNum = msg->FindInt16("seqNum");
			uint16 flapLen = msg->FindInt16("flapLen");
			const uchar *data;
			int32 bytes = 0;
			msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);

			
			uint32 offset = 0;
			uint16 family = (data[offset] << 8) + data[++offset];
			uint16 subtype = (data[++offset] << 8) + data[++offset];
			uint16 flags = (data[++offset] << 8) + data[++offset];
			uint32 requestid = (data[++offset] << 24) + (data[++offset] << 16) +
				(data[++offset] << 8) + data[++offset];

			LOG(kProtocolName, liLow, "AIMConn(%s:%i): Got SNAC (0x%04x, 0x%04x)",
				Server(), Port(), family, subtype);
			
			if (family != SERVICE_CONTROL) {
				fManMsgr.SendMessage(msg);
			} else {
				switch (subtype) {
					case VERIFICATION_REQUEST: {
						LOG("AIM", liLow, "AIMConn: AOL sent us a client verification");
						PrintHex((uchar *)data, bytes);
					} break;
						
					case SERVER_SUPPORTED_SNACS: {
						LOG(kProtocolName, liLow, "Got server supported SNACs");
	
						while (offset < bytes) {
							uint16 s = (data[++offset] << 8) + data[++offset];
							LOG(kProtocolName, liLow, "Server supports 0x%04x", s);
							fSupportedSNACs.push_back(s);
						};
	
						fManMsgr.SendMessage(AMAN_NEW_CAPABILITIES);
						
						if (fState == AMAN_CONNECTING) {
							Flap *f = new Flap(SNAC_DATA);
							f->AddSNAC(new SNAC(SERVICE_CONTROL,
								REQUEST_RATE_LIMITS, 0x00, 0x00, ++fRequestID));
							Send(f);
						};
					} break;
					
					case SERVICE_REDIRECT: {
						LOG("AIM", liLow, "Got service redirect SNAC");
						uint16 tlvType = 0;
						uint16 tlvLen = 0;
						char *tlvValue = NULL;
						
						offset = 10;
						
						if (data[4] & 0x80) {
							uint16 skip = (data[10] << 8) + data[11];
							LOG("AIM", liLow, "Skipping %i bytes", skip);
							offset += skip + 2;
						};
						
						printf("Offset: %i - 0x%x\n", offset, data[offset]);
						offset--;
						
						BMessage service(AMAN_NEW_CONNECTION);
						PrintHex((uchar *)data, bytes);
						
						while (offset < bytes) {
							tlvType = (data[++offset] << 8) + data[++offset];
							tlvLen = (data[++offset] << 8) + data[++offset];
							tlvValue = (char *)calloc(tlvLen + 1, sizeof(char));
							memcpy(tlvValue, (void *)(data + offset + 1), tlvLen);
							tlvValue[tlvLen] = '\0';
							
							offset += tlvLen;
	
							switch(tlvType) {
								case 0x000d: {	// Service Family
									if ((tlvValue[0] == 0x00) && (tlvValue[1] == 0x10)) {
//										service.AddBool("icon", true);
										printf("Icon service!\n");
									};
								} break;
								
								case 0x0005: {	// Server Details
									LOG("AIM", liLow, "Server details: %s\n", tlvValue);
	
									if (strchr(tlvValue, ':')) {
										pair<char *, uint16> sd = ExtractServerDetails(tlvValue);
										service.AddString("host", sd.first);
										service.AddInt16("port", sd.second);
										free(sd.first);
									} else {
										service.AddString("host", tlvValue);
										service.AddInt16("port", 5190);
									};
									
								};
								
								case 0x0006: {	// Cookie, nyom nyom nyom!
									for (int i = 0; i < 10; i++) printf("0x%x ", tlvValue[i]);
									printf("\n");
									for (int i = 0; i < 10; i++) printf("0x%x ", tlvValue[(tlvLen - 10)+ i]);
									printf("\n");
								
								
									LOG("AIM", liLow, "Cookie");
									service.AddData("cookie", B_RAW_TYPE, tlvValue,
										tlvLen);
								};
								
								default: {
								};
							};
							
							free(tlvValue);
						};					
						fManMsgr.SendMessage(&service);
						
					};
					
					case RATE_LIMIT_RESPONSE: {
						Flap *f = new Flap(SNAC_DATA);
						f->AddSNAC(new SNAC(SERVICE_CONTROL, RATE_LIMIT_ACK,
							0x00, 0x00, ++fRequestID));
						f->AddRawData((uchar []){0x00, 0x01}, 2);
						f->AddRawData((uchar []){0x00, 0x02}, 2);
						f->AddRawData((uchar []){0x00, 0x03}, 2);
						f->AddRawData((uchar []){0x00, 0x04}, 2);						
						Send(f);
						
						if (fState != AMAN_CONNECTING) return;
						
//						Server won't respond to the Rate ACK above
//						Carry on with login (Send Privacy bits)
						Flap *SPF = new Flap(SNAC_DATA);
						SPF->AddSNAC(new SNAC(SERVICE_CONTROL,
							SET_PRIVACY_FLAGS, 0x00, 0x00, ++fRequestID));
						SPF->AddRawData((uchar []){0x00, 0x00, 0x00, 0x03}, 4); // View everything
						Send(SPF);
						
//						And again... all these messages will be
//						replied to as the server sees fit
						Flap *OIR = new Flap(SNAC_DATA);
						OIR->AddSNAC(new SNAC(SERVICE_CONTROL, UPDATE_STATUS,
							0x00, 0x00, ++fRequestID));
						Send(OIR);
	
						Flap *buddy = new Flap(SNAC_DATA);
						buddy->AddSNAC(new SNAC(BUDDY_LIST_MANAGEMENT,
							ADD_BUDDY_TO_LIST, 0x00, 0x00, ++fRequestID));
						buddy->AddRawData((uchar []){0x00}, 1);
	
						Flap *cap = new Flap(SNAC_DATA);
						cap->AddSNAC(new SNAC(LOCATION, SET_USER_INFORMATION,
							0x00, 0x00, ++fRequestID));
	
						const char *profile = fManager->Profile();
						if (profile != NULL) {
							cap->AddTLV(new TLV(0x0001, kEncoding, strlen(kEncoding)));
							cap->AddTLV(new TLV(0x0002, profile, strlen(profile)));
						};
//						cap->AddTLV(0x0005, "", 0);
						cap->AddTLV(0x0005, kBuddyIconCap, kBuddyIconCapLen);
						Send(cap);
						
						
						Flap *icbm = new Flap(SNAC_DATA);
						icbm->AddSNAC(new SNAC(ICBM, SET_ICBM_PARAMS, 0x00,
							0x00, ++fRequestID));
						icbm->AddRawData((uchar []){0x00, 0x01}, 2); // Plain text?
//						Capabilities - (LSB) Typing Notifications, Missed Calls,
//							Unknown, Messages allowed
						icbm->AddRawData((uchar []){0xff, 0x00, 0x00, 0xff}, 4);
						icbm->AddRawData((uchar []){0x1f, 0x40}, 2);// Max SNAC
						icbm->AddRawData((uchar []){0x03, 0xe7}, 2);// Max Warn - send
						icbm->AddRawData((uchar []){0x03, 0xe7}, 2);// Max Warn - Recv
						icbm->AddRawData((uchar []){0x00, 0x00}, 2);// Min Message interval (sec);
						icbm->AddRawData((uchar []){0x00, 0x64}, 2); //??
	
						Send(icbm);
						
						Flap *cready = new Flap(SNAC_DATA);
						cready->AddSNAC(new SNAC(SERVICE_CONTROL, CLIENT_READY,
							0x00, 0x00, ++fRequestID));
						cready->AddRawData((uchar []){0x00, 0x01, 0x00, 0x03,
							0x01, 0x10, 0x07, 0x39}, 8);
						cready->AddRawData((uchar []){0x00, 0x02, 0x00, 0x01,
							0x01, 0x10, 0x07, 0x39}, 8);
						cready->AddRawData((uchar []){0x00, 0x03, 0x00, 0x01,
							0x01, 0x10, 0x07, 0x39}, 8);
						cready->AddRawData((uchar []){0x00, 0x04, 0x00, 0x01,
							0x01, 0x10, 0x07, 0x39}, 8);
						cready->AddRawData((uchar []){0x00, 0x06, 0x00, 0x01,
							0x01, 0x10, 0x07, 0x39}, 8);
						cready->AddRawData((uchar []){0x00, 0x08, 0x00, 0x01,
							0x01, 0x04, 0x00, 0x01}, 8);
						cready->AddRawData((uchar []){0x00, 0x09, 0x00, 0x01,
							0x01, 0x10, 0x07, 0x39}, 8);
//						Possibly remove.
						cready->AddRawData((uchar []){0x00, 0x10, 0x00, 0x01,
							0x01, 0x10, 0x07, 0x39}, 8);
						cready->AddRawData((uchar []){0x00, 0x13, 0x00, 0x03,
							0x01, 0x10, 0x07, 0x39}, 8);
	
						Send(cready);
						
						Flap *ssi = new Flap(SNAC_DATA);
						ssi->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION,
							REQUEST_LIST, 0x00, 0x00, ++fRequestID));
						Send(ssi);
						
						Flap *useSSI = new Flap(SNAC_DATA);
						useSSI->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION,
							ACTIVATE_SSI_LIST, 0x00, 0x00, ++fRequestID));
						Send(useSSI);
						
						BMessage status(AMAN_STATUS_CHANGED);
						status.AddInt8("status", AMAN_ONLINE);
	
						fManMsgr.SendMessage(&status);
						fState = AMAN_ONLINE;
					
					} break;
					case SERVER_FAMILY_VERSIONS: {
						LOG(kProtocolName, liLow, "Supported SNAC families for "
							"this server");
						while (offset < bytes) {
							LOG(kProtocolName, liLow, "\tSupported family: 0x%x "
								" 0x%x, Version: 0x%x 0x%x", data[++offset],
								data[++offset], data[++offset], data[++offset]);
						};
					} break;
	
					default: {
						fManMsgr.SendMessage(msg);
					};
				};
			};
		} break;
			
		case AMAN_FLAP_ERROR: {
//			We ignore this for now
		} break;
				
		case AMAN_FLAP_CLOSE_CON: {
			const uchar *data;
			int32 bytes = 0;
			msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);

			if (fState == AMAN_CONNECTING) {
	
				char *server = NULL;
				char *cookie = NULL;
				char *value = NULL;

				int32 i = 0;
				uint16 port = 0;
				uint16 cookieSize = 0;
	
				uint16 type = 0;
				uint16 length = 0;
	
				while (i < bytes) {
					type = (data[i] << 8) + data[++i];
					length = (data[++i] << 8) + data[++i];
					value = (char *)calloc(length + 1, sizeof(char));
					memcpy(value, (char *)(data + i + 1), length);
					value[length] = '\0';
	
					switch (type) {
						case 0x0001: {	// Our name, god knows why
						} break;
						
						case 0x0005: {	// New Server:IP
printf("\n\n\n\nRaw details: %s\n", value);
							pair<char *, uint16> serv = ExtractServerDetails(value);
							server = serv.first;
							port = serv.second;
							LOG("AIM", liLow, "Need to reconnect to: %s:%i", server, port);
						} break;
	
						case 0x0006: {
							cookie = (char *)calloc(length, sizeof(char));
							memcpy(cookie, value, length);
							cookieSize = length;
						} break;
					};
	
					free(value);
					i += length + 1;
					
				};
				
	
				StopReceiver();
				
//				Nuke the old packet queue
				fOutgoing.empty();

PrintHex((uchar *)data, bytes);				
printf("Attempting connection to %s:%i\n", server, port);
				fSock = ConnectTo(server, port);

				free(fServer);
				fPort = port;
				fServer = strdup(server);
				
				free(server);
				
				StartReceiver();
				
				Flap *f = new Flap(OPEN_CONNECTION);
				f->AddRawData((uchar []){0x00, 0x00, 0x00, 0x01}, 4); // ID
				f->AddTLV(0x0006, cookie, cookieSize);
				
				Send(f);
			} else {
				
				BMessage msg(AMAN_CLOSED_CONNECTION);
				msg.AddPointer("connection", this);

				fManMsgr.SendMessage(&msg);
				fState = AMAN_OFFLINE;

				StopReceiver();
				fSock = -1;
			};

			
		} break;
		
		default: {
			BLooper::MessageReceived(msg);
		};
	};
};

uint8 AIMConnection::SupportedSNACs(void) const {
	return fSupportedSNACs.size();
};

uint16 AIMConnection::SNACAt(uint8 index) const {
	return fSupportedSNACs[index];
};

bool AIMConnection::Supports(const uint16 family) const {
	vector <const uint16>::iterator i;
		
	for (i = fSupportedSNACs.begin(); i != fSupportedSNACs.end(); i++) {
		if (family == (*i)) return true;
	};

	return false;
};

ServerAddress AIMConnection::ExtractServerDetails(char *details) {
	char *colon = strchr(details, ':');
	uint16 port = atoi(colon + 1);
	char *server = (char *)calloc((colon - details) + 1, sizeof(char));
	strncpy(server, details, colon - details);
	server[(colon - details)] = '\0';
	
	ServerAddress p(server, port);
	return p;
};

