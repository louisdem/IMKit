#include "Yahoo.h"

#include <libim/Constants.h>
#include <libim/Helpers.h>
#include "YahooConnection.h"

const char *  kProtocolName = "yahoo";

extern "C" IM::Protocol * load_protocol()
{
	return new Yahoo();
}

extern "C" void register_callbacks();

Yahoo::Yahoo()
:	IM::Protocol( IM::Protocol::MESSAGES | IM::Protocol::OFFLINE_MESSAGES),
	fYahooID(""),
	fPassword(""),
	fYahoo(NULL)
{
	register_callbacks();
}

Yahoo::~Yahoo()
{
	if ( fYahoo )
		delete fYahoo;
}

status_t
Yahoo::Init( BMessenger msgr )
{
	fServerMsgr = msgr;
	
	return B_OK;
}

status_t
Yahoo::Shutdown()
{
	if ( fYahoo )
	{
		YahooConnection * oldYahoo = fYahoo;
		fYahoo = NULL;
		delete oldYahoo;
	}
	
	return B_OK;
}

status_t
Yahoo::Process( BMessage * msg )
{
	switch ( msg->what )
	{
		case IM::MESSAGE:
		{
			int32 im_what = 0;
			
			msg->FindInt32("im_what", &im_what );
			
			switch ( im_what )
			{
				case IM::SET_STATUS:
				{
					const char *status = msg->FindString("status");
					LOG(kProtocolName, liMedium, "Set status to %s", status);
					
					if (strcmp(status, OFFLINE_TEXT) == 0) 
					{
						if ( fYahoo )
						{
							YahooConnection * oldYahoo = fYahoo;
							fYahoo = NULL;
							delete oldYahoo;
						}
					} else 
					if (strcmp(status, AWAY_TEXT) == 0) 
					{
						if ( fYahoo )
						{
							fYahoo->SetAway( true );
						}
					} else 
					if (strcmp(status, ONLINE_TEXT) == 0) 
					{
						if ( !fYahoo )
						{
							if ( fYahooID != "" )
							{
								Progress("Yahoo Login", "Yahoo: Connecting..", 0.50);
								
								fYahoo = new YahooConnection(
									this,
									fYahooID.String(), 
									fPassword.String()
								);
							}
						} else 
						{
							fYahoo->SetAway( false );
						}
					} else
					{
						LOG(kProtocolName, liHigh, "Invalid status when setting status: '%s'", status);
					}
				}	break;
				
				case IM::SEND_MESSAGE:
					if ( fYahoo )
					{
						fYahoo->Message( msg->FindString("id"), msg->FindString("message") );
					} else
						return B_ERROR;
					break;
				case IM::REGISTER_CONTACTS:
					if ( fYahoo )
					{
						const char * buddy=NULL;
						
						for ( int i=0; msg->FindString("id", i, &buddy) == B_OK; i++ )
						{
							fYahoo->AddBuddy( buddy );
						}
					} else
						return B_ERROR;
					break;
				case IM::UNREGISTER_CONTACTS:
					if ( fYahoo )
					{
						const char * buddy=NULL;
						
						for ( int i=0; msg->FindString("id", i, &buddy) == B_OK; i++ )
						{
							fYahoo->RemoveBuddy( buddy );
						}
					} else
						return B_ERROR;
					break;
				default:
					// we don't handle this im_what code
					return B_ERROR;
			}
		}	break;
		
		default:
			// we don't handle this what code
			return B_ERROR;
	}
	
	return B_OK;
}

const char *
Yahoo::GetSignature()
{
	return kProtocolName;
}

const char *
Yahoo::GetFriendlySignature()
{
	return "Yahoo";
}

BMessage
Yahoo::GetSettingsTemplate()
{
	BMessage main_msg(IM::SETTINGS_TEMPLATE);
	
	BMessage user_msg;
	user_msg.AddString("name","yahooID");
	user_msg.AddString("description", "Yahoo! ID");
	user_msg.AddInt32("type",B_STRING_TYPE);
	
	BMessage pass_msg;
	pass_msg.AddString("name","password");
	pass_msg.AddString("description", "Password");
	pass_msg.AddInt32("type",B_STRING_TYPE);
	pass_msg.AddBool("is_secret", true);
	
	main_msg.AddMessage("setting", &user_msg);
	main_msg.AddMessage("setting", &pass_msg);
	
	return main_msg;
}

status_t
Yahoo::UpdateSettings( BMessage & msg )
{
	const char *yahooID = NULL;
	const char *password = NULL;

	msg.FindString("yahooID", &yahooID);
	msg.FindString("password", &password);
	
	if ( (yahooID == NULL) || (password == NULL) ) {
//		invalid settings, fail
		LOG( kProtocolName, liHigh, "Invalid settings!");
		return B_ERROR;
	};
	
	fYahooID = yahooID;
	fPassword = password;
	
	return B_OK;
}

uint32
Yahoo::GetEncoding()
{
	return 0xffff; // No conversion, Yahoo handles UTF-8
}

// YahooManager stuff

void
Yahoo::Error( const char * message, const char * who )
{
	LOG("Yahoo", liDebug, "Yahoo::Error(%s,%s)", message, who);
	
	BMessage msg(IM::ERROR);
	msg.AddString("protocol", kProtocolName);
	if ( who )
		msg.AddString("id", who);
	msg.AddString("message", message);
	
	fServerMsgr.SendMessage( &msg );
}

void
Yahoo::GotMessage( const char * from, const char * message )
{
	LOG("Yahoo", liDebug, "Yahoo::GotMessage()");
	
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::MESSAGE_RECEIVED);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("id", from);
	msg.AddString("message", message);
	
	fServerMsgr.SendMessage( &msg );
}

void
Yahoo::MessageSent( const char * to, const char * message )
{
	LOG("Yahoo", liDebug, "Yahoo::GotMessage()");
	
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::MESSAGE_SENT);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("id", to);
	msg.AddString("message", message);
	
	fServerMsgr.SendMessage( &msg );
}

void
Yahoo::LoggedIn()
{
	LOG("Yahoo", liDebug, "Yahoo::LoggedIn()");
	
	Progress("Yahoo Login", "Yahoo: Logged in!", 1.00);
	
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::STATUS_SET);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("status", ONLINE_TEXT);
	
	fServerMsgr.SendMessage( &msg );
}

void
Yahoo::SetAway(bool away)
{
	LOG("Yahoo", liDebug, "Yahoo::SetAway()");

	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::STATUS_SET);
	msg.AddString("protocol", kProtocolName);
	if ( away )
		msg.AddString("status", AWAY_TEXT);
	else
		msg.AddString("status", ONLINE_TEXT);
	
	fServerMsgr.SendMessage( &msg );
}

void
Yahoo::LoggedOut()
{
	LOG("Yahoo", liDebug, "Yahoo::LoggedOut()");

	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::STATUS_SET);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("status", OFFLINE_TEXT);
	
	fServerMsgr.SendMessage( &msg );
	
	if ( fYahoo )
	{
		YahooConnection * oldYahoo = fYahoo;
		fYahoo = NULL;
		delete oldYahoo;
	}
}

void
Yahoo::GotBuddyList( list<string> & /*buddies*/ )
{
	LOG("Yahoo", liDebug, "Yahoo::GotBuddyList()");
}

void
Yahoo::BuddyStatusChanged( const char * who, const char * status )
{
	LOG("Yahoo", liDebug, "Yahoo::BuddyStatusChanged()");
	
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::STATUS_CHANGED);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("id", who);
	msg.AddString("status", status);
	
	fServerMsgr.SendMessage( &msg );
}

void
Yahoo::Progress( const char * id, const char * message, float progress )
{
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::PROGRESS );
	msg.AddString("protocol", kProtocolName);
	msg.AddString("progressID", id);
	msg.AddString("message", message);
	msg.AddFloat("progress", progress);
	msg.AddInt32("state", IM::impsConnecting );
	
	fServerMsgr.SendMessage(&msg);
}
