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
	: fThread(0) {
	
	fPassword = NULL;
	fScreenName = NULL;
};

AIMProtocol::~AIMProtocol() {
	if (fScreenName) free(fScreenName);
	if (fPassword) free(fPassword);
}

status_t AIMProtocol::Init(BMessenger msgr) {
	fMsgr = msgr;
	LOG("AIM", MEDIUM, "AIMProtocol::Init() start");
	
	fManager = new AIMManager(dynamic_cast<AIMHandler *>(this));
	fManager->Run();


	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what",IM::STATUS_SET);
	msg.AddString("protocol","AIM");
	msg.AddString("status",ONLINE_TEXT);
//	fMsgr.SendMessage( &msg );
	
	return B_OK;
}

status_t AIMProtocol::Shutdown() {
	fManager->LogOff();
	if (fManager->Lock()) fManager->Quit();
	
	LOG("icq", MEDIUM, "AIMProtocol::Shutdown() done");
		
	return B_OK;
}

status_t AIMProtocol::Process(BMessage * msg) {
	switch (msg->what) {
		case IM::MESSAGE: {
			int32 im_what=0;
			
			msg->FindInt32("im_what",&im_what);
			
			switch (im_what) {
				case IM::REGISTER_CONTACTS:
				{
					int32 count = 0;
					msg->GetInfo("id", NULL, &count);
								
					if (count > 0) {
						list<char *> buddies;
						printf("%i buddies\n", count);
						for ( int i=0; msg->FindString("id",i); i++ )
						{
							const char * id = msg->FindString("id",i);
						printf("\t%i: %s\n", i, id);
							buddies.push_back(strdup(id));
						};
						fManager->AddBuddies(buddies);
					} else {
						printf("One buddy: %s\n", msg->FindString("id"));
						fManager->AddBuddy(msg->FindString("id"));
					};
				}	break;
				
				case IM::SET_STATUS: {
					LOG("AIM", HIGH, "Set status");
					const char *status = msg->FindString("status");
					
					if (strcmp(status, OFFLINE_TEXT) == 0) {
						fManager->LogOff();
					} else
					if (strcmp(status, AWAY_TEXT) == 0) {
					} else
					if (strcmp(status, ONLINE_TEXT) == 0) {	
						fManager->Login("login.oscar.aol.com", (uint16)5190,
							fScreenName, fPassword);
					} else
					{
						LOG("AIM", LOW, "Invalid status when setting status: '%s'", status);
					}
				} break;

				case IM::GET_CONTACT_INFO:
				{
					LOG("AIM", HIGH, "Getting contact info");
					const char * id = msg->FindString("id");
					
					BMessage *msg = new BMessage(IM::MESSAGE);
					msg->AddInt32("im_what", IM::CONTACT_INFO);
					msg->AddString("protocol", "AIM");
					msg->AddString("id", id);
					msg->AddString("nick", id);
					msg->AddString("first name", id);
					//msg->AddString("last name", id);

					fMsgr.SendMessage(msg);
				}	break;
		
				case IM::SEND_MESSAGE:
				{
					const char * message_text = msg->FindString("message");
					const char * id = msg->FindString("id");
					
					if ( !id )
						return B_ERROR;
					
					if ( !message_text )
						return B_ERROR;
					
					fManager->MessageUser(id, message_text);
					
					msg->RemoveName("contact");
					msg->ReplaceInt32("im_what", IM::MESSAGE_SENT);
					
					fMsgr.SendMessage(msg);
					
					fManager->RequestBuddyIcon(id);
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
	
	BMessage enc_msg;
	enc_msg.AddString("name","encoding");
	enc_msg.AddString("description","Text encoding");
	enc_msg.AddInt32("type", B_STRING_TYPE);
	enc_msg.AddString("valid_value", "ISO 8859-1");
	enc_msg.AddString("valid_value", "UTF-8");
	enc_msg.AddString("valid_value", "JIS");
	enc_msg.AddString("valid_value", "Shift-JIS");
	enc_msg.AddString("valid_value", "EUC");
	enc_msg.AddString("default", "ISO 8859-1");
	
	main_msg.AddMessage("setting",&user_msg);
	main_msg.AddMessage("setting",&pass_msg);
	main_msg.AddMessage("setting",&enc_msg);
	
	return main_msg;
}

status_t AIMProtocol::UpdateSettings( BMessage & msg ) {
	const char * screenname = NULL;
	const char * password = NULL;
	const char * encoding = NULL;
	
	msg.FindString("screenname", &screenname);
	password = msg.FindString("password");
	encoding = msg.FindString("encoding");
	
	if ( screenname == NULL || password == NULL || encoding == NULL )
		// invalid settings, fail
		return B_ERROR;
	
	if ( strcmp(encoding,"ISO 8859-1") == 0 )
	{
//		fClient.setEncoding( B_ISO1_CONVERSION );
	} else
	if ( strcmp(encoding,"UTF-8") == 0 )
	{
//		fClient.setEncoding( 0xffff );
	} else
	if ( strcmp(encoding,"JIS") == 0 )
	{
//		fClient.setEncoding( B_JIS_CONVERSION );
	} else
	if ( strcmp(encoding,"Shift-JIS") == 0 )
	{
//		fClient.setEncoding( B_SJIS_CONVERSION );
	} else
	if ( strcmp(encoding,"EUC") == 0 )
	{
//		fClient.setEncoding( B_EUC_CONVERSION );
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
	
	return B_OK;
}

uint32 AIMProtocol::GetEncoding() 
{
//	return fClient.fEncoding;
	return B_ISO1_CONVERSION;
}

status_t AIMProtocol::StatusChanged(const char *nick, online_types status) {
	BMessage msg(IM::MESSAGE);
	msg.AddString("protocol", "AIM");

printf("Nick for status change: \"%s\" vs \"%s\"\n", nick, fScreenName);
	if (strcmp(nick, fScreenName) == 0) {
		msg.AddInt32("im_what", IM::STATUS_SET);
	} else {
		msg.AddInt32("im_what", IM::STATUS_CHANGED);
		msg.AddString("id", nick);
	};

	switch (status) {
		case ONLINE: {
			msg.AddString("status", ONLINE_TEXT);
		} break;
		case AWAY:
		case IDLE: {
			msg.AddString("status", AWAY_TEXT);
		} break;
		case OFFLINE: {
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
	printf("AIMProt: Msg from %s content: \"%s\"\n", nick, msg);
	BMessage im_msg(IM::MESSAGE);
	im_msg.AddInt32("im_what", IM::MESSAGE_RECEIVED);
	im_msg.AddString("protocol", "AIM");
	im_msg.AddString("id", nick);
	im_msg.AddString("message", msg);
	im_msg.AddInt32("charset",B_ISO1_CONVERSION);
	
	fMsgr.SendMessage(&im_msg);											

	return B_OK;
};

status_t AIMProtocol::UserIsTyping(const char *nick, typing_notification type) {
	BMessage im_msg(IM::MESSAGE);
	im_msg.AddString("protocol", "AIM");
	im_msg.AddString("id", nick);

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

