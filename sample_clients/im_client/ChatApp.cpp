#include "ChatApp.h"

ChatApp::ChatApp()
:	BApplication("application/x-vnd.m_eiman.sample_im_client"),
	fMan( new IM::Manager(BMessenger(this)) ),
	fIsQuiting(false)
{
	add_system_beep_event(kImNewMessageSound, 0);

	fMan->StartListening();
	
	BMessage msg(IM::ADD_AUTOSTART_APPSIG);
	msg.AddString("app_sig", "application/x-vnd.m_eiman.sample_im_client");
	
	fMan->SendMessage( &msg );
}

ChatApp::~ChatApp()
{
	fMan->Lock();
	fMan->Quit();
}

bool
ChatApp::QuitRequested()
{
	fIsQuiting = true;
	
	fMan->StopListening();
	
	return BApplication::QuitRequested();
}

bool
ChatApp::IsQuiting()
{
	return fIsQuiting;
}

void
ChatApp::RefsReceived( BMessage * msg )
{
	entry_ref ref;
	for ( int i=0; msg->FindRef("refs", i, &ref ) == B_OK; i++ )
		msg->AddRef("contact", &ref);
	
	msg->what = IM::MESSAGE;
	msg->AddInt32("im_what", IM::MESSAGE_RECEIVED);
	msg->AddBool("user_opened", true);
	
	PostMessage(msg);
}

void
ChatApp::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case IM::MESSAGE:
		{
			entry_ref ref;
			
			if ( msg->FindRef("contact",&ref) != B_OK )
			{ // skip messages not to specific contacts
				return;
			}
			
			int32 im_what=-1;
			
			msg->FindInt32("im_what",&im_what);
			
			switch ( im_what )
			{
				case IM::CONTACT_INFO:
				{ // handle contact info updates
					//LOG("sample_client", DEBUG, "Got contact info:",msg);
					
					const char * first_name = msg->FindString("first name");
					const char * last_name = msg->FindString("last name");
					const char * email = msg->FindString("email");
					const char * nick = msg->FindString("nick");
					
					if ( first_name || last_name )
					{
						char full_name[256];
						
						full_name[0] = 0;
						
						if ( first_name )
							strcat(full_name, first_name);
						
						if ( first_name && last_name )
							strcat(full_name, " ");
							
						if ( last_name )
							strcat(full_name, last_name);
						
						if ( strlen( full_name ) > 0 )
							setAttributeIfNotPresent( ref, "META:name", full_name );
					}
					
					if ( email )
					{
						if ( strlen( email ) > 0 )
							setAttributeIfNotPresent( ref, "META:email", email );
					}
					
					if ( nick )
					{
						if ( strlen( nick ) > 0 )
							setAttributeIfNotPresent( ref, "META:nickname", nick );
					}
				}	return;
				
				case IM::GET_CONTACT_INFO:
				case IM::STATUS_CHANGED:
					// ignore these so we don't open new windows that aren't needed.
					return;
				
				default:
					break;
			}
			
			ChatWindow * win = findWindow(ref);
			
			if ( !win && (im_what == IM::MESSAGE_RECEIVED) )
			{ // open new window on message received or user request
				LOG("sample_client", MEDIUM, "Creating new window to handle message");
				win = new ChatWindow(ref);
				if ( win->Lock() )
				{
					win->Show();
					win->SetFlags(win->Flags() ^ B_AVOID_FOCUS);
					win->PostMessage(msg);
					win->Unlock();
					
					bool user_opened;
					
					if ( msg->FindBool("user_opened",&user_opened) != B_OK )
					{ // play sound if not opened by user
						system_beep(kImNewMessageSound);
					} else {
//						Was opened by the user, so make focus
						win->Activate(true);
					};
				} else
				{
					LOG("sample_client", LOW, "This is a fatal error that should never occur. Lock fail on new win.");
				}
				
			} else
			{
				if ( win->Lock() )
				{
					win->PostMessage(msg);
					if ( win->IsMinimized() )
					{ // window is hidden, move to this workspace and show it
						win->SetWorkspaces( B_CURRENT_WORKSPACE );
						win->SetFlags(win->Flags() | B_AVOID_FOCUS);
						win->Minimize(false);
						win->SetFlags(win->Flags() ^ B_AVOID_FOCUS);
					}
					win->Unlock();
				} else
				{
					LOG("sample_client", LOW, "This is a fatal error that should never occur. Lock fail on old win.");
				}
			}
		}	break;
		default:
			BApplication::MessageReceived(msg);
	}
}

ChatWindow *
ChatApp::findWindow( entry_ref & ref )
{
	for ( int i=0; i<CountWindows(); i++ )
	{
		ChatWindow * win = (ChatWindow*)WindowAt(i);
		
		if ( win->handlesRef(ref) )
			return win;
	}
	
	return NULL;
}

void
ChatApp::Flash( BMessenger msgr )
{
	fMan->FlashDeskbar(msgr);
}

void
ChatApp::NoFlash( BMessenger msgr )
{
	fMan->StopFlashingDeskbar(msgr);
}
