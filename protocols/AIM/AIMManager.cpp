#include "AIMManager.h"

#include <libim/Protocol.h>
#include <libim/Constants.h>
#include <UTF8.h>

#include "AIMHandler.h"

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

AIMManager::AIMManager(AIMHandler *handler) {	
	fConnectionState = AMAN_OFFLINE;
	
	fHandler = handler;
	fOurNick = NULL;
	fProfile.SetTo("Mikey?");
};

AIMManager::~AIMManager(void) {
	free(fOurNick);

	LogOff();
}

status_t AIMManager::Send(Flap *f) {
	if (f->Channel() == SNAC_DATA) {
		SNAC *s = f->SNACAt(0);
		
		if (s != NULL) {
			uint16 family = s->Family();

			list <AIMConnection *>::iterator i;
			
			for (i = fConnections.begin(); i != fConnections.end(); i++) {
				AIMConnection *con = (*i);
				if (con->Supports(family) == true) {
					LOG("AIM", LOW, "Sending SNAC (0x%04x) via %s:%i", family,
						con->Server(), con->Port());
					con->Send(f);
					return B_OK;
				};
			}
			
			LOG("AIM", LOW, "No connections handle SNAC (0x%04x) requesting service",
				family);
			AIMConnection *con = fConnections.front();
			if (con == NULL) {
				LOG("AIM", HIGH, "No available connections to send SNAC");
			} else {
			
				Flap *newService = new Flap(SNAC_DATA);
				newService->AddSNAC(new SNAC(SERVICE_CONTROL, REQUEST_NEW_SERVICE,
					0x00, 0x00, 0x00000000));
					
				uchar highF = (family & 0xff00) >> 8;
				uchar lowF = (family & 0x00ff);
				newService->AddRawData((uchar *)&highF, sizeof(highF));
				newService->AddRawData((uchar *)&lowF, sizeof(lowF));
				con->Send(newService);

				fWaitingSupport.push_back(f);
				
			};
		};
	} else {
		AIMConnection *con = fConnections.front();
		if (con != NULL) con->Send(f);
	};
};

status_t AIMManager::Login(const char *server, uint16 port, const char *username,
	const char *password) {
	
	if ((username == NULL) || (password == NULL)) return B_ERROR;
	
printf("Connection state (0x%x) needs to be 0x%x\n", fConnectionState, AMAN_OFFLINE);
	if (fConnectionState == AMAN_OFFLINE) {
		uint8 nickLen = strlen(username);
	
		fOurNick = (char *)realloc(fOurNick, (nickLen + 1) * sizeof(char));
		strncpy(fOurNick, username, nickLen);
		fOurNick[nickLen] = '\0';

		fConnectionState = AMAN_CONNECTING;

		Flap *flap = new Flap(OPEN_CONNECTION);

		flap->AddRawData((uchar []){0x00, 0x00, 0x00, 0x01}, 4);
		flap->AddTLV(0x0001, username, nickLen);

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
	
		AIMConnection *c = new AIMConnection(server, port, this);
		c->Run();
		fConnections.push_back(c);
		c->Send(flap);

		return B_OK;
	} else {
		return B_ERROR;
	};
};

char *AIMManager::EncodePassword(const char *pass) {
	int32 passLen = strlen(pass);
	char *ret = (char *)calloc(passLen + 1, sizeof(char));
	
	char encoding_table[] = {
		0xf3, 0xb3, 0x6c, 0x99,
		0x95, 0x3f, 0xac, 0xb6,
		0xc5, 0xfa, 0x6b, 0x63,
		0x69, 0x6c, 0xc3, 0x9f
	};

	// encode the password
	for(int32 i = 0; i < passLen; i++ ) ret[i] = (pass[i] ^encoding_table[i]);

	ret[passLen] = '\0';
		
	return ret;
};

void AIMManager::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case AMAN_NEW_CAPABILITIES: {
			list <Flap *>::iterator i;
			
//			We can cheat here. Just try resending all the items, Send() will
//			take care of finding a connection for it
			for (i = fWaitingSupport.begin(); i != fWaitingSupport.end(); i++) {
				fWaitingSupport.pop_front();
				Send((*i));
			};
		} break;
	
		case AMAN_STATUS_CHANGED: {
			uint8 status = msg->FindInt8("status");
			fHandler->StatusChanged(fOurNick, (online_types)status);
			fConnectionState = status;
		} break;
	
		case AMAN_NEW_CONNECTION: {
			const char *cookie;
			int32 bytes = 0;
			int16 port = 0;
			char *host = NULL;

			if (msg->FindData("cookie", B_RAW_TYPE, (const void **)&cookie, &bytes) != B_OK) return;
			if (msg->FindString("host", (const char **)&host) != B_OK) return;
			if (msg->FindInt16("port", &port) != B_OK) return;
			
			Flap *srvCookie = new Flap(OPEN_CONNECTION);
			srvCookie->AddTLV(new TLV(0x0006, cookie, bytes));

bool icon = msg->FindBool("icon");
printf("Is icon: %i\n", icon);
			AIMConnection *con = new AIMConnection(host, port, this, icon);
			con->Run();
			fConnections.push_back(con);
			
			con->Send(srvCookie);

		} break;
		
		case AMAN_CLOSED_CONNECTION: {
			AIMConnection *con = NULL;
			msg->FindPointer("connection", (void **)&con);
			if (con != NULL) {
				LOG("AIM", LOW, "Connection (%s:%i) closed", con->Server(),
					con->Port());
					
				fConnections.remove(con);
				con->Lock();
				con->Quit();
				LOG("AIM", LOW, "After close we have %i connections",
					fConnections.size());
					
				if (fConnections.size() == 0) {
					fHandler->StatusChanged(fOurNick, AMAN_OFFLINE);
					fConnectionState = AMAN_OFFLINE;
				};
			};
		} break;

		case AMAN_FLAP_OPEN_CON: {
//			We don't do anything with this currently
			const uchar *data;
			int32 bytes = 0;
			msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);
			LOG(kProtocolName, LOW, "Got FLAP_OPEN_CON packet\n");
			PrintHex((uchar *)data, bytes);
		} break;
		
		case AMAN_FLAP_SNAC_DATA: {
			const uchar *data;
			int32 bytes = 0;
			msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);
			
			uint32 offset = 0;
			uint16 family = (data[offset] << 8) + data[++offset];
			uint16 subtype = (data[++offset] << 8) + data[++offset];
			uint16 flags = (data[++offset] << 8) + data[++offset];
			uint32 requestid = (data[++offset] << 24) + (data[++offset] << 16) +
				(data[++offset] << 8) + data[++offset];

			LOG("AIM", LOW, "AIMManager: Got SNAC (0x%04x, 0x%04x)", family, subtype);
			
			switch(family) {
				case SERVICE_CONTROL: {
					LOG(kProtocolName, LOW, "Got an unhandled SNAC of "
						"family 0x0001 (Service Control). Subtype 0x%04x",
						subtype);						
				} break;
				
				case BUDDY_LIST_MANAGEMENT: {
					HandleBuddyList(msg);
				} break;
				
				case ICBM: {
					switch (subtype) {
						case ERROR: {
							LOG(kProtocolName, MEDIUM, "GOT SERVER ERROR 0x%x "
								"0x%x", data[++offset], data[++offset]);
						} break;
						
						case MESSAGE_FROM_SERVER: {
							offset = 18;
							
							uint16 channel = (data[offset] << 8) + data[++offset];
							uint8 nickLen = data[++offset];
							char *nick = (char *)calloc(nickLen + 1, sizeof(char));

							memcpy(nick, (void *)(data + offset + 1), nickLen);
							nick[nickLen] = '\0';

							offset += nickLen;
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
									LOG("AIM", HIGH, "AIMManager: PLAIN_TEXT "
										"message, content length: %i (0x%0.4X)", contentLen-offset, contentLen-offset);
									
									PrintHex(data, bytes );
									
									while ( offset < contentLen ) 
									{
										uint32 tlvlen = 0;
										uint16 msgtype = (data[++offset] << 8) + data[++offset];
										switch ( msgtype) 
										{
											case 0x0501: {	// Client Features, ignore
												tlvlen = (data[++offset] << 8) + data[++offset];
												//if ( tlvlen == 1 )
												//	tlvlen = 0;
												LOG("AIM", DEBUG, "Ignoring Client Features, %ld bytes", tlvlen);
											} break;
											case 0x0101: { // Message Len
												
												/*if ( data[offset+1] == 1 )
												{ // message from BeMSN
													offset++;
													tlvlen = (data[++offset] << 8) + data[++offset];
												} else												
												*/	
												tlvlen = (data[++offset] << 8) + data[++offset];
												
												if ( tlvlen == 0 )
												{ // old-style message?
													tlvlen = (data[offset] << 8) + data[offset+1];
													offset++;
													printf("zero size message, new size: %ld\n", tlvlen);
												}
												
												char *msg = (char *)calloc(tlvlen - 2, sizeof(char));
												memcpy(msg, (void *)(data + offset + 5), tlvlen - 4);
												msg[tlvlen - 3] = '\0';
												
												remove_html( msg );
												
												LOG("AIM", LOW, "AIMManager: Got message from %s: \"%s\"",
													nick, msg);
								
												fHandler->MessageFromUser(nick, msg);
												
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
						
						case TYPING_NOTIFICATION: {
							offset += 8;
							uint16 channel = (data[++offset] << 8) + data[++offset];
							uint8 nickLen = data[++offset];
							char *nick = (char *)calloc(nickLen + 1, sizeof(char));
							memcpy(nick, (char *)(data + offset + 1), nickLen);
							nick[nickLen] = '\0';
							offset += nickLen;
							uint16 typingType = (data[++offset] << 8) + data[++offset];
							
							LOG(kProtocolName, LOW, "Got typing notification "
								"(0x%04x) for \"%s\"", typingType, nick);
								
							fHandler->UserIsTyping(nick, (typing_notification)typingType);
							free(nick);
							
						} break;
						default: {
							LOG(kProtocolName, LOW, "Got unhandled SNAC of family "
								"0x0004 (ICBM) of subtype 0x%04x", subtype);
						};
					};
				} break;
				
				default: {
					LOG(kProtocolName, LOW, "Got unhandled family. SNAC(0x%04x, "
						"0x%04x)", family, subtype);
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

			if (fConnectionState == AMAN_CONNECTING) {
	
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
				
				free(server);
				
				Flap *f = new Flap(OPEN_CONNECTION);
				f->AddRawData((uchar []){0x00, 0x00, 0x00, 0x01}, 4); // ID
				f->AddTLV(0x0006, cookie, cookieSize);
				
				Send(f);
			} else {
				fHandler->StatusChanged(fOurNick, AMAN_OFFLINE);
			};
		} break;
		
		default: {
			BLooper::MessageReceived(msg);
		};
	};
};

status_t AIMManager::HandleBuddyList(BMessage *msg) {
	ssize_t bytes = 0;
	const uchar *data = NULL;
	uint16 offset = 0;
	uint16 subtype = 0;

	msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);
	subtype = (data[2] << 8) + data[3];
	
	offset = 9;

	switch (subtype) {
		case USER_ONLINE: {
//			This message contains lots of stuff, most of which we
//			ignore. We're good like that :)
			PrintHex(data, bytes);
			uint8 nickLen = data[++offset];
			char *nick = (char *)calloc(nickLen + 1, sizeof(char));
			memcpy(nick, (void *)(data + offset + 1), nickLen);
			nick[nickLen] = '\0';
		
			offset += nickLen;
			uint16 warningLevel = (data[++offset] << 8) + data[++offset];

			uint16 tlvs = (data[++offset] << 8) + data[++offset];

			while (offset < bytes) {
				uint16 tlvtype = (data[++offset] << 8) + data[++offset];
				uint16 tlvlen = (data[++offset] << 8) + data[++offset];
				switch (tlvtype) {
					case 0x0001: {	// User class / status
						uint16 userclass = (data[++offset] << 8) +
							data[++offset];

						if ((userclass & CLASS_AWAY) == CLASS_AWAY) {
							fHandler->StatusChanged(nick, AMAN_AWAY);
						} else {
							fHandler->StatusChanged(nick, AMAN_ONLINE);
						};
					} break;
					
					case 0x001d: {	// Icon / available message
						LOG("AIM", LOW, "User %s has icon / available"
							" message.", nick);
						uint16 type;
						uint8 index;
						uint8 length;
						uint16 end = offset + tlvlen;
						
						while (offset < end) {
							type = (data[++offset] << 8) + data[++offset];
							index = data[++offset];
							length = data[++offset];
							switch (type) {
								case 0x0000: {	// Official icon
								} break;
								case 0x0001: {	// Icon
									char *v = (char *)calloc(length, sizeof(char));
									memcpy(v, data + offset, length);
									Flap *buddy = new Flap(SNAC_DATA);
									buddy->AddSNAC(new SNAC(SERVER_STORED_BUDDY_ICONS,
										0x0004, 0x00, 0x00, 0x00000000));
									buddy->AddRawData((uchar *)&nickLen, 1);
									buddy->AddRawData((uchar *)nick, nickLen);
									buddy->AddRawData((uchar []){0x01, 0x00, 0x01, 0x01}, 4);
									buddy->AddRawData((uchar []){0x10}, 1);
									buddy->AddRawData((uchar *)v, 16);
									Send(buddy);
									free(v);
								} break;
								case 0x0002: {
								} break;
								default: {
								};
							};

							offset += length;
						}; 
					} break;
					
					default: {
						offset += tlvlen;
					} break;
				}
			};

			free(nick);

		} break;
		case USER_OFFLINE: {
			offset = 10;
			uint8 nickLen = data[++offset];
			char *nick = (char *)calloc(nickLen + 1, sizeof(char));
			memcpy(nick, (void *)(data + offset), nickLen);
			nick[nickLen] = '\0';
								
			LOG("AIM", LOW, "AIMManager: \"%s\" went offline", nick);
			
			fHandler->StatusChanged(nick, AMAN_OFFLINE);
			free(nick);
			
		} break;
		default: {
			LOG(kProtocolName, LOW, "Got an unhandled SNAC of "
				"family 0x0003 (Buddy List). Subtype 0x%04x",
				subtype);						
		}
	};
};


// -- Interface

status_t AIMManager::MessageUser(const char *screenname, const char *message) {
	LOG("AIM", LOW, "AIMManager::MessageUser: Sending \"%s\" (%i) to %s (%i)",
		message, strlen(message), screenname, strlen(screenname));
		
	Flap *msg = new Flap(SNAC_DATA);
	msg->AddSNAC(new SNAC(ICBM, SEND_MESSAGE_VIA_SERVER, 0x00, 0x00, 0x00000000)); //++fRequestID));
	msg->AddRawData((uchar []){0x00, 0x00, 0xff, 0x00, 0x00, 0x0f, 0x08, 0x03}, 8); // MSG-ID Cookie
	msg->AddRawData((uchar []){0x00, 0x01}, 2); // Channel: Plain Text

	uint8 screenLen = strlen(screenname);
	msg->AddRawData((uchar *)&screenLen, sizeof(screenLen));
	msg->AddRawData((uchar *)screenname, screenLen);

	TLV *msgData = new TLV(0x0002);
	msgData->AddTLV(new TLV(0x0501, "", 0));

	uint16 messageLen = strlen(message);
	char *buffer = (char *)calloc(messageLen + 4, sizeof(char));
	buffer[0] = 0x00;
	buffer[1] = 0x00;
	buffer[2] = 0xff;
	buffer[3] = 0xff;
	memcpy((void *)(buffer + 4), message, messageLen);
	msgData->AddTLV(new TLV(0x101, buffer, messageLen + 4));
	
	free(buffer);
	msg->AddTLV(msgData);
	msg->AddTLV(0x0006, "", 0);
	msg->AddTLV(0x0009, "", 0);
	
	Send(msg);

	return B_OK;
};

status_t AIMManager::AddBuddy(const char *buddy) {
	status_t ret = B_ERROR;
	if (buddy == NULL) return B_ERROR;
//	if ((fConnectionState == AMAN_ONLINE) || (fConnectionState == AMAN_AWAY)) {
		LOG("AIM", LOW, "AIMManager::AddBuddy: Adding \"%s\" to list", buddy);
		fBuddy.push_back(BString(buddy));
	
		Flap *addBuddy = new Flap(SNAC_DATA);
		addBuddy->AddSNAC(new SNAC(BUDDY_LIST_MANAGEMENT, ADD_BUDDY_TO_LIST, 0x00,
			0x00, 0x00000000));
		
		uint8 buddyLen = strlen(buddy);
		addBuddy->AddRawData((uchar [])&buddyLen, sizeof(buddyLen));
		addBuddy->AddRawData((uchar *)buddy, buddyLen);
		
		Send(addBuddy);
		
		ret = B_OK;
//	};

	return ret;
};

status_t AIMManager::AddBuddies(list <char *>buddies) {
	Flap *addBuds = new Flap(SNAC_DATA);
	addBuds->AddSNAC(new SNAC(BUDDY_LIST_MANAGEMENT, ADD_BUDDY_TO_LIST, 0x00,
		0x00, 0x00000000));
	
	list <char *>::iterator i;
	uint8 buddyLen = 0;
	
	for (i = buddies.begin(); i != buddies.end(); i++) {
		char *buddy = (*i);
		buddyLen = strlen(buddy);

		addBuds->AddRawData((uchar *)&buddyLen, sizeof(buddyLen));
		addBuds->AddRawData((uchar *)buddy, buddyLen);
		
		free(buddy);
	};
	
	Send(addBuds);
	
	return B_OK;	
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
		fConnectionState = AMAN_OFFLINE;
		
		LOG("AIM", LOW, "%i connection(s) to kill", fConnections.size());
		list <AIMConnection *>::iterator i;
		
		for (i = fConnections.begin(); i != fConnections.end(); i++) {
			AIMConnection *con = (*i);
			if (con == NULL) continue;
			LOG("AIM", LOW, "Killing connection to %s:%i", con->Server(),
				con->Port());
			con->Lock();
			con->Quit();	
		};

		fHandler->StatusChanged(fOurNick, AMAN_OFFLINE);
		ret = B_OK;
	};

	return ret;
};

status_t AIMManager::RequestBuddyIcon(const char *buddy) {
	LOG(kProtocolName, DEBUG, "Requesting buddy icon for \"%s\"", buddy);
	Flap *icon = new Flap(SNAC_DATA);
//	icon->AddSNAC(new SNAC(0x0010, 0x0004, 0x00, 0x00, ++fRequestID));
	uchar slen = strlen(buddy);
	icon->AddRawData((uchar *)&slen, 1);
	icon->AddRawData((uchar *)buddy, slen);
	icon->AddRawData((uchar []){0x01, 0x00, 0x01, 0x10}, 4);
	icon->AddRawData((uchar []){0x10}, 1); // Icon hash
	icon->AddRawData((uchar []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,	0x00, 0x00, 0x00, 0x00}, 16);

//	Send(icon);

	return B_OK;
};

status_t AIMManager::TypingNotification(const char *buddy, uint16 typing) {
	LOG(kProtocolName, LOW, "Sending typing notification (0x%04x) to \"%s\"",
		typing, buddy);
	
	Flap *notify = new Flap(SNAC_DATA);
//	notify->AddSNAC(new SNAC(ICBM, TYPING_NOTIFICATION, 0x00, 0x00, ++fRequestID));
	notify->AddRawData((uchar []){0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		8); // Notification Cookie
	notify->AddRawData((uchar []){0x00, 0x01}, 2); // Notification channel - Plain Text

	uint8 buddyLen = strlen(buddy);
	notify->AddRawData((uchar *)&buddyLen, sizeof(buddyLen));
	notify->AddRawData((uchar *)buddy, buddyLen);
	notify->AddRawData((uchar *)&typing, sizeof(typing));
	
//	Send(notify);
	
	return B_OK;
};

status_t AIMManager::SetAway(const char *message) {
	Flap *away = new Flap(SNAC_DATA);
	away->AddSNAC(new SNAC(LOCATION, SET_USER_INFORMATION, 0x00, 0x00, 0x00000000));

	if (message) {
		away->AddTLV(new TLV(0x0003, kEncoding, strlen(kEncoding)));
		away->AddTLV(new TLV(0x0004, message, strlen(message)));
		fAwayMsg = message;

		fHandler->StatusChanged(fOurNick, AMAN_AWAY);
		fConnectionState = AMAN_AWAY;
	} else {
		away->AddTLV(new TLV(0x0003, kEncoding, strlen(kEncoding)));
		away->AddTLV(new TLV(0x0004, "", 0));
	
		fAwayMsg = "";
		
		fHandler->StatusChanged(fOurNick, AMAN_ONLINE);
		fConnectionState = AMAN_ONLINE;
	};
		
	Send(away);
};

status_t AIMManager::SetProfile(const char *profile) {
	if (profile == NULL) {
		fProfile = "";
	} else {
		fProfile.SetTo(profile);
	};

	if ((fConnectionState == AMAN_ONLINE) || (fConnectionState == AMAN_AWAY)) {
		Flap *p = new Flap(SNAC_DATA);
		p->AddSNAC(new SNAC(LOCATION, SET_USER_INFORMATION, 0x00, 0x00,
			0x00000000));
		
		if (fProfile.Length() > 0) {
			p->AddTLV(new TLV(0x0001, kEncoding, strlen(kEncoding)));
			p->AddTLV(new TLV(0x0002, fProfile.String(), fProfile.Length()));
		};
		
		if ((fConnectionState == AMAN_AWAY) && (fAwayMsg.Length() > 0)) {
			p->AddTLV(new TLV(0x0003, kEncoding, strlen(kEncoding)));
			p->AddTLV(new TLV(0x0004, fAwayMsg.String(), fAwayMsg.Length()));
		};
				
		Send(p); 
	};
				
	return B_OK;
};
