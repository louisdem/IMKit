#include "AIMManager.h"

#include <libim/Protocol.h>
#include <libim/Constants.h>
#include <UTF8.h>

const char kThreadName[] = "IMKit: AIM Protocol";
const char kProtocolName[] = "AIM";

void PrintHex(const unsigned char* buf, size_t size) {
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
	printf("removing html from: [%s]\n", msg);
	
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


AIMManager::AIMManager(BMessenger im_kit) {	
	fSock = -1;
	fOutgoingSeqNum = 10;
	fIncomingSeqNum = 0;
	fRequestID = 0;
	fConnectionState = AMAN_OFFLINE;
	fSockThread = 0;
	
	fIMKit = im_kit;
	fRunner = new BMessageRunner(BMessenger(NULL, (BLooper *)this),
		new BMessage(AMAN_PULSE), 1000000, -1);	

	fKeepAliveRunner = new BMessageRunner(BMessenger(NULL, (BLooper *)this),
		new BMessage(AMAN_KEEP_ALIVE), 30000000, -1);
};

AIMManager::~AIMManager(void) {
	StopMonitor();
	
	delete fRunner;
	delete fKeepAliveRunner;
	
	if ( fSockThread )
	{
		status_t tr;
	
		wait_for_thread( fSockThread, &tr );
	}
}

void AIMManager::StartMonitor(void) {
	fSockMsgr = new BMessenger(NULL, (BLooper *)this);
	fSockThread = spawn_thread(MonitorSocket, kThreadName, B_NORMAL_PRIORITY,
		(void *)this);
	if (fSockThread > B_ERROR) 
		resume_thread(fSockThread);

};

void AIMManager::StopMonitor(void) {
	if (fSockMsgr) 
	{
		BMessenger * old_msgr = fSockMsgr;
		fSockMsgr = new BMessenger((BHandler *)NULL);
		delete old_msgr;
	}
	
	fSockThread = 0;
}

status_t AIMManager::Send(Flap *f) {
	fOutgoing.push_back(f);
}

int32 AIMManager::ConnectTo(const char *hostname, uint16 port) {
	struct hostent *he;
	struct sockaddr_in their_addr;
	int32 sock = 0;

	LOG("AIM", LOW, "AIMManager::ConnectTo(%s, %i)", hostname, port);

	if ((he = gethostbyname(hostname)) == NULL) {
		LOG("AIM", LOW, "AIMManager::ConnectTo: Couldn't get Server name (%s)", hostname);
		return B_ERROR;
	};

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		LOG("AIM", LOW, "AIMManager::ConnectTo: Couldn't create socket");	
		return B_ERROR;
	};

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(port);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), 0, 8);

	if (connect(sock, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
		LOG("AIM", LOW, "AIMManager::ConnectTo: Couldn't connect to %s:%i", hostname, port);
		return B_ERROR;
	};
	
	return sock;
};

status_t AIMManager::Login(const char *server, uint16 port, const char *username,
	const char *password) {
	
	fConnectionState = AMAN_CONNECTING;
	
	fSock = ConnectTo(server, port);
	StartMonitor();
/*
	Flap *flap = new Flap(OPEN_CONNECTION);
	flap->AddRawData((uchar []){0x00, 0x00}, 2);
	flap->AddRawData((uchar []){0x00, 0x01}, 2);
	flap->AddTLV(0x0001, username, strlen(username));
	flap->AddTLV(0x0002, EncodePassword(password), strlen(password));
	flap->AddTLV(0x0003, "BeOS IM Kit by slaad", strlen("BeOS IM Kit by slaad"));
	flap->AddTLV(0x0016, (char []){0x00, 0x01}, 2);
	flap->AddTLV(0x0017, (char []){0x00, 0x00}, 2);
	flap->AddTLV(0x0018, (char []){0x00, 0x00}, 2);
	flap->AddTLV(0x0019, (char []){0x00, 0x00}, 2);
//	flap->AddTLV(0x001a, (char []){0x00, 0x00}, 2);
//	flap->AddTLV(0x0014, (char []){0x20, 0x04, 0x02, 0x16}, 4);
	flap->AddTLV(0x000e, "us", 2);
	flap->AddTLV(0x000f, "en", 2);	
//	flap->AddTLV(0x0009, (char []){0x00, 0x15}, 2);
*/

	Flap *flap = new Flap(OPEN_CONNECTION);
	flap->AddRawData((uchar []){0x00, 0x00, 0x00, 0x01}, 4);
	flap->AddTLV(0x0001, username, strlen(username));

	char *encPass = EncodePassword(password);
	flap->AddTLV(0x0002, encPass, strlen(encPass));
	free(encPass);
	
	flap->AddTLV(0x0003, "BeOS IM Kit: AIM Addon", strlen("BeOS IM Kit: AIM Addon"));
	flap->AddTLV(0x0016, (char []){0x00, 0x01}, 2);
	flap->AddTLV(0x0017, (char []){0x00, 0x04}, 2);
	flap->AddTLV(0x0018, (char []){0x00, 0x02}, 2);
	flap->AddTLV(0x001a, (char []){0x00, 0x00}, 2);
	flap->AddTLV(0x000e, "us", 2);
	flap->AddTLV(0x000f, "en", 2);
	flap->AddTLV(0x0009, (char []){0x00, 0x15}, 2);

	Send(flap);
	
	return B_OK;
};

char *AIMManager::EncodePassword(const char *pass) {
	int32 passLen = strlen(pass);
	char *ret = (char *)calloc(passLen + 1, sizeof(char));
	
//( 0xF3, 0x26, 0x81, 0xC4, 0x39, 0x86, 0xDB, 0x92, 0x71, 0xA3, 0xB9, 
//0xE6, 0x53, 0x7A, 0x95, 0x7C )
	char encoding_table[] = {
		0xf3, 0xb3, 0x6c, 0x99,
		0x95, 0x3f, 0xac, 0xb6,
		0xc5, 0xfa, 0x6b, 0x63,
		0x69, 0x6c, 0xc3, 0x9f
	};

	// encode the password
	for(int32 i = 0; i < passLen; i++ )
		ret[i] = (pass[i] ^encoding_table[i]);
	ret[passLen] = '\0';
		
	return ret;
};

void AIMManager::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case AMAN_PULSE: {
			if (fSock > 0) {
				if (fOutgoing.size() == 0) return;
				Flap *f = fOutgoing.front();
				
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
						LOG("AIM", LOW, "AIMManager::MessageReceived: Couldn't send packet");
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
				
//				PrintHex((uchar *)f->Flatten(fOutgoingSeqNum), f->FlattenedSize());
				fOutgoing.pop_front();
				delete f;
				
			};
		} break;
		
		case AMAN_KEEP_ALIVE: {
			if ((fConnectionState == AMAN_ONLINE) || (fConnectionState == AMAN_AWAY)) {
				Flap *keepAlive = new Flap(KEEP_ALIVE);
				Send(keepAlive);
			};
		} break;
		
		case AMAN_GET_SOCKET: {
			printf("Socket requested\n");
			
			BMessage reply(B_REPLY);
			reply.AddInt32("socket", fSock);
			msg->SendReply(&reply);
		} break;
		
		case AMAN_FLAP_OPEN_CON: {
//			We don't do anything with this currently
			const uchar *data;
			int32 bytes = 0;
			msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);
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

			LOG("AIM", LOW, "AIMManager: Got SNAC (0x%04x, 0x%04x)", family, subtype);
			
			switch(family) {
				case SERVICE_CONTROL: {
					switch (subtype) {
						case VERIFICATION_REQUEST: {
							LOG("AIM", LOW, "AIMManager: AOL sent us a client verification");
							PrintHex((uchar *)data, bytes);
						} break;
							
						case SERVER_SUPPORTED_SNACS: {
//							This is a list of Server supported SNACs
//							We should, in a perfect happy world, parse this
//							and make a note. We aren't going to, we'll just
//							carry on.

							Flap *f = new Flap(SNAC_DATA);
							f->AddSNAC(new SNAC(SERVICE_CONTROL,
								REQUEST_RATE_LIMITS, 0x00, 0x00, ++fRequestID));														
							Send(f);
						} break;
						
						case RATE_LIMIT_RESPONSE: {
							Flap *f = new Flap(SNAC_DATA);
							f->AddSNAC(new SNAC(SERVICE_CONTROL, RATE_LIMIT_ACK,
								0x00, 0x00, ++fRequestID));
							f->AddRawData((uchar []){0x00, 0x01}, 2);
							f->AddRawData((uchar []){0x00, 0x02}, 2);
							f->AddRawData((uchar []){0x00, 0x03}, 2);
							f->AddRawData((uchar []){0x00, 0x04}, 2);						
							Send(f);
							
							if (fConnectionState != AMAN_CONNECTING) return;
							
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
							cap->AddTLV(0x0005, "", 0);
							
							Send(cap);
							
							
							Flap *icbm = new Flap(SNAC_DATA);
							icbm->AddSNAC(new SNAC(ICBM, SET_ICBM_PARAMS, 0x00,
								0x00, ++fRequestID));
							icbm->AddRawData((uchar []){0x00, 0x01}, 2); // Plain text?
//							Capabilities - (LSB) 0b-----tom;
//								Typing notifications
//								Offline Messages
//								Channel allowed
							icbm->AddRawData((uchar []){0x00, 0x00, 0x00, 0x01}, 4);
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
							cready->AddRawData((uchar []){0x00, 0x13, 0x00, 0x03,
								0x01, 0x10, 0x07, 0x39}, 8);


							Send(cready);

							BMessage msg(IM::MESSAGE);
							msg.AddInt32("im_what", IM::STATUS_SET);
							msg.AddString("protocol", kProtocolName);
							msg.AddString("status", ONLINE_TEXT);
						
							fIMKit.SendMessage(&msg);
							fConnectionState = AMAN_ONLINE;
						} break;
					};
				} break;
				
				case BUDDY_LIST_MANAGEMENT: {
					switch (subtype) {
						case USER_ONLINE: {
//							This message contains lots of stuff, most of which we
//							ignore. We're good like that :)
							uint8 nickLen = data[16];
							char *nick = (char *)calloc(nickLen + 1, sizeof(char));
							memcpy(nick, (void *)(data + 17), nickLen);
							nick[nickLen] = '\0';
							
							LOG("AIM", LOW, "AIMManager: \"%s\" is online", nick);
							
							BMessage *msg = new BMessage(IM::MESSAGE);
							msg->AddInt32("im_what", IM::STATUS_CHANGED);
							msg->AddString("protocol", kProtocolName);
							msg->AddString("id", nick);
							msg->AddString("status", ONLINE_TEXT);
							
							fIMKit.SendMessage(msg);

							free(nick);

						} break;
						case USER_OFFLINE: {
							uint8 nickLen = data[16];
							char *nick = (char *)calloc(nickLen + 1, sizeof(char));
							memcpy(nick, (void *)(data + 17), nickLen);
							nick[nickLen] = '\0';
												
							LOG("AIM", LOW, "AIMManager: \"%s\" went offline", nick);
							
							BMessage *msg = new BMessage(IM::MESSAGE);
							msg->AddInt32("im_what", IM::STATUS_CHANGED);
							msg->AddString("protocol", kProtocolName);
							msg->AddString("id", nick);
							msg->AddString("status", OFFLINE_TEXT);
							
							free(nick);
							
						} break;
					};
				} break;
				
				case ICBM: {
					switch (subtype) {
						case MESSAGE_FROM_SERVER: {
							
							uint16 channel = (data[24] << 8) + data[25];
							uint8 nickLen = data[26];
							char *nick = (char *)calloc(nickLen + 1, sizeof(char));

							memcpy(nick, (void *)(data + 27), nickLen);
							nick[nickLen] = '\0';

							int offset = 26 + nickLen;
							uint16 warning = (data[++offset] << 8) + data[++offset];
							uint16 TLVs = (data[++offset] << 8) + data[++offset];
						
							for (uint16 i = 0; i < TLVs; i++) {
								uint16 type = 0;
								uint16 length = 0;
								char *value = NULL;
								
								type = (data[++offset] << 8) + data[++offset];
								length = (data[++offset] << 8) + data[++offset];

								value = (char *)calloc(length + 1, sizeof(char));
								memcpy(value, (void *)(data + offset), length);
								value[length] = '\0';

								free(value);
								offset += length;
							};

//							We only support plain text channels currently									
							switch (channel) {
								case PLAIN_TEXT: {
									//offset--;
									offset += 2; // hack, hack. slaad should look at this :9
									uint32 contentLen = offset + (data[offset+1] << 8) + data[offset+2];
									offset += 2;
									LOG("AIM", HIGH, "AIMManager: PLAIN_TEXT message, content length: %i (0x%0.4X", contentLen-offset, contentLen-offset);
									
									PrintHex(data, bytes );
									
									while ( offset < contentLen ) 
									{
										uint32 tlvlen = 0;
										uint16 msgtype = (data[++offset] << 8) + data[++offset];
										switch ( msgtype) 
										{
											case 0x0501: {	// Client Features, ignore
												tlvlen = (data[++offset] << 8) + data[++offset];
												if ( tlvlen == 1 )
													tlvlen = 0;
												LOG("AIM", DEBUG, "Ignoring Client Features, %ld bytes", tlvlen);
											} break;
											case 0x0101: { // Message Len
												
												if ( data[offset+1] == 1 )
												{ // message from BeMSN
													offset++;
													tlvlen = (data[++offset] << 8) + data[++offset];
												} else												
													tlvlen = (data[++offset] << 8) + data[++offset];
												
												if ( tlvlen == 0 )
												{ // old-style message?
													tlvlen = (data[offset] << 8) + data[offset+1];
													offset++;
													printf("zero size message, new size: %ld\n", tlvlen);
												}
												
												//PrintHex((uchar *)(data + offset + 5), tlvlen);
												char *msg = (char *)calloc(tlvlen - 2, sizeof(char));
												memcpy(msg, (void *)(data + offset + 5), tlvlen - 4);
												msg[tlvlen - 3] = '\0';
												
												remove_html( msg );
												
												LOG("AIM", LOW, "AIMManager: Got message from %s: \"%s\"",
													nick, msg);
												
												BMessage im_msg(IM::MESSAGE);
												im_msg.AddInt32("im_what", IM::MESSAGE_RECEIVED);
												im_msg.AddString("protocol", kProtocolName);
												im_msg.AddString("id", nick);
												im_msg.AddString("message", msg);
												im_msg.AddInt32("charset",B_ISO1_CONVERSION);
												
												fIMKit.SendMessage(&im_msg);	  
												
												free(msg);
											} break;
											
											default:
												LOG("AIM", DEBUG, "Unknown msgtype: %.04x", msgtype);
										}
										offset += tlvlen;
										//i += 4 + tlvlen;
									}
								} break;
							} // end switch(channel)
							
							free(nick);
											
						} break;
					};
				} break;
			};
		} break;
				
		case AMAN_FLAP_ERROR: {
//			We ignore this for now
		} break;
				
		case AMAN_FLAP_CLOSE_CON: {
//			The only CLOSE_CONNECTION event I know of is the redirect with
//			the BOS cookie, let's just assume that's the only one :)
			const uchar *data;
			int32 bytes = 0;
			msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);

			int32 i = 6;
			char *server = NULL;
			uint32 port = 0;
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
						char *colon = strchr(value, ':');
						port = atoi(colon + 1);
						server = (char *)calloc((colon - value) + 1,
							sizeof(char));
						strncpy(server, value, colon - value);
						server[(colon - value)] = '\0';
						
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
			

			StopMonitor();
			
//			Nuke the old packet queue
			fOutgoing.empty();
			
			fSock = ConnectTo(server, port);
			free(server);
			
			StartMonitor();
			
			Flap *f = new Flap(OPEN_CONNECTION);
			f->AddRawData((uchar []){0x00, 0x00, 0x00, 0x01}, 4); // ID
			f->AddTLV(0x0006, cookie, cookieSize);
			
			Send(f);
			
		} break;
		
		default: {
			BLooper::MessageReceived(msg);
		};
	};
};

status_t AIMManager::MessageUser(const char *screenname, const char *message) {
	LOG("AIM", LOW, "AIMManager::MessageUser: Sending \"%s\" (%i) to %s (%i)",
		message, strlen(message), screenname, strlen(screenname));
		
	Flap *f = new Flap(SNAC_DATA);
	f->AddSNAC(new SNAC(ICBM, SEND_MESSAGE_VIA_SERVER, 0x00, 0x00, ++fRequestID));
/*
	f->AddRawData((uchar []){0x00, 0x04}, 2); // Family
	f->AddRawData((uchar []){0x00, 0x06}, 2); // Subtype
	f->AddRawData((uchar []){0x00, 0x00}, 2); // Flags

	f->AddRawData((uchar []){0x00, 0xf0, 0x0e, 0x00}, 4); // ReqID
*/
	f->AddRawData((uchar []){0x00, 0x00, 0xff, 0x00, 0x00, 0x0f, 0x08, 0x03}, 8); // MSG-ID Cookie
	f->AddRawData((uchar []){0x00, 0x01}, 2);

	uint16 l = strlen(screenname);
	f->AddRawData((uchar *)&l, 1);
	f->AddRawData((uchar *)screenname, strlen(screenname));
	f->AddRawData((uchar []){0x00, 0x02}, 2);

	uint16 len = strlen(message);
	l = len + 0x0d;
	l = (l & 0xff00) >> 8;
	f->AddRawData((uchar *)&l, 1);
	l = (len + 0x0d) & 0x00ff;
	f->AddRawData((uchar *)&l, 1);

//	f->AddRawData((uchar []){((l & 0xff00) >> 8), (l & 0x00ff)}, 2);
	l = 0x05;
	f->AddRawData((uchar *)&l, 1);
	f->AddRawData((uchar []){0x01, 0x00}, 2);
	f->AddRawData((uchar []){0x01, 0x01}, 2);
	f->AddRawData((uchar []){0x01, 0x00}, 2);

	l = len + 0x04;
	l = (l & 0xff00) >> 8;
	f->AddRawData((uchar *)&l, 1);
	l = (len + 0x04) & 0x00ff;
	f->AddRawData((uchar *)&l, 1);
//	f->AddRawData((uchar []){(0 & 0xff00) >> 8, (l & 0x00ff)}, 2);
	f->AddRawData((uchar []){0x00, 0x00}, 2);
	f->AddRawData((uchar []){0x00, 0x00}, 2);

	for (uint16 i = 0; i < strlen(message); i++) {
		f->AddRawData((uchar *)&message[i], 1);
	};
//	f->AddRawData((uchar *)&message, len);

	Send(f);

	return B_OK;
};

int32 AIMManager::MonitorSocket(void * manager) {
	AIMManager *mgr = reinterpret_cast<AIMManager *>(manager);
	int32 socket = 0;

	if ( !mgr->fSockMsgr->IsValid() )
	{
		LOG("AIM", LOW, "AIMManager::MonitorSocket: Messenger wasn't valid!\n");
		return B_ERROR;
	}

	BMessage reply;

	status_t ret = 0;
	if ((ret = mgr->fSockMsgr->SendMessage(AMAN_GET_SOCKET, &reply)) == B_OK) 
	{
		if ((ret = reply.FindInt32("socket", &socket)) != B_OK) 
		{
			LOG("AIM", LOW, "AIMManager::MonitorSocket: Couldn't get socket: %i", ret);
			return B_ERROR;
		}
	} else 
	{
		LOG("AIM", LOW, "AIMManager::MonitorSocket: Couldn't obtain socket: %i", ret);
		return B_ERROR;
	}

	struct fd_set read;
	struct fd_set error;
	char buffer[2048];
	//uint16 bufferLen = 2048; using sizeof(buffer) instead
	int32 bytes;
	int32 processed;
	
	while (mgr->fSockMsgr->IsValid()) 
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
				LOG("AIM", LOW, "AIMManager::MonitorSocket: Error reading socket");
			}
				
			if (FD_ISSET(socket, &read)) 
			{
				if ((bytes = recv(socket, buffer, sizeof(buffer), 0)) > 0) 
				{
					LOG("AIM", LOW, "AIMManager::MonitorSocket: Got data (%i bytes)", bytes);
					
					// what does fLen do?
					//uint32 fLen = (buffer[4] << 8) + buffer[5];
						
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
								// unkown message
								break;
						}
							
						dataReady.AddInt32("bytes", bytes);
						dataReady.AddData("data", B_RAW_TYPE, buffer, bytes);
								
						mgr->fSockMsgr->SendMessage(&dataReady);
					}
						
					memset(buffer, 0, bytes);
				} else 
				{ // recv return zero or less
					if ( bytes == 0 )
					{ // no error, just no data. this shouldn't happen though.
						snooze(2000000);
					} else
					{
						LOG("AIM", LOW, "Got socket error:");
						perror("SOCKET ERROR");
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

status_t AIMManager::AddBuddy(const char *buddy) {
	status_t ret = B_ERROR;
	if ((fConnectionState == AMAN_ONLINE) || (fConnectionState == AMAN_AWAY)) {
		LOG("AIM", LOW, "AIMManager::AddBuddy: Adding \"%s\" to list", buddy);
		fBuddy.push_back(BString(buddy));
	
		Flap *addBuddy = new Flap(SNAC_DATA);
		addBuddy->AddSNAC(new SNAC(BUDDY_LIST_MANAGEMENT, ADD_BUDDY_TO_LIST, 0x00,
			0x00, ++fRequestID));
		
		uint8 buddyLen = strlen(buddy);
		addBuddy->AddRawData((uchar [])&buddyLen, 1);
		addBuddy->AddRawData((uchar *)buddy, buddyLen);
		
		Send(addBuddy);
		
		ret = B_OK;
	};

	return ret;
};

int32 AIMManager::Buddies(void) const {
	return fBuddy.size();
};

uchar AIMManager::IsConnected(void) const {
	return fConnectionState;
};

status_t AIMManager::LogOff(void) {
	status_t ret = B_ERROR;
	if (fConnectionState != AMAN_OFFLINE) {
		StopMonitor();
		
		snooze(10000);
		close(fSock);
		fSock = -1;
		fSockThread = -1;
		
		fSockMsgr = NULL;
		
		BMessage msg(IM::MESSAGE);
		msg.AddInt32("im_what",IM::STATUS_SET);
		msg.AddString("protocol","AIM");
		msg.AddString("status", OFFLINE_TEXT);
	
		fIMKit.SendMessage(&msg);
		fConnectionState = AMAN_OFFLINE;

		ret = B_OK;
	};

	return ret;
};
