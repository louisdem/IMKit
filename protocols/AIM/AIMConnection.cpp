#include "AIMConnection.h"

#include <UTF8.h>

void remove_html(char *msg);
void PrintHex(const unsigned char* buf, size_t size);
const char kProtocolName[] = "AIM";

AIMConnection::AIMConnection(const char *server, uint16 port, BMessenger manager) {
	
	fManager = manager;
	
	uint8 serverLen = strlen(server);
	fServer = (char *)calloc(serverLen + 1, sizeof(char));
	memcpy(fServer, server, serverLen);
	fServer[serverLen] = '\0';
	fPort = port;
	
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
		LOG("AIM", LOW, "ConnectTo() called with NULL hostname - probably"
			" not authorised to login at this time");
		return B_ERROR;
	};

	struct hostent *he;
	struct sockaddr_in their_addr;
	int32 sock = 0;

	LOG("AIM", LOW, "AIMConn::ConnectTo(%s, %i)", hostname, port);

	if ((he = gethostbyname(hostname)) == NULL) {
		LOG("AIM", LOW, "AIMConn::ConnectTo: Couldn't get Server name (%s)", hostname);
		return B_ERROR;
	};

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG("AIM", LOW, "AIMConn::ConnectTo(%s, %i): Couldn't create socket",
			hostname, port);	
		return B_ERROR;
	};

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), 0, 8);

	if (connect(sock, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
		LOG("AIM", LOW, "AIMConn::ConnectTo: Couldn't connect to %s:%i", hostname, port);
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

	int32 socket = 0;

	if ( !connection->fSockMsgr->IsValid() ) {
		LOG("AIM", LOW, "AIMConnection::: Messenger wasn't valid!");
		return B_ERROR;
	}

	BMessage reply;

	status_t ret = 0;

	if ((ret = connection->fSockMsgr->SendMessage(AMAN_GET_SOCKET, &reply)) == B_OK) 
	{
		if ((ret = reply.FindInt32("socket", &socket)) != B_OK) 
		{
			LOG("AIM", LOW, "AIMConn::MonitorSocket: Couldn't get socket: %i", ret);
			return B_ERROR;
		}
	} else 
	{
		LOG("AIM", LOW, "AIMConn::MonitorSocket: Couldn't obtain socket: %i", ret);
		return B_ERROR;
	}

	struct fd_set read;
	struct fd_set error;
	uchar buffer[2048];
	int16 bytes = 0;
	int32 processed = 0;
	uint16 flapLen = 0;
	
	while (connection->fSockMsgr->IsValid()) 
	{
		//bytes = 0;
		
		FD_ZERO(&read);
		FD_ZERO(&error);
		
		FD_SET(socket, &read);
		FD_SET(socket, &error);
		
		if (select(socket + 1, &read, NULL, &error, NULL) > 0) 
		{
			if (FD_ISSET(socket, &error)) 
			{
				LOG("AIM", LOW, "AIMConn::MonitorSocket: Error reading socket");
			}
				
			if (FD_ISSET(socket, &read)) 
			{
				if ((bytes = recv(socket, (void *)(buffer + processed),
					sizeof(buffer) - processed, 0)) > 0) 
				{
					LOG("AIM", LOW, "AIMConn::MonitorSocket: Got data (%i bytes)", bytes);
											
					if (buffer[0] == COMMAND_START) 
					{
						BMessage dataReady;
						switch (buffer[1]) 
						{
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
							default:
//								LOG(kProtocolName, LOW, "Got unknown FLAP channel");
								PrintHex((uchar *)buffer, bytes);
								// unkown message
								break;
						}
						
						flapLen = (buffer[4] << 8) + ((uint8)buffer[5]);
						flapLen += 6;
						LOG("AIM", LOW, "%s:%i. Got FLAP. FLAP is %u bytes, read %u bytes",
							connection->Server(), connection->Port(), flapLen, bytes);

						if (flapLen == bytes) {
							dataReady.AddInt32("bytes", bytes);
							dataReady.AddData("data", B_RAW_TYPE, buffer, (uint16)bytes);
							memset(buffer, 0, bytes);
							processed = 0;
						} else {
							if ((uint16)flapLen < (uint16)bytes) {
								dataReady.AddInt32("bytes", flapLen);
								dataReady.AddData("data", B_RAW_TYPE, buffer, flapLen);
								memmove(buffer, (void *)(buffer + flapLen), flapLen);
								processed = flapLen;
							} else {
								LOG("AIM", HIGH, "Huge packet! Abort for now");
								processed = 0;
								continue;
							}
						};
													
						connection->fSockMsgr->SendMessage(&dataReady);
					} else {
						LOG("AIM", LOW, "Got packet without COMMAND_START");
						memset(buffer, 0, sizeof(buffer));
						processed = 0;
					};
						
				} else 
				{ // recv return zero or less
					if ( bytes == 0 )
					{ // no error, just no data. this shouldn't happen though.
						snooze(2000000);
					} else
					{
						LOG("AIM", LOW, "%s:%i. Got socket error:",
							connection->Server(), connection->Port());
						perror("SOCKET ERROR");
						
						BMessage msg(AMAN_CLOSED_CONNECTION);
						msg.AddPointer("connection", con);
		
						connection->fManager.SendMessage(&msg);
						connection->fState = AMAN_OFFLINE;
						
						return B_ERROR;
//						We seem to get here somehow :/
//						LOG("AIMMananager::MonitorSocket", LOW, "Socket error");
					}
				}
			}
		}
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
					LOG("AIM", LOW, "Connection %s:%i 0x%04x / 0x%04x", Server(),
						Port(), f->SNACAt()->Family(), f->SNACAt()->SubType());
				};
				const char * data = f->Flatten(++fOutgoingSeqNum);
				int32 data_size = f->FlattenedSize();
				int32 sent_data = 0;
				
				LOG("AIM", DEBUG, "Sending %ld bytes of data", data_size);
				
				while ( sent_data < data_size )
				{
					int32 sent = send(fSock, &data[sent_data], data_size-sent_data, 0);
					
					if ( sent < 0 ) 
					{
						delete f;
						LOG("AIM", LOW, "AIMConn::MessageReceived: Couldn't send packet");
						perror("Socket error");
						return;
					}
					
					if ( sent == 0 )
					{
						LOG("AIM", HIGH, "send() returned 0, is this bad?");
						snooze(1*1000*1000);
					}
					
					sent_data += sent;
				}
				
				LOG("AIM", DEBUG, "Sent %ld bytes of data", data_size);
				
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
			LOG(kProtocolName, LOW, "Got FLAP_OPEN_CON packet");
		} break;
		
		case AMAN_FLAP_SNAC_DATA: {
			const uchar *data;
			int32 bytes = 0;
			msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);
			
			uint32 offset = 5;
			uint16 family = (data[++offset] << 8) + data[++offset];
			uint16 subtype = (data[++offset] << 8) + data[++offset];
			uint16 flags = (data[++offset] << 8) + data[++offset];
			uint32 requestid = (data[++offset] << 24) + (data[++offset] << 16) +
				(data[++offset] << 8) + data[++offset];

			LOG("AIM", LOW, "AIMConn: Got SNAC (0x%04x, 0x%04x)", family, subtype);
			
			switch(family) {
				case SERVICE_CONTROL: {
					switch (subtype) {
						case VERIFICATION_REQUEST: {
							LOG("AIM", LOW, "AIMConn: AOL sent us a client verification");
							PrintHex((uchar *)data, bytes);
						} break;
							
						case SERVER_SUPPORTED_SNACS: {
							LOG(kProtocolName, LOW, "Got server supported SNACs");

							while (offset < bytes) {
								uint16 s = (data[++offset] << 8) + data[++offset];
								LOG(kProtocolName, LOW, "Server supports 0x%04x", s);
								fSupportedSNACs.push_back(s);
							};

							fManager.SendMessage(AMAN_NEW_CAPABILITIES);
							
							if (fState == AMAN_CONNECTING) {
								Flap *f = new Flap(SNAC_DATA);
								f->AddSNAC(new SNAC(SERVICE_CONTROL,
									REQUEST_RATE_LIMITS, 0x00, 0x00, ++fRequestID));
								Send(f);
							};
						} break;
						
						case SERVICE_REDIRECT: {
							LOG("AIM", LOW, "Got service redirect SNAC");
							uint16 tlvType = 0;
							uint16 tlvLen = 0;
							char *tlvValue = NULL;
							
							if (data[10] & 0x80) {
								uint16 skip = (data[16] << 8) + data[17];
								LOG("AIM", LOW, "Skipping %i bytes", skip);
								offset += skip + 2;
							};
							
							
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
									} break;
									
									case 0x0005: {	// Server Details
										LOG("AIM", LOW, "Server details: %s\n", tlvValue);

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
									
									
										LOG("AIM", LOW, "Cookie");
										service.AddData("cookie", B_RAW_TYPE,
											tlvValue, tlvLen);
									};
									
									default: {
									};
								};
								
								free(tlvValue);
							};
							
							fManager.SendMessage(&service);
							
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
							
//							Server won't respond to the Rate ACK above
//							Carry on with login (Send Privacy bits)
							Flap *SPF = new Flap(SNAC_DATA);
							SPF->AddSNAC(new SNAC(SERVICE_CONTROL,
								SET_PRIVACY_FLAGS, 0x00, 0x00, ++fRequestID));
							SPF->AddRawData((uchar []){0x00, 0x00, 0x00, 0x03}, 4); // View everything
							Send(SPF);
							
//							And again... all these messages will be
//							replied to as the server sees fit
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
//							cap->AddTLV(0x0005, "", 0);
							cap->AddTLV(0x0005, (char []) { 0x09, 0x46, 0x13, 0x46,
								0x4c, 0x7f, 0x11, 0xd1, 0x82, 0x22, 0x44, 0x45, 0x53,
								0x54, 0x00, 0x00}, 16);							
							Send(cap);
							
							
							Flap *icbm = new Flap(SNAC_DATA);
							icbm->AddSNAC(new SNAC(ICBM, SET_ICBM_PARAMS, 0x00,
								0x00, ++fRequestID));
							icbm->AddRawData((uchar []){0x00, 0x01}, 2); // Plain text?
//							Capabilities - (LSB) Typing Notifications, Missed Calls,
//								Unknown, Messages allowed
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
//							Possibly remove.
							cready->AddRawData((uchar []){0x00, 0x10, 0x00, 0x01,
								0x01, 0x10, 0x07, 0x39}, 8);
							cready->AddRawData((uchar []){0x00, 0x13, 0x00, 0x03,
								0x01, 0x10, 0x07, 0x39}, 8);

							Send(cready);
							
							BMessage status(AMAN_STATUS_CHANGED);
							status.AddInt8("status", AMAN_ONLINE);

							fManager.SendMessage(&status);
							fState = AMAN_ONLINE;
						
						} break;
						case SERVER_FAMILY_VERSIONS: {
							LOG(kProtocolName, LOW, "Supported SNAC families for "
								"this server");
							while (offset < bytes) {
								LOG(kProtocolName, LOW, "\tSupported family: 0x%x "
									" 0x%x, Version: 0x%x 0x%x", data[++offset],
									data[++offset], data[++offset], data[++offset]);
							};
						} break;

						default: {
							fManager.SendMessage(msg);
						}
					};
				} break;
				
				default: {
					fManager.SendMessage(msg);
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
	
				int32 i = 6;
				char *server = NULL;
				uint16 port = 0;
				char *cookie = NULL;
				uint16 cookieSize = 0;
	
				uint16 type = 0;
				uint16 length = 0;
				char *value = NULL;
	
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
							pair<char *, uint16> serv = ExtractServerDetails(value);
							server = serv.first;
							port = serv.second;
							LOG("AIM", LOW, "Need to reconnect to: %s:%i", server, port);
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

				fManager.SendMessage(&msg);
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

