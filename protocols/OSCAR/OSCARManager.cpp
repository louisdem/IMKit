#include "OSCARManager.h"

#include <libim/Protocol.h>
#include <libim/Constants.h>
#include <libim/Helpers.h>
#include <UTF8.h>
#include <ctype.h>
#include <string.h>

#include <openssl/md5.h>

#include "FLAP.h"
#include "TLV.h"
#include "Buddy.h"
#include "OSCARConnection.h"
#include "OSCARBOSConnection.h"
#include "OSCARReqConn.h"
#include "OSCARHandler.h"
#include "htmlparse.h"
#include "Group.h"
#include "BufferReader.h"
#include "BufferWriter.h"

#if B_BEOS_VERSION==B_BEOS_VERSION_5
//Not beautiful. An inline function should probably be used instead.
# define strlcpy(_dest,_src,_len) \
	do { \
		if( _dest && _src ) {\
			if(_len>0) {\
				strncpy(_dest,_src,_len-1); \
			} \
		} \
		*(_dest+_len-1)='\0'; \
	}while(0)
#endif		

const uint16 OSCAR_ERROR_COUNT = 0x18;
const char *kErrors[] = {
	"",
	"Invalid SNAC header",
	"Server rate limit exceeded",
	"Client rate limit exceeded",
	"Recipient is not logged in",
	"Requested service unavailable",
	"Requested service undefined",
	"An old SNAC was sent",
	"Command not supported by server",
	"Command not supprted by client",
	"Refused by client",
	"Reply too big",
	"Responses lost",
	"Request denied",
	"Malformed SNAC",
	"Insufficient rights",
	"Recipient blocked",
	"Sender too evil",
	"Receiver too evil",
	"User unavailable",
	"No match",
	"List overflow",
	"Request ambiguous",
	"Server queue full",
	"Not while on my watch ... err... AOL"
};

const uint16 kSSIResultCount = 14;
const char *kSSIResult[] = {
	"Success",
	"N/A",
	"Item not found in list",
	"Item already exists",
	"N/A",
	"N/A",
	"N/A",
	"N/A",
	"N/A",
	"N/A",
	"Error adding - invalid ID / already in list / invalid data",
	"N/A",
	"Can't add item, limit exceeded",
	"ICQ contacts cannot be added to AIM list",
	"Requires authorisation"
};


void PrintHex(const unsigned char* buf, size_t size, bool override = false) {
	if ((g_verbosity_level != liDebug) && (override == false)){
		// only print this stuff in debug mode
		return;
	}
	
	uint16 i = 0;
	uint16 j = 0;
	int breakpoint = 0;

	for(;i < size; i++) {
		fprintf(stdout, "%02x ", (unsigned char)buf[i]);
		breakpoint++;	

		if(!((i + 1)%16) && i) {
			fprintf(stdout, "\t\t");
			for(j = ((i+1) - 16); j < ((i+1)/16) * 16; j++)	{
				if ((buf[j] < 0x21) || (buf[j] > 0x7d)) {
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

void MD5(const char *data, int length, char *result) {
	MD5state_st state;
	
	MD5_Init(&state);
	MD5_Update(&state, data, length);
	MD5_Final((uchar *)result, &state);
};

//#pragma mark -

OSCARManager::OSCARManager(OSCARHandler *handler) {	
	fConnectionState = OSCAR_OFFLINE;
	
	fHandler = handler;
	fOurNick = NULL;
	fProfile.SetTo("IMKit OSCAR");
	
	fIcon = NULL;
	fIconSize = -1;
};

OSCARManager::~OSCARManager(void) {
	free(fOurNick);
	if (fIcon) free(fIcon);

	LogOff();
}

//#pragma mark -

status_t OSCARManager::ClearConnections(void) {
	LOG(kProtocolName, liLow, "%i pending connections to close",
		fPendingConnections.size());

	pfc_map::iterator pIt;
	for (pIt = fPendingConnections.begin(); pIt != fPendingConnections.end(); pIt++) {
		OSCARReqConn *con = dynamic_cast<OSCARReqConn *>(pIt->second);
		if (con == NULL) continue;
		BMessenger(con).SendMessage(B_QUIT_REQUESTED);
	};
	fPendingConnections.clear();


	LOG(kProtocolName, liLow, "%i used connections to close", fConnections.size());

	connlist::iterator it;
	for (it = fConnections.begin(); it != fConnections.end(); it++) {
		OSCARConnection *con = (*it);
		if (con == NULL) continue;

		BMessenger(con).SendMessage(B_QUIT_REQUESTED);
	};	
	fConnections.clear();
	
	return B_OK;
};

status_t OSCARManager::ClearWaitingSupport(void) {
	flap_stack::iterator it;
	
	for (it = fWaitingSupport.begin(); it != fWaitingSupport.end(); it++) {
		Flap *f = (*it);
		delete f;
	};
	
	fWaitingSupport.clear();
	
	return B_OK;
};

status_t OSCARManager::HandleServiceControl(SNAC *snac, BufferReader *reader) {
	status_t ret = B_OK;
	
	int16 subtype = snac->SubType();
	int32 request = snac->RequestID();

	reader->OffsetTo(snac->DataOffset());
	
	switch (subtype) {
		case OWN_ONLINE_INFO: {
//			For some reason we're getting this upon sending an icon upload request
//			We should be getting a EXTENDED_STATUS

			if ((fIcon) && (fIconSize > 0)) {
				Flap *icon = new Flap(SNAC_DATA);
				icon->AddSNAC(new SNAC(SERVER_STORED_BUDDY_ICONS, ICON_UPLOAD_REQ));
				icon->AddInt16(fSSIItems++);		// First icon
				icon->AddInt16(fIconSize);
				icon->AddRawData((uchar *)fIcon, fIconSize);
							
				Send(icon);
			};
		} break;

		case EXTENDED_STATUS: {
			while (reader->Offset() < reader->Length()) {
				int16 notice = reader->ReadInt16();

				switch (notice) {
//					Official icon
					case 0x0000: {
						int16 length = reader->ReadInt16();
						reader->OffsetBy(length);
					} break;
					case 0x0001: {
						uint8 flags = reader->ReadInt8();
						int8 hashLen = reader->ReadInt8();
						uchar *currentHash = reader->ReadData(hashLen);
						
						if ((fIcon) && (fIconSize > 0)) {
							uchar hash[hashLen];
							MD5((uchar *)fIcon, fIconSize, (uchar *)hash);
							
							if (memcmp(hash, currentHash, hashLen) != 0) {
								LOG(kProtocolName, liLow, "Server stored buddy "
									"icon is different to ours - uploading");
								Flap *upload = new Flap(SNAC_DATA);
								upload->AddSNAC(new SNAC(SERVER_STORED_BUDDY_ICONS,
									 ICON_UPLOAD_REQ));
								upload->AddInt16(fSSIItems++);	// Next SSI item
								upload->AddInt16(fIconSize);
								upload->AddRawData((uchar *)fIcon, fIconSize);
								
								Send(upload);
							};
						};
						
						free(currentHash);
					} break;
				};
			};
			
		} break;
		
		case MOTD: {
			PrintHex(reader->Buffer(), reader->Length(), true);
			ret = B_OK;
		} break;
		
		case RATE_LIMIT_WARNING: {
			LOG(kProtocolName, liHigh, "Rate limit warning!");
		} break;
		
		default: {
			ret = kUnhandled;
		};
	};

	return ret;
};

status_t OSCARManager::HandleICBM(SNAC *snac, BufferReader *reader) {
	status_t ret = B_OK;
	
	uint16 subtype = snac->SubType();
	uint32 request = snac->RequestID();
	
	reader->OffsetTo(snac->DataOffset());
	
	switch (subtype) {
		case ERROR: {
			LOG(kProtocolName, liHigh, "Server error 0x%04x", reader->ReadInt16());
		} break;
		
		case MESSAGE_FROM_SERVER: {
			int64 messageId = reader->ReadInt64();
			uint16 channel = reader->ReadInt16();
			int8 nickLen = reader->ReadInt8();
			char *nick = reader->ReadString(nickLen);
			uint16 warning = reader->ReadInt16();
			uint16 tlvCount = reader->ReadInt16();
			bool autoReply = false;
			int16 userStatus = 0x0000;
			int16 userClass = 0x0000;
			char *message = NULL;

			for (int32 i = 0; i < tlvCount; i++) {
				TLV tlv(reader);
				BufferReader *tlvReader = tlv.Reader();
				
				switch (tlv.Type()) {
					case 0x0001: {	// Userclass
						userClass = tlvReader->ReadInt16();
						printf("USer class: 0x%04x\n", userClass);
						
						LOG(kProtocolName, liHigh, "Got user class: 0x%04x", userClass);
					} break;
					case 0x0004: {	// Automated reply
						autoReply = true;
					} break;
					case 0x0006: {	// User status
						printf("User status-pre: 0x%04x\n", tlvReader->ReadInt16()); // Webaware status, etc
						userStatus = tlvReader->ReadInt16();
						printf("User status: 0x%04x\n");
					} break;
				};
				
				delete tlvReader;
			};
					
			// We currently only handle plain text messages
			if (channel == PLAIN_TEXT) {		
				// There should only be one bit TLV here, but who knows
				while (reader->Offset() < reader->Length()) {
					TLV tlv(reader);
					BufferReader *tlvReader = tlv.Reader();

					if (tlv.Type() == 0x0002) {
						while (tlvReader->Offset() < tlvReader->Length()) {
							int8 id = tlvReader->ReadInt8();
							int8 version = tlvReader->ReadInt8();
							int16 length = tlvReader->ReadInt16();
												
							tlvReader->Debug();
												
							// If it's not the text fragment, just skip it
							if (id == 0x01) {
								uint16 charSet = tlvReader->ReadInt16();
								uint16 charSubset = tlvReader->ReadInt16();
								uint16 messageLen = length - (sizeof(int16) * 2);
								message = tlvReader->ReadString(messageLen);
								
								if (charSet == 0x0002){
									LOG(kProtocolName, liLow, "Got a UTF-16 encoded message");
									char *msg16 = (char *)calloc(messageLen,
										sizeof(char));
									int32 state = 0;
									int32 utf8Size = length * 2;
									int32 ut16Size = length;
									memcpy(msg16, message, length);
									message = (char *)realloc(message, utf8Size *
										sizeof(char));
									
									convert_to_utf8(B_UNICODE_CONVERSION, msg16,
										&ut16Size, message, &utf8Size, &state);
									message[utf8Size] = '\0';
									length = utf8Size;
				
									free(msg16);
					
									LOG(kProtocolName, liLow, "Converted message: \"%s\"",
										message);
								};
							} else {
								tlvReader->OffsetBy(length);
							};
						};
					};

					delete tlvReader;
				};
			} else {
				LOG(kProtocolName, liHigh, "Message on non-plain text channel!");
			};
			
			if (message) {
				parse_html(message);
										
				// If the user is invisible, get their real status and update
				if ((userStatus & STATUS_INVISIBLE) == STATUS_INVISIBLE) {
					uint16 statusMask = userStatus & 0x00ff;
					online_types status = OSCAR_AWAY;
					
					if (statusMask & STATUS_FREE_FOR_CHAT) status = OSCAR_ONLINE;
					if (statusMask == STATUS_ONLINE) status = OSCAR_ONLINE;
					
					fHandler->StatusChanged(nick, status);
				};

				fHandler->MessageFromUser(nick, message, autoReply);
			};

			free(nick);
			free(message);
		} break;
		
		case TYPING_NOTIFICATION: {
			uint64 id = reader->ReadInt64();
			uint16 channel = reader->ReadInt16();
			uint8 nickLen = reader->ReadInt8();
			char *nick = reader->ReadString(nickLen);
			uint16 type = reader->ReadInt16();
			
			LOG(kProtocolName, liDebug, "Got typing notification (0x%04x) for "
				"\"%s\"", type, nick);
			
			fHandler->UserIsTyping(nick, (typing_notification)type);
			free(nick);
			
		} break;
		default: {
			LOG(kProtocolName, liMedium, "Got unhandled SNAC of family 0x0004 "
				"(ICBM) of subtype 0x%04x", subtype);
			ret = kUnhandled;
		};
	};
	
	return ret;
};

status_t OSCARManager::HandleBuddyList(SNAC *snac, BufferReader *reader) {
	status_t ret = B_OK;
	
	int16 subtype = snac->SubType();
	int32 request = snac->RequestID();
	
	reader->OffsetTo(snac->DataOffset());

	switch (subtype) {
		case USER_ONLINE: {
		
		
			// There can be multiple users in a packet
			while (reader->Offset() < reader->Length()) {
				//	This message contains lots of stuff, most of which we
				//	ignore. We're good like that :)
				uint8 nickLen = reader->ReadInt8();
				char *nick = reader->ReadString(nickLen);
				uint16 userclass = 0;
				Buddy *buddy = NULL;
				buddymap::iterator bIt = fBuddy.find(nick);
				
				if (bIt == fBuddy.end()) {
					buddy = new Buddy(nick, -1);
					fBuddy[nick] = buddy;
				} else {
					buddy = bIt->second;
				};
				
				//	These are currently unused.
				uint16 warningLevel = reader->ReadInt16();
				uint16 tlvs = reader->ReadInt16();
	
				for (int32 i = 0; i < tlvs; i++) {
					TLV tlv(reader);
					BufferReader *tlvReader = tlv.Reader();
					
					switch (tlv.Type()) {
						case 0x0001: {	// User class / status
							userclass = tlvReader->ReadInt16();
						} break;
						
						case 0x001d: {	// Icon / available message
							LOG(kProtocolName, liLow, "User %s has an icon / available message",
								nick);
							uint16 type = tlvReader->ReadInt16();
							uint8 flags = tlvReader->ReadInt8();
							uint8 hashLen = tlvReader->ReadInt8();
							uchar *hash = tlvReader->ReadData(hashLen);
							
							
							Flap *icon = new Flap(SNAC_DATA);
							icon->AddSNAC(new SNAC(SERVER_STORED_BUDDY_ICONS,
								AIM_ICON_REQUEST));
							icon->AddInt8(nickLen);
							icon->AddRawData((uchar *)nick, nickLen);
							icon->AddInt8(0x01);	// Command
							icon->AddInt16(type);	// Icon Id
							icon->AddInt8(flags);
							icon->AddInt8(hashLen);
							icon->AddRawData(hash, hashLen);
							
							Send(icon);
							
							free(hash);
						} break;
						
						case 0x000d: {
							while (tlvReader->HasMoreData()) {
								int32 caplen = 16;
								char *cap = (char *)tlvReader->ReadData(caplen);
								
								if (buddy->HasCapability(cap, caplen) == false) {
									buddy->AddCapability(cap, caplen);
								};
								
								free(cap);
							};
						} break;
					};
					
					delete tlvReader;
				};			
				
				buddy->SetUserclass(userclass);
				
				if ((userclass & CLASS_AWAY) == CLASS_AWAY) {
					fHandler->StatusChanged(nick, OSCAR_AWAY, buddy->IsMobileUser());
				} else {
					fHandler->StatusChanged(nick, OSCAR_ONLINE, buddy->IsMobileUser());
				};
			
				free(nick);
			}
		} break;
		case USER_OFFLINE: {
			uint8 nickLen = reader->ReadInt8();
			char *nick = reader->ReadString(nickLen);
			buddymap::iterator bIt = fBuddy.find(nick);
			if (bIt != fBuddy.end()) bIt->second->ClearCapabilities();
								
			LOG(kProtocolName, liLow, "OSCARManager: \"%s\" went offline", nick);
			
			fHandler->StatusChanged(nick, OSCAR_OFFLINE);
			free(nick);
		} break;
		default: {
			LOG(kProtocolName, liMedium, "Got an unhandled SNAC of family 0x0003 "
				"(Buddy List). Subtype 0x%04x", subtype);
			ret = kUnhandled;
		}
	};
	
	return ret;
};

status_t OSCARManager::HandleSSI(SNAC *snac, BufferReader *reader) {
	status_t ret = B_OK;

	int16 subtype = snac->SubType();
	int32 request = snac->RequestID();

	reader->OffsetTo(snac->DataOffset());
	
	switch (subtype) {
		case SERVICE_PARAMETERS: {
			TLV limits(reader);
			BufferReader *tlvReader = limits.Reader();
				
			if (limits.Type() == 0x0004) {
				// SSI Params
				for (int32 i = 0; i < kSSILimitCount; i++) {
					fSSILimits[i] = tlvReader->ReadInt16();
					LOG(kProtocolName, liHigh, "SSI Limit: %i: %i", i, fSSILimits[i]);
				};
			};
			
			delete tlvReader;
		} break;
		case ROSTER_CHECKOUT: {	
			list <BString> contacts;
		
			uint8 ssiVersion = reader->ReadInt8();
			uint16 itemCount = reader->ReadInt16();

			LOG(kProtocolName, liDebug, "SSI Version 0x%x", ssiVersion);
			LOG(kProtocolName, liLow, "%i SSI items", itemCount);

			fSSIItems = itemCount;

			for (uint16 i = 0; i < itemCount; i++) {
				LOG(kProtocolName, liHigh, "Item %i / %i", i, itemCount);
				
				uint16 nameLen = reader->ReadInt16();
				char *name = reader->ReadString(nameLen);
							
				uint16 groupID = reader->ReadInt16();
				uint16 itemID = reader->ReadInt16();
				uint16 type = reader->ReadInt16();
				uint16 len = reader->ReadInt16();
				
				fItemIds[itemID] = true;
				
				LOG(kProtocolName, liLow, "SSI item %i is of type 0x%04x (%i bytes)",
					 i, type, len);
				
				switch (type) {
					case GROUP_RECORD: {
						int32 end = reader->Offset() + len;
						
						printf("Should end at %i\n", end);
						
						while (reader->Offset() < end) {
							printf("Offset: %i\n", reader->Offset());
							
							TLV tlv(reader);
							int16 size = tlv.Length();
							BufferReader *tlvReader = tlv.Reader();
						
							Group *group = new Group(groupID, name);
						
							LOG(kProtocolName, liHigh, "Group %s (0x%04x)", name, groupID);
	
							// Bunch o' groups
							if (tlv.Type() == 0x00c8) {
								for (int32 i = 0; i < (size / 2); i++) {
									int16 child = tlvReader->ReadInt16();
									LOG(kProtocolName, liHigh, "\tChild 0x%04x", child);
									group->AddItem(child);
								};
							};
	
							delete tlvReader;
							fGroups[groupID] = group;
						};
					} break;
					case BUDDY_RECORD: {
						//	There's some custom info here.
						buddymap::iterator bIt = fBuddy.find(name);
						Buddy *buddy = NULL;
						if (bIt == fBuddy.end()) {
							buddy = new Buddy(name, itemID);
							fBuddy[name] = buddy;
						} else {
							buddy = bIt->second;
						};
						
						if (buddy->IsInGroup(groupID) == false) buddy->AddGroup(groupID);
						contacts.push_back(name);

						reader->OffsetBy(len);
						
						LOG(kProtocolName, liHigh, "Got contact %s (0x%04x)", name, itemID);
					} break;
					case BUDDY_ICON_INFO: {
						reader->OffsetBy(len);
					} break;
					default: {
						reader->OffsetBy(len);
					} break;
				};

				if (name) free(name);
			};
						
			uint32 checkOut = reader->ReadInt32();
			LOG(kProtocolName, liLow, "Last checkout of SSI list 0x%08x", checkOut);

			fHandler->SSIBuddies(contacts);
		} break;
		case SSI_MODIFY_ACK: {
			int16 count = 0;
			
			while (reader->Offset() < reader->Length()) {
				uint16 code = reader->ReadInt16();
				LOG(kProtocolName, liHigh, "Upload for item %i is %s (0x%04x)",
					count, kSSIResult[code], code);
				count++;
			};
		} break;
		
		case 0x0011:
		case 0x0009:
		case 0x0012: {
			reader->Debug();
		};
		
		default: {
			LOG(kProtocolName, liLow, "Got an unhandled SSI SNAC (0x0013 / 0x%04x)",
				subtype);
			ret = kUnhandled;
		} break;
	};
	
	return ret;
};

status_t OSCARManager::HandleLocation(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandleAdvertisement(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandleInvitation(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandleAdministrative(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandlePopupNotice(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandlePrivacy(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandleUserLookup(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandleUsageStats(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandleTranslation(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandleChatNavigation(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandleChat(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandleUserSearch(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandleBuddyIcon(SNAC *snac, BufferReader *reader) {
	status_t ret = B_OK;
	
	uint16 subtype = snac->SubType();
	uint32 request = snac->RequestID();
	
	reader->OffsetTo(snac->DataOffset());

	switch (subtype) {
		case AIM_ICON: {
			uint8 nickLen = reader->ReadInt8();
			char *nick = reader->ReadString(nickLen);
			uint16 type = reader->ReadInt16();
			uint8 flags = reader->ReadInt8();
			uint8 hashLen = reader->ReadInt8();
			uchar *hash = reader->ReadData(hashLen);
			uint16 iconLen = reader->ReadInt16();
			uchar *icon = reader->ReadData(iconLen);
			
			if ((type == 0x0001) && (iconLen > 0)) {
				fHandler->BuddyIconFromUser(nick, icon, iconLen);
			};
			
			free(nick);
			free(hash);
		} break;
		
		case ICON_UPLOAD_ACK: {
		} break;
		
		default: {
			ret = kUnhandled;
		} break;
	};
	
	return ret;
};

status_t OSCARManager::HandleICQ(BMessage *msg) {
	return kUnhandled;
};

status_t OSCARManager::HandleAuthorisation(BMessage *msg) {
	return kUnhandled;
};

//#pragma mark -

status_t OSCARManager::Send(Flap *f) {
	if (f == NULL) return B_ERROR;
	
	if (f->Channel() == SNAC_DATA) {
		SNAC *s = f->SNACAt(0);
		
		if (s != NULL) {
			uint16 family = s->Family();

			connlist::iterator i;
			
			for (i = fConnections.begin(); i != fConnections.end(); i++) {
				OSCARConnection *con = (*i);
				if (con == NULL) continue;
				if (con->Supports(family) == true) {
					LOG(kProtocolName, liLow, "Sending SNAC (0x%04x) via %s:%i", family,
						con->Server(), con->Port());
					con->Send(f);
					return B_OK;
				};
			}
					
			LOG(kProtocolName, liMedium, "No connections handle SNAC (0x%04x) requesting service",
				family);
			OSCARConnection *con = fConnections.front();
			if (con == NULL) {
				LOG(kProtocolName, liHigh, "No available connections to send SNAC");
				return B_ERROR;
			} else {
				pfc_map::iterator pIt = fPendingConnections.find(family);
				if (pIt == fPendingConnections.end()) {
					Flap *newService = new Flap(SNAC_DATA);
					newService->AddSNAC(new SNAC(SERVICE_CONTROL, REQUEST_NEW_SERVICE));
					newService->AddInt16(family);

					con->Send(newService, atImmediate);
					
					fPendingConnections[family] = NULL;
					fWaitingSupport.push_back(f);
				} else {
					if (pIt->second != NULL) {
						pIt->second->Send(f, atOnline);
					} else {
						fWaitingSupport.push_back(f);
					};
				};
			};
		};
	} else {
		OSCARConnection *con = fConnections.front();
		if (con != NULL) {
			con->Send(f);
		} else {
			return B_ERROR;
		};
	};
	
	return B_OK;
};

status_t OSCARManager::Login(const char *server, uint16 port, const char *username,
	const char *password) {
	
	if ((username == NULL) || (password == NULL)) {
		LOG(kProtocolName, liHigh, "OSCARManager::Login: username or password not set");
		return B_ERROR;
	}
	
	ClearConnections();
	ClearWaitingSupport();
	
	if (fConnectionState == OSCAR_OFFLINE) {
		uint8 nickLen = strlen(username);
	
		fOurNick = (char *)realloc(fOurNick, (nickLen + 1) * sizeof(char));
		strncpy(fOurNick, username, nickLen);
		fOurNick[nickLen] = '\0';

		fConnectionState = OSCAR_CONNECTING;

		Flap *flap = new Flap(OPEN_CONNECTION);

		flap->AddInt32(0x00000001);
		flap->AddTLV(0x0001, username, nickLen);

		char *encPass = fHandler->RoastPassword(password);
		flap->AddTLV(0x0002, encPass, strlen(encPass));
		free(encPass);

		flap->AddTLV(0x0003, "BeOS IM Kit: OSCAR Addon", strlen("BeOS IM Kit: OSCAR Addon"));
		flap->AddTLV(0x0016, (char []){0x00, 0x01}, 2);
		flap->AddTLV(0x0017, (char []){0x00, 0x04}, 2);
		flap->AddTLV(0x0018, (char []){0x00, 0x02}, 2);
		flap->AddTLV(0x001a, (char []){0x00, 0x00}, 2);
		flap->AddTLV(0x000e, "us", 2);
		flap->AddTLV(0x000f, "en", 2);
		flap->AddTLV(0x0009, (char []){0x00, 0x15}, 2);
		
		OSCARConnection *c = new OSCARBOSConnection(server, port, this);
		c->Run();
		fConnections.push_back(c);
		c->Send(flap);

		fHandler->Progress("OSCAR Login", "OSCAR: Connecting...", 0.10);

		return B_OK;
	} else {
		LOG(kProtocolName, liDebug, "OSCARManager::Login: Already online");
		return B_ERROR;
	};
};

status_t OSCARManager::Progress(const char *id, const char *msg, float progress) {
	return fHandler->Progress(id, msg, progress);
};

status_t OSCARManager::Error(const char *msg) {
	return fHandler->Error(msg);
};

void OSCARManager::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case AMAN_NEW_CAPABILITIES: {

			LOG(kProtocolName, liLow, "Got a possible new capability %i connections"
				", %i pending", fConnections.size(), fPendingConnections.size());

			int16 family = 0;
			pfc_map::iterator pIt;
			for (int32 i = 0; msg->FindInt16("family", i, &family) == B_OK; i++) {
				pIt = fPendingConnections.find(family);
				if (pIt != fPendingConnections.end()) {
					fPendingConnections.erase(pIt);
					OSCARConnection *c = pIt->second;
					if (c != NULL) {
						LOG(kProtocolName, liLow, "%s:%i (%s) handles a new "
							"capability 0x%04x", c->Server(), c->Port(),
							c->ConnName(), family);
						fConnections.push_back(c);
					} else {
						LOG(kProtocolName, liDebug, "Connection to support 0x%04x "
							"has gone null on our ass", family);
					};
				} else {
					LOG(kProtocolName, liMedium, "An unexpected connection came in "
						"for family 0x%04x", family);
				};
			};
					
//			We can cheat here. Just try resending all the items, Send() will
//			take care of finding a connection for it
			flap_stack::iterator i;
			for (i = fWaitingSupport.begin(); i != fWaitingSupport.end(); i++) {
				Flap *f = (*i);
				if (f) Send(f);
			};
			
			fWaitingSupport.clear();
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
			int16 family = -1;
			OSCARConnection *con = NULL;
			
			if (msg->FindData("cookie", B_RAW_TYPE, (const void **)&cookie, &bytes) != B_OK) return;
			if (msg->FindString("host", (const char **)&host) != B_OK) return;
			if (msg->FindInt16("port", &port) != B_OK) return;
			if (msg->FindInt16("family", &family) == B_OK) {
				LOG(kProtocolName, liMedium, "Connecting to %s:%i for 0x%04x\n",
					host, port, family);
				con = new OSCARReqConn(host, port, this);
				fPendingConnections[family] = con;
			} else {
				con = new OSCARConnection(host, port, this);
				fConnections.push_back(con);
			};
			
			Flap *srvCookie = new Flap(OPEN_CONNECTION);
			srvCookie->AddRawData((uchar []){0x00, 0x00, 0x00, 0x01}, 4);
			srvCookie->AddTLV(new TLV(0x0006, cookie, bytes));

			con->Run();
			con->Send(srvCookie);
		} break;
		
		case AMAN_CLOSED_CONNECTION: {
			OSCARConnection *con = NULL;
			msg->FindPointer("connection", (void **)&con);
			if (con != NULL) {
				LOG(kProtocolName, liLow, "Connection (%s:%i) closed", con->Server(),
					con->Port());
					
				fConnections.remove(con);
				con->Lock();
				con->Quit();
				LOG(kProtocolName, liLow, "After close we have %i connections",
					fConnections.size());
				
				bool hasBOS = false;
				connlist::iterator cIt = fConnections.begin();
				for (; cIt != fConnections.end(); cIt++) {
					OSCARConnection *con = (*cIt);
					if ((con) && (con->ConnectionType() == connBOS)) {
						hasBOS = true;
						break;
					};
				};
				
				if (hasBOS == false) {
					ClearWaitingSupport();
					ClearConnections();
					fHandler->StatusChanged(fOurNick, OSCAR_OFFLINE);
					fConnectionState = OSCAR_OFFLINE;
				};
			};
		} break;

		case AMAN_FLAP_OPEN_CON: {
//			We don't do anything with this currently
//			const uchar *data;
//			int32 bytes = 0;
//			msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);
		} break;
		
		case AMAN_FLAP_SNAC_DATA: {
			status_t result = kUnhandled;
			const uchar *data;
			int32 bytes = 0;
			msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);
			
			BufferReader reader(data, bytes);
			SNAC snac(&reader);		
			reader.OffsetTo(0);

			uint16 family = snac.Family();
			uint16 subtype = snac.SubType();

			LOG(kProtocolName, liLow, "OSCARManager: Got SNAC (0x%04x, 0x%04x)", family, subtype);

			if (subtype == ERROR) {
				reader.OffsetTo(snac.DataOffset());
				uint16 code = reader.ReadInt16();
				fHandler->Error(kErrors[code]);
				return;
			};
			
			switch (family) {
				case SERVICE_CONTROL: {
					result = HandleServiceControl(&snac, &reader);
				} break;
				case LOCATION: {
					result = HandleLocation(msg);
				} break;
				case BUDDY_LIST_MANAGEMENT: {
					result = HandleBuddyList(&snac, &reader);
				} break;
				case ICBM: {
					result = HandleICBM(&snac, &reader);
				} break;
				case ADVERTISEMENTS: {
					result = HandleAdvertisement(msg);
				} break;
				case INVITATION: {
					result = HandleInvitation(msg);
				} break;
				case ADMINISTRATIVE: {
					result = HandleAdministrative(msg);
				} break;
				case POPUP_NOTICES: {
					result = HandlePopupNotice(msg);
				} break;
				case PRIVACY_MANAGEMENT: {
					result = HandlePrivacy(msg);
				} break;
				case USER_LOOKUP: {
					result = HandleUserLookup(msg);
				} break;
				case USAGE_STATS: {
					result = HandleUsageStats(msg);
				} break;
				case TRANSLATION: {
					result = HandleTranslation(msg);
				} break;
				case CHAT_NAVIGATION: {
					result = HandleChatNavigation(msg);
				} break;
				case CHAT: {
					result = HandleChat(msg);
				} break;
				case DIRECTORY_USER_SEARCH: {
					result = HandleUserSearch(msg);
				} break;
				case SERVER_STORED_BUDDY_ICONS: {
					result = HandleBuddyIcon(&snac, &reader);
				} break;
				case SERVER_SIDE_INFORMATION: {		
					result = HandleSSI(&snac, &reader);
				} break;
				case ICQ_SPECIFIC_EXTENSIONS: {
					result = HandleICQ(msg);
				} break;
				case AUTHORISATION_REGISTRATION: {
					result = HandleAuthorisation(msg);
				} break;
			};
			
			if (result == kUnhandled) {
				LOG(kProtocolName, liHigh, "Got totally unhandled SNAC (0x%04x"
					", 0x%04x)", family, subtype);
			};
		} break;
				
		case AMAN_FLAP_ERROR: {
//			We ignore this for now
		} break;
	
		case AMAN_FLAP_CLOSE_CON: {
			const uchar *data;
			int32 bytes = 0;
			msg->FindData("data", B_RAW_TYPE, (const void **)&data, &bytes);

			if (fConnectionState == OSCAR_CONNECTING) {
	
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
							
							LOG(kProtocolName, liHigh, "Need to reconnect to: %s:%i", server, port);
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
				f->AddInt32(0x00000001);
				f->AddTLV(0x0006, cookie, cookieSize);
				
				Send(f);
			} else {
				fHandler->StatusChanged(fOurNick, OSCAR_OFFLINE);
			};
		} break;
		
		default: {
			BLooper::MessageReceived(msg);
		};
	};
};

//#pragma mark Interface

status_t OSCARManager::MessageUser(const char *screenname, const char *message) {
	LOG(kProtocolName, liLow, "OSCARManager::MessageUser: Sending \"%s\" (%i) to %s (%i)",
		message, strlen(message), screenname, strlen(screenname));
		
	Flap *msg = new Flap(SNAC_DATA);
	msg->AddSNAC(new SNAC(ICBM, SEND_MESSAGE_VIA_SERVER));
	msg->AddInt64(0x0000000000000000); // MSG-ID Cookie
	msg->AddInt16(PLAIN_TEXT);	// Channel

	uint8 screenLen = strlen(screenname);
	msg->AddInt8(screenLen);
	msg->AddRawData((uchar *)screenname, screenLen);
	
	TLV *msgData = new TLV(0x0002);
	msgData->AddTLV(new TLV(0x0501, (char []){0x01}, 1));	// Text capability
	
	BString html = message;
	fHandler->FormatMessageText(html);
	char *msgFragment = (char *)calloc(html.Length() + 5, sizeof(char));
	msgFragment[0] = 0x00; // UTF-8. Maybe... who knows?
	msgFragment[1] = 0x00;
	msgFragment[2] = 0xff;
	msgFragment[3] = 0xff;
	strncpy(msgFragment + 4, html.String(), html.Length());
	
	msgData->AddTLV(new TLV(0x0101, msgFragment, html.Length() + 4));
	
	free(msgFragment);
	msg->AddTLV(msgData);
	Send(msg);
	
	return B_OK;
};

status_t OSCARManager::AddSSIBuddy(const char *name, grouplist_t groups) {
	LOG(kProtocolName, liLow, "OSCARManager::AddSSIBuddy(%s) called", name);
	int32 reqGroupCount = groups.size();
	Group *master = NULL;
	group_t::iterator gIt;
	group_t newGroups;
	group_t existingGroups;
	
	// Get the master group
	gIt = fGroups.find(0x0000);
	if (gIt == fGroups.end()) {
		LOG(kProtocolName, liHigh, "Could not obtain a reference to the master "
			"group, bailing on add SSI buddy");
		return B_ERROR;
	};
	master = gIt->second;
	
	Flap *startTran = new Flap(SNAC_DATA);
	startTran->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, SSI_EDIT_BEGIN, 0x00,
		0x00, 0x00000000));
	startTran->AddInt32(0x00010000); // Import, avoids authorisation requests
	Send(startTran);
	
	bool createBuddy = false;
	Buddy *buddy = GetBuddy(name);
	if (buddy == NULL) {
		createBuddy = true;
		buddy = new Buddy(name, GetNewItemId());

		fBuddy[name] = buddy;
	};
	
	for (int32 i = 0; i < reqGroupCount; i++) {
		Group *group = NULL;
		
		for (gIt = fGroups.begin(); gIt != fGroups.end(); gIt++) {
			Group *curGroup = gIt->second;
			if (groups[i] == curGroup->Name()) {
				group = curGroup;			
				break;
			};
		};

		if (group == NULL) {
			group = new Group(GetNewItemId(), groups[i].String());
			group->AddItem(buddy->ItemID());
			
			newGroups[group->Id()] = group;
			fGroups[group->Id()] = group;
			master->AddItem(group->Id());
			
		} else {
			if (buddy->IsInGroup(group->Id()) == false) {
				group->AddItem(buddy->ItemID());
				existingGroups[group->Id()] = group;
			};
		};
	};
	
	// This Buddy isn't in a group already, and doesn't have any groups, throw
	// them in the first group
	if ((reqGroupCount == 0) && (buddy->CountGroups() == 0)) {
		gIt = fGroups.begin();
		existingGroups[gIt->first] = gIt->second;
		buddy->AddGroup(gIt->first);
	};
		
	if (newGroups.empty() == false) {
		Flap *createGroups = new Flap(SNAC_DATA);
		createGroups->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, ADD_SSI_ITEM));
		
		for (gIt = newGroups.begin(); gIt != newGroups.end(); gIt++) {
			Group *group = gIt->second;

			// Create the group entry on the server side
			createGroups->AddInt16(strlen(group->Name()));
			createGroups->AddRawData((uchar *)group->Name(), strlen(group->Name()));
			createGroups->AddInt16(group->Id());	// Group ID
			createGroups->AddInt16(0x0000);			// Item ID
			createGroups->AddInt16(GROUP_RECORD);	// Type

			// Add all the buddy items in this group
			BufferWriter contentWriter;
			for (int32 i = 0; i < group->ItemsInGroup(); i++) {
				int16 value = group->ItemAt(i);
				LOG(kProtocolName, liHigh, "%'s contains: 0x%04x\n",
					group->Name(), value);
			
				contentWriter.WriteInt16(value);
			};
				
			TLV *items = new TLV(0x00c8, (char *)contentWriter.Buffer(), contentWriter.Length());
			createGroups->AddInt16(items->FlattenedSize());
			createGroups->AddTLV(items);
			
			// Add the buddy record for this group
			createGroups->AddInt16(strlen(buddy->Name()));
			createGroups->AddRawData((uchar *)buddy->Name(), strlen(buddy->Name()));
			createGroups->AddInt16(group->Id());		// Group ID
			createGroups->AddInt16(buddy->ItemID());	// Buddy ID
			createGroups->AddInt16(BUDDY_RECORD);
			createGroups->AddInt16(0x0000);				// No additional info
		};
	
		// For all the groups that already exist, add the buddy to them
		for (gIt = existingGroups.begin(); gIt != existingGroups.end(); gIt++) {
			Group *group = gIt->second;
			
			createGroups->AddInt16(strlen(buddy->Name()));
			createGroups->AddRawData((uchar *)buddy->Name(), strlen(buddy->Name()));
			createGroups->AddInt16(group->Id());		// Group ID
			createGroups->AddInt16(buddy->ItemID());	// Buddy ID
			createGroups->AddInt16(BUDDY_RECORD);
			createGroups->AddInt16(0x0000);				// No additional info
		};
	
		Send(createGroups);
	};

	if (existingGroups.empty() == false) {
		Flap *modifyGroups = new Flap(SNAC_DATA);
		modifyGroups->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, SSI_UPDATE_ITEM));
		
		for (gIt = existingGroups.begin(); gIt != existingGroups.end(); gIt++) {
			Group *group = gIt->second;
			
			printf("Adding existing group: %s\n", group->Name());

			modifyGroups->AddInt16(strlen(group->Name()));
			modifyGroups->AddRawData((uchar *)group->Name(), strlen(group->Name()));
			modifyGroups->AddInt16(group->Id());	// Group ID
			modifyGroups->AddInt16(0x0000);			// Item ID
			modifyGroups->AddInt16(GROUP_RECORD);	// Type

			BufferWriter contentWriter;
			for (int32 i = 0; i < group->ItemsInGroup(); i++) {
				int16 value = group->ItemAt(i);
				LOG(kProtocolName, liHigh, "%'s contains: 0x%04x\n",
					group->Name(), value);
			
				contentWriter.WriteInt16(value);
			};
			
			// Group contents
			TLV *items = new TLV(0x00c8, (char *)contentWriter.Buffer(), contentWriter.Length());
			modifyGroups->AddInt16(items->FlattenedSize());
			modifyGroups->AddTLV(items);
		};
		
		// Update the Master group's contents
		modifyGroups->AddInt16(0x0000);	// Name length
		modifyGroups->AddInt16(0x0000);	// Group Id
		modifyGroups->AddInt16(0x0000);	// Item Id
		modifyGroups->AddInt16(GROUP_RECORD);
			
		BufferWriter masterWriter;
		for (int32 i = 0; i < master->ItemsInGroup(); i++) {
			int16 value = master->ItemAt(i);
			LOG(kProtocolName, liHigh, "Master's %i group: 0x%04x\n",
				i, value);
		
			masterWriter.WriteInt16(value);
		};
		
		TLV *items = new TLV(0x00c8, (char *)masterWriter.Buffer(), masterWriter.Length());
		modifyGroups->AddInt16(items->FlattenedSize());
		modifyGroups->AddTLV(items);		
		
		Send(modifyGroups);
	};
	
	Flap *endTran = new Flap(SNAC_DATA);
	endTran->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, SSI_EDIT_END, 0x00,
		0x00, 0x00000000));
	Send(endTran);

	return B_OK;	
};

#if 0
status_t OSCARManager::AddSSIBuddy(const char *name, const char *groupname) {
	LOG(kProtocolName, liLow, "OSCARManager::AddSSIBuddy(%s, %s) called\n",
		name, groupname);

	if (name == NULL) return B_ERROR;
	
	buddymap::iterator bIt = fBuddy.find(name);
	if (bIt != fBuddy.end()) {
		LOG(kProtocolName, liLow, "OSCARManager::AddSSIBuddy(%s, %s) - Already exists",
			name, groupname);
		return B_ERROR;
	};
	
	LOG(kProtocolName, liLow, "OSCARManager::AddSSIBuddy(%s, %s)", name, groupname);
	Group *group = NULL;
	int16 itemId = GetNewItemId();
	int16 groupSubType = SSI_UPDATE_ITEM;
	group_t::iterator gIt;
	
	Flap *startTran = new Flap(SNAC_DATA);
	startTran->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, SSI_EDIT_BEGIN, 0x00,
		0x00, 0x00000000));
	startTran->AddInt32(0x00010000); // Import, avoids authorisation requests
	Send(startTran);

	if (groupname != NULL) {
		for (gIt = fGroups.begin(); gIt != fGroups.end(); gIt++) {
			if (strcmp(gIt->second->Name(), groupname) == 0) {
				group = gIt->second;
				break;
			};
		};
	} else {
		gIt = fGroups.begin();
		group = gIt->second;
	};

	if (group == NULL) {
		LOG(kProtocolName, liLow, "OSCARManager::AddSSIBuddy(%s, %s) - Creating group",
			name, groupname);
			
		Flap *modifyMasterGroup = new Flap(SNAC_DATA);
		modifyMasterGroup->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, SSI_UPDATE_ITEM,
			0x00, 0x00, 0x00000000));
		modifyMasterGroup->AddInt16(0x0000);	// Name length
		modifyMasterGroup->AddInt16(0x0000);	// Group Id
		modifyMasterGroup->AddInt16(0x0000);	// Item Id
		modifyMasterGroup->AddInt16(GROUP_RECORD);
		
		group_t::iterator mIt = fGroups.find(0x0000);
		if (mIt == fGroups.end()) {
			LOG(kProtocolName, liHigh, "Could not find master group! UT OH!");
			delete modifyMasterGroup;
			return B_ERROR;
		};

		Group *master = mIt->second;

		// Ideally this shouldn't be added until it's all successful
		group = new Group(GetNewItemId(), groupname);
		fGroups[group->Id()] = group;
		fItemIds[group->Id()] = true;
		groupSubType = ADD_SSI_ITEM;
		master->AddItem(group->Id());
		
		LOG(kProtocolName, liHigh, "%s will be group 0x%04x\n", groupname,
			group->Id());
		
		BufferWriter masterWriter;
		for (int32 i = 0; i < master->ItemsInGroup(); i++) {
			int16 value = master->ItemAt(i);
			LOG(kProtocolName, liHigh, "Master's %i group: 0x%04x\n",
				i, value);
		
			masterWriter.WriteInt16(value);
		};
		
		LOG(kProtocolName, liDebug, "Master Contents");
		PrintHex((uchar *)masterWriter.Buffer(), masterWriter.Length(), true);
		
		TLV *items = new TLV(0x00c8, (char *)masterWriter.Buffer(), masterWriter.Length());
		modifyMasterGroup->AddInt16(items->FlattenedSize());
		modifyMasterGroup->AddTLV(items);
		
		Send(modifyMasterGroup);
	};
	
	// Add the new item id to the child
	group->AddItem(itemId);

	Flap *modifyGroup = new Flap(SNAC_DATA);
	modifyGroup->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, groupSubType, 0x00,
		0x00, 0x00000000));
	modifyGroup->AddInt16(strlen(group->Name()));
	modifyGroup->AddRawData((uchar *)group->Name(), strlen(group->Name()));
	modifyGroup->AddInt16(group->Id());		// Group Id
	modifyGroup->AddInt16(0x0000);			// Item Id
	modifyGroup->AddInt16(GROUP_RECORD);
	
	BufferWriter writer;

	for (int32 i = 0; i < group->ItemsInGroup(); i++) {
		int16 value = group->ItemAt(i);
		writer.WriteInt16(value);
	};
	
	LOG(kProtocolName, liDebug, "Group Contents");
	PrintHex(writer.Buffer(), writer.Length(), true);
	
	TLV *items = new TLV(0x00c8, (char *)writer.Buffer(), writer.Length());
	modifyGroup->AddInt16(items->FlattenedSize());
	modifyGroup->AddTLV(items);
	
	Send(modifyGroup);
	
	Buddy *buddy = new Buddy(name, group->Id(), itemId);
	fBuddy[name] = buddy;
	fItemIds[itemId] = true;
	
	LOG(kProtocolName, liHigh, "Adding \"%s\" to %s (0x%04x) with id 0x%04x",
		name, groupname, group->Id(), buddy->ItemID());
		
	Flap *addBuddy = new Flap(SNAC_DATA);
	addBuddy->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, ADD_SSI_ITEM));
	addBuddy->AddInt16(strlen(name));
	addBuddy->AddRawData((uchar *)name, strlen(name));
	addBuddy->AddInt16(group->Id());		// Group Id
	addBuddy->AddInt16(buddy->ItemID());	// Item Id
	addBuddy->AddInt16(BUDDY_RECORD);
	addBuddy->AddInt16(0x0000);				// No additional inforty
	
	Send(addBuddy);
	
	Flap *endTran = new Flap(SNAC_DATA);
	endTran->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, SSI_EDIT_END, 0x00,
		0x00, 0x00000000));
	Send(endTran);

	return B_OK;
};
#endif

status_t OSCARManager::AddBuddy(const char *buddy) {
	status_t ret = B_ERROR;
	if (buddy != NULL) {
		LOG(kProtocolName, liLow, "OSCARManager::AddBuddy: Adding \"%s\" to list", buddy);
//		fBuddy[buddy] = NULL;
		
		Flap *addBuddy = new Flap(SNAC_DATA);
		addBuddy->AddSNAC(new SNAC(BUDDY_LIST_MANAGEMENT, ADD_BUDDY_TO_LIST, 0x00,
			0x00, 0x00000000));
		
		uint8 buddyLen = strlen(buddy);
		addBuddy->AddRawData((uchar [])&buddyLen, sizeof(buddyLen));
		addBuddy->AddRawData((uchar *)buddy, buddyLen);
		
		ret = B_OK;
	};

	return ret;
};

status_t OSCARManager::AddBuddies(list <char *>buddies) {
	Flap *addBuds = new Flap(SNAC_DATA);
	addBuds->AddSNAC(new SNAC(BUDDY_LIST_MANAGEMENT, ADD_BUDDY_TO_LIST, 0x00,
		0x00, 0x00000000));
	
	list <char *>::iterator i;
	uint8 buddyLen = 0;
	
	for (i = buddies.begin(); i != buddies.end(); i++) {
		char *buddy = (*i);
		buddyLen = strlen(buddy);

//		fBuddy[buddy] = NULL;		
		
		addBuds->AddRawData((uchar *)&buddyLen, sizeof(buddyLen));
		addBuds->AddRawData((uchar *)buddy, buddyLen);
		
		free(buddy);
	};
	
	Send(addBuds);
	
	return B_OK;	
};

status_t OSCARManager::RemoveBuddy(const char *buddy) {
	status_t ret = B_ERROR;

	if (buddy) {
		buddymap::iterator bIt = fBuddy.find(buddy);
		uint8 buddyLen = strlen(buddy);

		if ((bIt == fBuddy.end()) || (bIt->second == NULL)) {		
//			Not an SSI buddy, client side only
			Flap *remove = new Flap(SNAC_DATA);
			remove->AddSNAC(new SNAC(BUDDY_LIST_MANAGEMENT, REMOVE_BUDDY_FROM_LIST,
				0x00, 0x00, 0x00000000));
	
			remove->AddRawData((uchar *)&buddyLen, sizeof(buddyLen));
			remove->AddRawData((uchar *)buddy, buddyLen);
	
			Send(remove);
	
			ret = B_OK;
		};
#if 0
		} else {
//			Start modification session
			Flap *begin = new Flap(SNAC_DATA);
			begin->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, SSI_EDIT_BEGIN));
			Send(begin);

//			Buddy is in our SSI list
			Flap *remove = new Flap(SNAC_DATA);
			remove->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, DELETE_SSI_ITEM));
			
//			Buddy name
			remove->AddInt8(buddyLen);
			remove->AddRawData((uchar *)buddy, buddyLen);

			Buddy *ssi = bIt->second;
			uint16 groupID = ssi->GroupID();
			uint16 itemID = ssi->ItemID();
			uint16 type = BUDDY_RECORD;

			remove->AddInt16(groupID);
			remove->AddInt16(itemID);
			remove->AddInt16(BUDDY_RECORD);
			remove->AddInt16(0x0000); // No additional data

			Send(remove);
			
			fBuddy.erase(buddy);
			delete ssi;
			
			Flap *end = new Flap(SNAC_DATA);
			end->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, SSI_EDIT_END));
			Send(end);
			
			ret = B_OK;
		};
#endif
	};
	
	return ret;
};

status_t OSCARManager::RemoveBuddies(list <char *>buddy) {
	(void)buddy;
	return B_OK;
};

Buddy *OSCARManager::GetBuddy(const char *screenname) {
	Buddy *buddy = NULL;
	buddymap::iterator bIt = fBuddy.find(screenname);
	if (bIt != fBuddy.end()) buddy = bIt->second;
	
	return buddy;
};

int32 OSCARManager::Buddies(void) const {
	return fBuddy.size();
};

uchar OSCARManager::IsConnected(void) const {
	return fConnectionState;
};

status_t OSCARManager::LogOff(void) {
	status_t ret = B_ERROR;
	if (fConnectionState != OSCAR_OFFLINE) {
		fConnectionState = OSCAR_OFFLINE;
		
		LOG(kProtocolName, liLow, "%i connection(s) to kill", fConnections.size());
		
		ClearConnections();
		ClearWaitingSupport();

		fHandler->StatusChanged(fOurNick, OSCAR_OFFLINE);
		ret = B_OK;
	};

	return ret;
};

status_t OSCARManager::TypingNotification(const char *buddy, uint16 typing) {
	LOG(kProtocolName, liLow, "Sending typing notification (0x%04x) to \"%s\"",
		typing, buddy);
	
	Flap *notify = new Flap(SNAC_DATA);
	notify->AddSNAC(new SNAC(ICBM, TYPING_NOTIFICATION));
	notify->AddInt64(0x0000000000000000);	// Notification cookie
	notify->AddInt16(PLAIN_TEXT);			// Notification channel

	uint8 buddyLen = strlen(buddy);
	notify->AddInt8(buddyLen);
	notify->AddRawData((uchar *)buddy, buddyLen);
	notify->AddInt16(typing);
	
	Send(notify);
	
	return B_OK;
};

status_t OSCARManager::SetAway(const char *message) {
	Flap *away = new Flap(SNAC_DATA);
	away->AddSNAC(new SNAC(LOCATION, SET_USER_INFORMATION, 0x00, 0x00, 0x00000000));

	if (message) {
		away->AddTLV(new TLV(0x0003, kEncoding, strlen(kEncoding)));
		away->AddTLV(new TLV(0x0004, message, strlen(message)));
		fAwayMsg = message;

		fHandler->StatusChanged(fOurNick, OSCAR_AWAY);
		fConnectionState = OSCAR_AWAY;
	} else {
		away->AddTLV(new TLV(0x0003, kEncoding, strlen(kEncoding)));
		away->AddTLV(new TLV(0x0004, "", 0));
	
		fAwayMsg = "";
		
		fHandler->StatusChanged(fOurNick, OSCAR_ONLINE);
		fConnectionState = OSCAR_ONLINE;
	};
		
	Send(away);
	
	return B_OK;
};

status_t OSCARManager::SetProfile(const char *profile) {
	if (profile == NULL) {
		fProfile = "";
	} else {
		fProfile.SetTo(profile);
	};

	if ((fConnectionState == OSCAR_ONLINE) || (fConnectionState == OSCAR_AWAY)) {
		Flap *p = new Flap(SNAC_DATA);
		p->AddSNAC(new SNAC(LOCATION, SET_USER_INFORMATION, 0x00, 0x00,
			0x00000000));
		
		if (fProfile.Length() > 0) {
			p->AddTLV(new TLV(0x0001, kEncoding, strlen(kEncoding)));
			p->AddTLV(new TLV(0x0002, fProfile.String(), fProfile.Length()));
		};
		
		if ((fConnectionState == OSCAR_AWAY) && (fAwayMsg.Length() > 0)) {
			p->AddTLV(new TLV(0x0003, kEncoding, strlen(kEncoding)));
			p->AddTLV(new TLV(0x0004, fAwayMsg.String(), fAwayMsg.Length()));
		};
				
		Send(p); 
	};
				
	return B_OK;
};

status_t OSCARManager::SetIcon(const char *icon, int16 size) {
	if ((icon == NULL) || (size < 0)) return B_ERROR;

	if (fIcon) free(fIcon);
	fIcon = (char *)calloc(size, sizeof(char));
	fIconSize = size;
	memcpy(fIcon, icon, size);

	if (fConnectionState == OSCAR_OFFLINE) return B_ERROR;
	
	Flap *add = new Flap(SNAC_DATA);
	add->AddSNAC(new SNAC(SERVER_SIDE_INFORMATION, ADD_SSI_ITEM));
	add->AddInt16(0x0001);		// Size of name
	add->AddInt8('3');			// Name
	add->AddInt16(0x0000);		// Group ID
	add->AddInt16(0x1813);		// Item ID
	add->AddInt16(BUDDY_ICON_INFO);
	add->AddInt16(0x0016);		// Length of additional data
	
	char buffer[18];
	buffer[0] = 0x01; // Icon flags
	buffer[2] = 0x10; // MD5 Length
	MD5(icon, size, buffer + 2);
	add->AddTLV(new TLV(0x00d5, buffer, 18));
	add->AddTLV(new TLV(0x0131, "", 0));
	Send(add);
	
	return B_OK;
};

char *OSCARManager::ParseMessage(TLV *parent) {
	int16 offset = 0;
	char *buffer = (char *)parent->Value();
	int16 bytes = parent->Length();
	char *message = NULL;
	int16 length = 0;
	 
	while (offset < bytes) {
		TLV tlv((uchar *)buffer + offset, bytes - offset);

		switch (tlv.Type()) {
			case 0x0101: {	// Message length, encoding and contents
				int16 subOffset = 0;
				char *value = (char *)tlv.Value();
				length = tlv.Length() - (sizeof(int16) * 2);
				// The charset and subset are part of the TLV				
				int16 charSet = (value[subOffset] << 8) + value[++subOffset];
				int16 charSubSet = (value[++subOffset] << 8) + value[++subOffset];
				subOffset += 1;

				message = (char *)calloc(length + 1, sizeof(char));
				memcpy(message, (char *)(value + subOffset), length);
				message[length] = '\0';

				// UTF-16 message
				if (charSet == 0x0002) {
					LOG(kProtocolName, liLow, "Got a UTF-16 encoded message");
					PrintHex((uchar *)message, length);
					char *msg16 = (char *)calloc(length, sizeof(char));
					int32 state = 0;
					int32 utf8Size = length * 2;
					int32 ut16Size = length;
					memcpy(msg16, message, length);
					message = (char *)realloc(message, utf8Size * sizeof(char));
					
					convert_to_utf8(B_UNICODE_CONVERSION, msg16, &ut16Size, message,
						&utf8Size, &state);
					message[utf8Size] = '\0';
					length = utf8Size;

					free(msg16);
					
					LOG(kProtocolName, liLow, "Converted message: \"%s\"",
						message);
				};
				
				return message;
			} break;
		
			default: {
				offset += tlv.FlattenedSize();
			};
		};
	};
	
	return message;
};

uint16 OSCARManager::GetNewItemId(void) {
	uint16 id = 0;
	id_t::iterator iIt;
	for (iIt = fItemIds.begin(); iIt != fItemIds.end(); iIt++) {
		id = max_c(iIt->first, id);
	};
	
	return id + 1;
};
