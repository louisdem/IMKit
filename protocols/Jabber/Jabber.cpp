#include "Jabber.h"
#include <stdio.h>
#include <Constants.h>
#include <Helpers.h>

#include "States.h"
#include "string.h"

const char *  kProtocolName = "Jabber";

int64 idsms=0;

extern "C" IM::Protocol * load_protocol()
{
	return new Jabber();
}


Jabber::Jabber()
:	IM::Protocol( IM::Protocol::MESSAGES | IM::Protocol::SERVER_BUDDY_LIST | IM::Protocol::OFFLINE_MESSAGES),
	JabberHandler("jabberHandler"),
	fUsername(""),
	fServer(""),
	fPassword("")
{
	fRostered=false;
}

Jabber::~Jabber()
{}

status_t
Jabber::Init( BMessenger msgr )
{
	fServerMsgr = msgr;
	return B_OK;
}

status_t
Jabber::Shutdown()
{
	return B_OK;
}


status_t
Jabber::Process( BMessage * msg )
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
							
						SetStatus(S_OFFLINE,OFFLINE_TEXT); //do the log-out?
					} 
					else 
					if (strcmp(status, AWAY_TEXT) == 0) 
					{
						if(IsAuthorized()){
						
						 const char *away_msg;						 
						 if(msg->FindString("away_msg",&away_msg) == B_OK)
						 		 SetStatus(S_AWAY,away_msg); 
						 	 else
						 		 SetStatus(S_AWAY,AWAY_TEXT); 
						 
						 			
						 SetAway(true);
						} 
					} 
					else 
					if (strcmp(status, ONLINE_TEXT) == 0) 
					{
							
							SetStatus(S_ONLINE,ONLINE_TEXT); //do the login!
							if(IsAuthorized()) SetAway(false); 
					} 
					else
					{
						Error("Invalid",NULL);
						LOG(kProtocolName, liHigh, "Invalid status when setting status: '%s'", status);
					}
				}	break;
				
				case IM::SEND_MESSAGE:
				{
						const char * buddy=msg->FindString("id");
						const char * sms=msg->FindString("message");
						JabberMessage jm;
						jm.SetTo(buddy);
						jm.SetFrom(GetJid());
						jm.SetBody(sms);
						TimeStamp(jm);
						
						//not the right place.. see Jabber::Message
						JabberContact *contact=getContact(buddy);
						
						//tmp: new mess id!
						BString messid("imkit_");
						messid << idsms;
						idsms++;
						
						if(contact)
							jm.SetID(messid);
												
						SendMessage(jm);
						
						MessageSent(buddy,sms);
				} 
				break;
				case IM::REGISTER_CONTACTS:
					
					{
					
					//debugger("REGISTER");
					type_code garbage;
					int32 count = 0;
					msg->GetInfo("id", &garbage, &count);
					
					
						
					if (count > 0 ) {
						list<char *> buddies;
						for ( int i=0; msg->FindString("id",i); i++ )
						{
							const char * id = msg->FindString("id",i);
							JabberContact* contact=getContact(id);
							if(contact)
								  BuddyStatusChanged(contact);
							else
							{
								//Here is not easy to understand!
								//msg->PrintToStream();
								//if(fRostered){
								// we need to send a new subscription?
								//msg->PrintToStream();
								//debugger("Register Unknown!");
								//} 
							} 
						};
						
					} 
					else 
						return B_ERROR;
					} 	
					break;
				case IM::UNREGISTER_CONTACTS:
				
						{
						const char * buddy=NULL;
						
						for ( int i=0; msg->FindString("id", i, &buddy) == B_OK; i++ )
						{
							BuddyStatusChanged(buddy,OFFLINE_TEXT);
						}
					} 
					
					break;
				case IM::USER_STARTED_TYPING: 
				{
						const char * id=NULL;
						
						if( msg->FindString("id", &id) == B_OK )
						{
						 JabberContact* contact=getContact(id);
						 if(contact)
							StartComposingMessage(contact);
						}
				} 
				break;
				case IM::USER_STOPPED_TYPING: 
				{
						const char * id=NULL;
						
						if( msg->FindString("id", &id) == B_OK )
						{
						 JabberContact* contact=getContact(id);
						 if(contact && (contact->GetLastMessageID().ICompare("")!=0)){
							StopComposingMessage(contact);
							contact->SetLastMessageID("");
							
							}
						}
				} 
				break;
				
				case IM::GET_CONTACT_INFO:
					//debugger("Get Contact Info! ;)");
					SendContactInfo(msg->FindString("id"));
				break;
				
				case IM::SEND_AUTH_ACK:
				{
					if(!IsAuthorized())
						return B_ERROR;
					
					const char * id = msg->FindString("id");
					int32 button = msg->FindInt32("which");
					
					if (button == 0) {
						
						//Authorization granted
						AcceptSubscription(id);
						BMessage im_msg(IM::MESSAGE);
						im_msg.AddInt32("im_what", IM::CONTACT_AUTHORIZED);
						im_msg.AddString("protocol", kProtocolName);
						im_msg.AddString("id", id);
						im_msg.AddString("message", "");
						fServerMsgr.SendMessage(&im_msg);
						
						//now we want to see you! ;)
						AddContact(id,id,"");
														
					} 
					else 
					{
						//Authorization rejected
						Error("Authorization rejected!",id);
					}
						
					
				}
				break;
				default:
					// we don't handle this im_what code
					msg->PrintToStream();	
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
Jabber::GetSignature()
{
	return kProtocolName;
}




BMessage
Jabber::GetSettingsTemplate()
{
	BMessage main_msg(IM::SETTINGS_TEMPLATE);
	
	BMessage user_msg;
	user_msg.AddString("name","username");
	user_msg.AddString("description", "Username"); 
	user_msg.AddInt32("type",B_STRING_TYPE);
	
	BMessage serv_msg;
	serv_msg.AddString("name","server");
	serv_msg.AddString("description", "Server"); 
	serv_msg.AddInt32("type",B_STRING_TYPE);
	
	BMessage pass_msg;
	pass_msg.AddString("name","password");
	pass_msg.AddString("description", "Password");
	pass_msg.AddInt32("type",B_STRING_TYPE);
	pass_msg.AddBool("is_secret", true);
	
	BMessage res;
	res.AddString("name","resource");
	res.AddString("description", "Resource");
	res.AddString("default", "IM Kit Jabber AddOn");
	res.AddInt32("type",B_STRING_TYPE);
		
	main_msg.AddMessage("setting", &user_msg);
	main_msg.AddMessage("setting", &serv_msg);
	main_msg.AddMessage("setting", &pass_msg);
	main_msg.AddMessage("setting", &res);
	
	return main_msg;
}

status_t
Jabber::UpdateSettings( BMessage & msg )
{
	const char *username = NULL;
	const char *server=NULL;
	const char *password = NULL;
	const char *res = NULL;
	
	msg.FindString("username", &username);
	msg.FindString("server",&server);
	msg.FindString("password", &password);
	msg.FindString("resource", &res);
	
	if ( (username == NULL) || (password == NULL) || (server == NULL)) {
//		invalid settings, fail
		LOG( kProtocolName, liHigh, "Invalid settings!");
		return B_ERROR;
	};
	
	fUsername = username;
	fServer = server;
	fPassword = password;
	
	SetUsername(fUsername);
	SetHost(fServer);
	SetPassword(fPassword);
	
	if(strlen(res)==0)
		SetResource("IMKit Jabber AddOn");
	else
		SetResource(res);
	
	SetPriority(5);
	SetPort(5222);
	
	return B_OK;
}

uint32
Jabber::GetEncoding()
{
	return 0xffff; // No conversion, Jabber handles UTF-8 ???
}

// JabberManager stuff

void
Jabber::Error( const char * message, const char * who )
{
	LOG("Jabber", liDebug, "Jabber::Error(%s,%s)", message, who);
	
	BMessage msg(IM::ERROR);
	msg.AddString("protocol", kProtocolName);
	if ( who )
		msg.AddString("id", who);
	msg.AddString("error", message);
	
	fServerMsgr.SendMessage( &msg );
}

void
Jabber::GotMessage( const char * from, const char * message )
{
	LOG("Jabber", liDebug, "Jabber::GotMessage()");
	
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::MESSAGE_RECEIVED);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("id", from);
	msg.AddString("message", message);
	
	fServerMsgr.SendMessage( &msg );
}

void
Jabber::MessageSent( const char * to, const char * message )
{
	LOG("Jabber", liDebug, "Jabber::GotMessage()");
	
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::MESSAGE_SENT);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("id", to);
	msg.AddString("message", message);
	
	fServerMsgr.SendMessage( &msg );
}

void
Jabber::LoggedIn()
{
	LOG("Jabber", liDebug, "Jabber::LoggedIn()");
	
	Progress("Jabber Login", "Jabber: Logged in!", 1.00);
	
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::STATUS_SET);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("status", ONLINE_TEXT);
	
	fServerMsgr.SendMessage( &msg );
}

void
Jabber::SetAway(bool away)
{
	LOG("Jabber", liDebug, "Jabber::SetAway()");

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
Jabber::LoggedOut()
{
	LOG("Jabber", liDebug, "Jabber::LoggedOut()");
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::STATUS_SET);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("status", OFFLINE_TEXT);
	fServerMsgr.SendMessage( &msg );
	
}

void
Jabber::BuddyStatusChanged( JabberContact* who )
{
	LOG("Jabber", liDebug, "Jabber::BuddyStatusChanged()");
	
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::STATUS_CHANGED);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("id", who->GetJid());
	
	AddStatusString(who->GetPresence(),&msg);
		
	fServerMsgr.SendMessage( &msg );
}
void
Jabber::BuddyStatusChanged( JabberPresence* jp )
{
	LOG("Jabber", liDebug, "Jabber::BuddyStatusChanged()");
	
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::STATUS_CHANGED);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("id", jp->GetJid());
	
	AddStatusString(jp,&msg);
	
	fServerMsgr.SendMessage( &msg );
}

void
Jabber::AddStatusString(JabberPresence* jp ,BMessage* msg)
{	
	int32 show=jp->GetShow();
	switch(show) 
	{
		case S_XA:
			msg->AddString("status", AWAY_TEXT);
			break;
		case S_AWAY:
			msg->AddString("status",AWAY_TEXT );
			break;
		case S_ONLINE:
			msg->AddString("status",ONLINE_TEXT);
			break;
		case S_DND:
			msg->AddString("status", AWAY_TEXT);
			break;
		case S_CHAT:
			msg->AddString("status", ONLINE_TEXT);
			break;
		case S_SEND: //???
			msg->AddString("status", ONLINE_TEXT);
			break;
		default:
			msg->AddString("status", OFFLINE_TEXT);
	}
}
void
Jabber::BuddyStatusChanged( const char * who, const char * status )
{
	LOG("Jabber", liDebug, "Jabber::BuddyStatusChanged()");
	
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::STATUS_CHANGED);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("id", who);
	msg.AddString("status", status);
	
	fServerMsgr.SendMessage( &msg );
}

void
Jabber::Progress( const char * id, const char * message, float progress )
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

JabberContact*
Jabber::getContact(const char* id)
{
	RosterList *rl=getRosterList();
	JabberContact* contact=NULL;
	for(int i=0;i<rl->CountItems();i++)
	{
		contact=reinterpret_cast<JabberContact*>(getRosterList()->ItemAt(i));
		if(contact->GetJid().Compare(id)==0)
		{
			return contact;								
		}								
	}
	return contact;
}

void			
Jabber::SendContactInfo(const char* id)
{
	JabberContact *jid=getContact(id);
	if(jid)
	{
		BMessage msg(IM::MESSAGE);
		msg.AddInt32("im_what", IM::CONTACT_INFO);
		msg.AddString("protocol", kProtocolName);
		msg.AddString("id", id);
		msg.AddString("nick", jid->GetName());     //just nick ??
		fServerMsgr.SendMessage(&msg);	
	}
}
//CALLBACK!
void
Jabber::Authorized()
{
	SetAway(false);
	LoggedIn();
	JabberHandler::Authorized();;
}

void
Jabber::Message(JabberMessage * message){
	
	if(message->GetBody()!="") //we have something to tell
		GotMessage(message->GetFrom().String(),message->GetBody().String());
	
	if(message->GetError()!="") //not a nice situation..
	{
		Error(message->GetError().String(),message->GetFrom().String());
		return;
	}
	
	LOG(kProtocolName, liHigh, "GETX: '%s'",message->GetX().String()) ;
				
		
	if(message->GetX().ICompare("composing") == 0)
	{
		//someone send a composing event.
		
		if(message->GetBody() == "") //Notification
		{
		 LOG(kProtocolName, liHigh,"CONTACT_STARTED_TYPING");
		 BMessage im_msg(IM::MESSAGE);
		 im_msg.AddInt32("im_what", IM::CONTACT_STARTED_TYPING);
		 im_msg.AddString("protocol", kProtocolName);
		 im_msg.AddString("id", message->GetFrom());
		 fServerMsgr.SendMessage(&im_msg);	
		}
		else //Request
		{
			//	where we put the last messge id? on the contact (is it the right place?)
			//	maybe we should make an hash table? a BMesage..
			
			JabberContact *contact=getContact(message->GetFrom().String());
			if(contact)
			contact->SetLastMessageID(message->GetID());
		}
	}
	else
	if(message->GetX().ICompare("jabber:x:event") == 0)
	{
		//not define event this maybe due to:
		// 	unkown event.
		// 	no event (means stop all)
		
		LOG(kProtocolName, liHigh,"CONTACT_STOPPED_TYPING");
			
		BMessage im_msg(IM::MESSAGE);
		im_msg.AddInt32("im_what", IM::CONTACT_STOPPED_TYPING);
		im_msg.AddString("protocol", kProtocolName);
		im_msg.AddString("id", message->GetFrom());
		fServerMsgr.SendMessage(&im_msg);
	}	
	

	
}
void
Jabber::Presence(JabberPresence * presence){
	//printf("JabberPresence\nType: %s\nStatus: %s\nShow: %ld\n",presence->GetType().String(),presence->GetStatus().String(),presence->GetShow());
	BuddyStatusChanged(presence);
}

void
Jabber::Roster(RosterList * roster){

	//debugger("Roster");
	BMessage serverBased(IM::SERVER_BASED_CONTACT_LIST);
	serverBased.AddString("protocol", kProtocolName);
	JabberContact* contact;
	int size = roster->CountItems();
	
	for(int i=0; i < size; i++) {
		contact = reinterpret_cast<JabberContact*>(roster->ItemAt(i));
		serverBased.AddString("id", contact->GetJid());
	}	
	fServerMsgr.SendMessage(&serverBased);		
	
	fRostered=true;
	
}
void
Jabber::Agents(AgentList * agents){
	//debugger("Agents");
}
void
Jabber::Disconnected(const BString & reason){
	//debugger("Disconnected");	
	LoggedOut();
}
void
Jabber::SubscriptionRequest(JabberPresence * presence){
	
	// someone want us!
	BMessage im_msg(IM::MESSAGE);
	im_msg.AddInt32("im_what", IM::AUTH_REQUEST);
	im_msg.AddString("protocol", kProtocolName);
	im_msg.AddString("id", presence->GetJid());
	im_msg.AddString("message", presence->GetStatus());
		
	fServerMsgr.SendMessage(&im_msg);	
}

void
Jabber::Unsubscribe(JabberPresence * presence){
	
	// what should we do when a people unsubscrive from us?
	//debugger("Unsubscribe");
	LOG("Jabber", liDebug, "Jabber::Unsubscribe()");
	
	BMessage msg(IM::MESSAGE);
	msg.AddInt32("im_what", IM::STATUS_CHANGED);
	msg.AddString("protocol", kProtocolName);
	msg.AddString("id", presence->GetJid());
	msg.AddString("status", OFFLINE_TEXT);
	fServerMsgr.SendMessage( &msg );
}

void
Jabber::Registration(JabberRegistration * registration){
	
	// Just created a new account ?
	// or we have ack of a registration? ack of registartion!
	debugger("Registration");
	registration->PrintToStream();
}
