#include "AIM.h"
/*
	Written by Michael "slaad" Davidson based off docs from
		http://iserverd1.khstu.ru/oscar/
		
					and
		
		http://aimdoc.sf.net/
*/

#include <libim/Constants.h>
#include <libim/Helpers.h>

#include <UTF8.h>

extern "C"
IM::Protocol * load_protocol() {
	return new AIMProtocol();
}

AIMProtocol::AIMProtocol()
	: IM::Protocol( Protocol::MESSAGES | Protocol::SERVER_BUDDY_LIST ),
	  fThread(0) {
	
	fPassword = NULL;
	fScreenName = NULL;
	fEncoding = 0xffff; // No converstion == UTF-8
	fManager = new AIMManager(dynamic_cast<AIMHandler *>(this));
};

AIMProtocol::~AIMProtocol() {
	if (fScreenName) free(fScreenName);
	if (fPassword) free(fPassword);
}

status_t AIMProtocol::Init(BMessenger msgr) {
	fMsgr = msgr;
	LOG("AIM", liMedium, "AIMProtocol::Init() start");
	
	fManager->Run();
	
	return B_OK;
}

status_t AIMProtocol::Shutdown() {
	fManager->LogOff();
	if (fManager->Lock()) fManager->Quit();
	
	LOG("AIM", liMedium, "AIMProtocol::Shutdown() done");
		
	return B_OK;
}

status_t AIMProtocol::Process(BMessage * msg) {
	switch (msg->what) {
		case IM::MESSAGE: {
			int32 im_what=0;
			
			msg->FindInt32("im_what",&im_what);
		
			switch (im_what) {
				case IM::UNREGISTER_CONTACTS: {
					printf("Unregister\n");
					msg->PrintToStream();
					
					fManager->RemoveBuddy(msg->FindString("id"));
					
				} break;
				case IM::REGISTER_CONTACTS:
				{
					if ( !fManager->IsConnected() )
						break;
					
					printf("AIM: Got Register\n");
					msg->PrintToStream();
					
					type_code garbage;
					int32 count = 0;
					msg->GetInfo("id", &garbage, &count);
					
					if (count > 0) {
						list<char *> buddies;
						for ( int i=0; msg->FindString("id",i); i++ )
						{
							const char * id = msg->FindString("id",i);//NormalizeNick(msg->FindString("id",i)).String();
							buddies.push_back(strdup(id));
						};
						fManager->AddBuddies(buddies);
					} else {
						fManager->AddBuddy(msg->FindString("id")); //NormalizeNick(msg->FindString("id")).String());
					};
				}	break;
				
				case IM::SET_STATUS: {
					const char *status = msg->FindString("status");
					LOG("AIM", liMedium, "Set status to %s", status);
					
					if (strcmp(status, OFFLINE_TEXT) == 0) {
						fManager->LogOff();
					} else
					if (strcmp(status, AWAY_TEXT) == 0) {
						if (fManager->ConnectionState() == (uchar)AMAN_ONLINE) {
							const char *away_msg = msg->FindString("away_msg");
							if (away_msg != NULL) {
								LOG("AIM", liMedium, "Setting away message: %s", away_msg);
								fManager->SetAway(away_msg);
							}
						};
					} else
					if (strcmp(status, ONLINE_TEXT) == 0) {
						if (fManager->IsConnected() == AMAN_AWAY) {
							fManager->SetAway(NULL);
						} else {
							LOG("AIM", liDebug, "Calling fManager.Login()");
							fManager->Login("login.oscar.aol.com", (uint16)5190,
								fScreenName, fPassword);
						};
					} else
					{
						LOG("AIM", liHigh, "Invalid status when setting status: '%s'", status);
					}
				} break;

				case IM::GET_CONTACT_INFO:
				{
					LOG("AIM", liLow, "Getting contact info");
					const char * id = NormalizeNick(msg->FindString("id")).String();
					
					BMessage *infoMsg = new BMessage(IM::MESSAGE);
					infoMsg->AddInt32("im_what", IM::CONTACT_INFO);
					infoMsg->AddString("protocol", "AIM");
					infoMsg->AddString("id", id);
					infoMsg->AddString("nick", id);
					infoMsg->AddString("first name", id);
					//msg->AddString("last name", id);

					fMsgr.SendMessage(infoMsg);
				}	break;
		
				case IM::SEND_MESSAGE:
				{
					const char * message_text = msg->FindString("message");
					BString srcid = msg->FindString("id");
					BString normal = NormalizeNick(srcid.String());
					BString screen = GetScreenNick(normal.String());
					
					const char * id = screen.String();
					
					LOG("AIM", liDebug, "SEND_MESSAGE (%s, %s)", msg->FindString("id"), msg->FindString("message"));
					LOG("AIM", liDebug, "  %s > %s > %s", srcid.String(), normal.String(), screen.String() );
					
					if ( !id )
						return B_ERROR;
					
					if ( !message_text )
						return B_ERROR;
					
					fManager->MessageUser(id, message_text);
					
					BMessage newMsg(*msg);
					
					newMsg.RemoveName("contact");
					newMsg.ReplaceInt32("im_what", IM::MESSAGE_SENT);
					
					fMsgr.SendMessage(&newMsg);
					
//					fManager->RequestBuddyIcon(id);
				}	break;
				case IM::USER_STARTED_TYPING: {
//					const char *id = msg->FindString("id");
//					if (!id) return B_ERROR;
				
//					fManager->TypingNotification(id, STARTED_TYPING);
//					snooze(1000 * 1);
//					fManager->TypingNotification(id, STILL_TYPING);
				} break;
				case IM::USER_STOPPED_TYPING: {
//					const char *id = msg->FindString("id");
//					if (!id) return B_ERROR;
					
//					fManager->TypingNotification(id, FINISHED_TYPING);
				} break;
/*				case IM::SEND_AUTH_ACK:
				{
					bool authreply;
					
					if ( !fClient.icqclient.isConnected() )
						return B_ERROR;
					
					const char * id = msg->FindString("id");
					int32 button = msg->FindInt32("which");
					
					if (button == 0) {
						LOG("icq", DEBUG, "Authorization granted to %s", id);
						authreply = true;
					} else {
						LOG("icq", DEBUG, "Authorization rejected to %s", id);
						authreply = false;					
					}
						
					ICQ2000::ContactRef c = new ICQ2000::Contact( atoi(id) );
					
					AuthAckEvent * ev = new AuthAckEvent(c, authreply);

					fClient.icqclient.SendEvent( ev );									
				}
*/				default:
					break;
			}
			
		}	break;
		default:
			break;
	}
	
	return B_OK;
}

const char * AIMProtocol::GetSignature() {
	return "AIM";
}

//uint32 AIMProtocol::Capabilities() {
//	return PROT_AWAY_MESSAGES | PROT_TYPING_NOTIFICATIONS;
//};

BMessage AIMProtocol::GetSettingsTemplate() {
	BMessage main_msg(IM::SETTINGS_TEMPLATE);
	
	BMessage user_msg;
	user_msg.AddString("name","screenname");
	user_msg.AddString("description", "Screen Name");
	user_msg.AddInt32("type",B_STRING_TYPE);
	
	BMessage pass_msg;
	pass_msg.AddString("name","password");
	pass_msg.AddString("description", "Password");
	pass_msg.AddInt32("type",B_STRING_TYPE);
	pass_msg.AddBool("is_secret", true);
	
/*	BMessage enc_msg;
	enc_msg.AddString("name","encoding");
	enc_msg.AddString("description","Text encoding");
	enc_msg.AddInt32("type", B_STRING_TYPE);
	enc_msg.AddString("valid_value", "ISO 8859-1");
	enc_msg.AddString("valid_value", "UTF-8");
	enc_msg.AddString("valid_value", "JIS");
	enc_msg.AddString("valid_value", "Shift-JIS");
	enc_msg.AddString("valid_value", "EUC");
	enc_msg.AddString("valid_value", "Windows 1251");
	enc_msg.AddString("default", "ISO 8859-1");
*/

	BMessage profile;
	profile.AddString("name", "profile");
	profile.AddString("description", "User Profile");
	profile.AddInt32("type", B_STRING_TYPE);
	profile.AddString("default", "IM Kit: AIM user");
	profile.AddBool("multi_line", true);

		
	main_msg.AddMessage("setting",&user_msg);
	main_msg.AddMessage("setting",&pass_msg);
//	main_msg.AddMessage("setting",&enc_msg);
	main_msg.AddMessage("setting", &profile);
	
	return main_msg;
}

status_t AIMProtocol::UpdateSettings( BMessage & msg ) {
	const char * screenname = NULL;
	const char * password = NULL;
	const char * encoding = NULL;
	const char *profile = NULL;
	
	msg.FindString("screenname", &screenname);
	password = msg.FindString("password");
	encoding = msg.FindString("encoding");
	profile = msg.FindString("profile");
	
	if ( screenname == NULL || password == NULL || encoding == NULL || profile == NULL)
		// invalid settings, fail
		return B_ERROR;
	
	if ( strcmp(encoding,"ISO 8859-1") == 0 )
	{
//		fEncoding = B_ISO1_CONVERSION;
	} else
	if ( strcmp(encoding,"UTF-8") == 0 )
	{
//		fEncoding = 0xffff;
	} else
	if ( strcmp(encoding,"JIS") == 0 )
	{
//		fEncoding = B_JIS_CONVERSION;
	} else
	if ( strcmp(encoding,"Shift-JIS") == 0 )
	{
//		fEncoding = B_SJIS_CONVERSION;
	} else
	if ( strcmp(encoding,"EUC") == 0 )
	{
//		fEncoding = B_EUC_CONVERSION;
	} else
	if ( strcmp(encoding,"Windows 1251") == 0 )
	{
//		fEncoding = B_MS_WINDOWS_1251_CONVERSION;
	} else
	{ // invalid encoding value
		return B_ERROR;
	}
	
	if ( fThread )
	{
		// we really should disconnect here if we're connected :/
		// ICQ won't like us changing UIN in the middle of everything.
	}
	

	if (fScreenName) free(fScreenName);
	fScreenName = strdup(screenname);
	
	if (fPassword) free(fPassword);
	fPassword = strdup(password);
	
	fManager->SetProfile(profile);
	
	return B_OK;
}

uint32 AIMProtocol::GetEncoding() 
{
	return fEncoding;
}

status_t AIMProtocol::Error(const char *error) {
	BMessage msg(IM::ERROR);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("error", error);
	
	fMsgr.SendMessage(&msg);
};

status_t AIMProtocol::Progress(const char *id, const char *message,
	float progress) {

	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::PROGRESS );
	msg.AddString("protocol", kProtocolName);
	msg.AddString("progressID", id);
	msg.AddString("message", message);
	msg.AddFloat("progress", progress);
	msg.AddInt32("state", IM::impsConnecting );
	
	fMsgr.SendMessage(&msg);
	
	return B_OK;
};


status_t AIMProtocol::StatusChanged(const char *nick, online_types status) {
	BMessage msg(IM::MESSAGE);
	msg.AddString("protocol", "AIM");

	//LOG("AIM", liMedium, "StatusChanged: %s vs %s\n", nick, NormalizeNick(nick).String());

	if (strcmp(nick, fScreenName) == 0) {
		msg.AddInt32("im_what", IM::STATUS_SET);
	} else {
		msg.AddInt32("im_what", IM::STATUS_CHANGED);
		msg.AddString("id", NormalizeNick(nick));
	};

	switch (status) {
		case AMAN_ONLINE: {
			msg.AddString("status", ONLINE_TEXT);
		} break;
		case AMAN_AWAY: {
//		case IDLE: {
			msg.AddString("status", AWAY_TEXT);
		} break;
		case AMAN_OFFLINE: {
			msg.AddString("status", OFFLINE_TEXT);
		} break;
		
		default: {
			return B_ERROR;
		};
	};

	fMsgr.SendMessage(&msg);
	
	return B_OK;
};

status_t AIMProtocol::MessageFromUser(const char *nick, const char *msg) {
	BMessage im_msg(IM::MESSAGE);
	im_msg.AddInt32("im_what", IM::MESSAGE_RECEIVED);
	im_msg.AddString("protocol", "AIM");
	im_msg.AddString("id", NormalizeNick(nick));
	im_msg.AddString("message", msg);
	im_msg.AddInt32("charset", fEncoding);
	
	fMsgr.SendMessage(&im_msg);											

	return B_OK;
};

status_t AIMProtocol::UserIsTyping(const char *nick, typing_notification type) {
	BMessage im_msg(IM::MESSAGE);
	im_msg.AddString("protocol", "AIM");
	im_msg.AddString("id", NormalizeNick(nick));

	switch (type) {
		case STILL_TYPING:
		case STARTED_TYPING: {
			im_msg.AddInt32("im_what", IM::CONTACT_STARTED_TYPING);
		} break;
		case FINISHED_TYPING:
		default: {
			im_msg.AddInt32("im_what", IM::CONTACT_STOPPED_TYPING);
		} break;
	};
	
	fMsgr.SendMessage(&im_msg);
	
	return B_OK;
};

status_t AIMProtocol::SSIBuddies(list<BString> buddies) {
	list <BString>::iterator i;

	BMessage serverBased(IM::SERVER_BASED_CONTACT_LIST);
	serverBased.AddString("protocol", "AIM");

	for (i = buddies.begin(); i != buddies.end(); i++) {
		LOG("AIM", liLow, "Got server side buddy %s", NormalizeNick(i->String()).String());
		serverBased.AddString("id", NormalizeNick(i->String()));
	};
			
	fMsgr.SendMessage(&serverBased);
};

BString AIMProtocol::NormalizeNick(const char *nick) {
	BString normal = nick;
	
	normal.ReplaceAll(" ", "");
	normal.ToLower();
	
	map<string,BString>::iterator i = fNickMap.find(normal.String());
	
	if ( i == fNickMap.end() ) {
		// add 'real' nick if it's not already there
		LOG("AIM", liDebug, "Adding normal (%s) vs screen (%s)", normal.String(), nick );
		fNickMap[string(normal.String())] = BString(nick);
	}
	
	LOG("AIM", liDebug, "Screen (%s) to normal (%s)", nick, normal.String() );
	
	return normal;
};

BString AIMProtocol::GetScreenNick( const char *nick ) {
	map<string,BString>::iterator i = fNickMap.find(nick);
	
	if ( i != fNickMap.end() ) {
		// found the nick
		LOG("AIM", liDebug, "Converted normal (%s) to screen (%s)", nick, (*i).second.String() );
		return (*i).second;
	}
	
	LOG("AIM", liDebug, "Nick (%s) not found in fNickMap, not converting", nick );
	
	return BString(nick);
};
