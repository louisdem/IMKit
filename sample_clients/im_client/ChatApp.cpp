#include "ChatApp.h"

ChatApp::ChatApp()
:	BApplication("application/x-vnd.m_eiman.sample_im_client"),
	fMan( new IM::Manager(BMessenger(this)) ),
	fIsQuiting(false)
{
	add_system_beep_event(kImNewMessageSound, 0);

	fMan->StartListening();
	
	// Save settings template
	BMessage autostart;
	autostart.AddString("name", "auto_start");
	autostart.AddString("description", "Auto-start");
	autostart.AddInt32("type", B_BOOL_TYPE);
	autostart.AddBool("default", true);
	
	BMessage appsig;
	appsig.AddString("name", "app_sig");
	appsig.AddString("description", "Application signature");
	appsig.AddInt32("type", B_STRING_TYPE);
	appsig.AddBool("default", "application/x-vnd.m_eiman.sample_im_client");
	
	BMessage iconSize;
	iconSize.AddString("name", "icon_size");
	iconSize.AddString("description", "Menubar Icon Size");
	iconSize.AddInt32("type", B_INT32_TYPE);
	iconSize.AddInt32("default", kLargeIcon);

#if B_BEOS_VERSION > B_BEOS_VERSION_5
#else
	iconSize.AddInt32("valid_value", kSmallIcon);
	iconSize.AddInt32("valid_value", kLargeIcon);
#endif

//	BMessage userColor;
//	userColor.AddString("name", "user_colour");
//	userColor.AddString("description", "User text colour");
//	userColor.AddInt32("type", B_RGB_COLOR_TYPE);

	BMessage useCommand;
	useCommand.AddString("name", "command_sends");
	useCommand.AddString("description", "Command key sends message");
	useCommand.AddInt32("type", B_BOOL_TYPE);
	useCommand.AddBool("default", true);
	
	BMessage tmplate(IM::SETTINGS_TEMPLATE);
	tmplate.AddMessage("setting", &autostart);
	tmplate.AddMessage("setting", &appsig);
	tmplate.AddMessage("setting", &iconSize);
	tmplate.AddMessage("setting", &useCommand);
//	tmplate.AddMessage("setting", &userColor);
	
	im_save_client_template("im_client", &tmplate);
	
	// Make sure default settings are there
	BMessage settings;
	bool temp;

	im_load_client_settings("im_client", &settings);
	if ( !settings.FindString("app_sig") )
		settings.AddString("app_sig", "application/x-vnd.m_eiman.sample_im_client");
	if ( settings.FindBool("auto_start", &temp) != B_OK )
		settings.AddBool("auto_start", true );
	if (settings.FindInt32("icon_size", &fIconBarSize) != B_OK) {
		settings.AddInt32("icon_size", kLargeIcon);
		fIconBarSize = kLargeIcon;
	};
	if (fIconBarSize == 0) fIconBarSize = kLargeIcon;
	
	im_save_client_settings("im_client", &settings);
	// done with template and settings.
}

ChatApp::~ChatApp()
{
	fMan->Lock();
	fMan->Quit();
}

bool
ChatApp::QuitRequested()
{
	RunMap::const_iterator it = fRunViews.begin();
	
	for (; it != fRunViews.end(); ++it) {
		BString key = it->first;
		RunView *rv = it->second;

		if (rv != NULL) {
			BWindow *win = rv->Window();

			if (win != NULL) {
				LOG("im_client", liLow, "RunView for %s has a Parent(), Lock()ing and "
					"RemoveSelf()ing.", key.String());
				win->Lock();
				rv->RemoveSelf();
				win->Unlock();
			} else {
				LOG("im_client", liLow, "RunView for %s doesn't have a Parent()", key.String());
				rv->RemoveSelf();
			};
			
			delete rv;
		} else {
			LOG("im_client", liLow, "RunView for %s was already NULL", key.String());
		};
	};

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
	BNode node;
	attr_info info;
	bool hasValidRefs = false;
	
	for ( int i=0; msg->FindRef("refs", i, &ref ) == B_OK; i++ ) {
		node = BNode(&ref);
		char *type = ReadAttribute(node, "BEOS:TYPE");
		if (strcmp(type, "application/x-person") == 0) {
			if (node.GetAttrInfo("IM:connections", &info) == B_OK) {
				msg->AddRef("contact", &ref);
				hasValidRefs = true;
			}
		} else {
			LOG("im_client", liLow, "Got a ref that wasn't a People file");
		};
		free(type);
	};
	
	if (hasValidRefs == false) return;
	
	msg->what = IM::MESSAGE;
	msg->AddInt32("im_what", IM::MESSAGE_RECEIVED);
	msg->AddBool("user_opened", true);
	
	BMessenger(this).SendMessage(msg);
}

void
ChatApp::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case IM::SETTINGS_UPDATED:
		{	
			BMessage settings;
			im_load_client_settings("im_client", &settings);

			if (settings.FindInt32("icon_size", &fIconBarSize) != B_OK) {
				fIconBarSize = kLargeIcon;
			};
			if (fIconBarSize == 0) fIconBarSize = kLargeIcon;
			
			if (settings.FindBool("command_sends", &fCommandSends) != B_OK) {
				fCommandSends = true;
			};
			
		}	break;
		
		case IM::ERROR:
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
					//LOG("sample_client", liDebug, "Got contact info:",msg);
					
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
			BMessenger _msgr(win); // fix for Lock() failing, hopefully.
			
			if ( !win && (im_what == IM::MESSAGE_RECEIVED) )
			{ // open new window on message received or user request
				LOG("im_client", liMedium, "Creating new window to handle message");
				win = new ChatWindow(ref, fIconBarSize, fCommandSends);
				_msgr = BMessenger(win);
				if ( _msgr.LockTarget() )
				{
					win->Show();
					win->SetFlags(win->Flags() ^ B_AVOID_FOCUS);
					BMessenger(win).SendMessage(msg);
					win->Unlock();
					
					bool user_opened=false;
					
					if ( msg->FindBool("user_opened",&user_opened) != B_OK )
					{ // play sound if not opened by user
						system_beep(kImNewMessageSound);
					} else {
//						Was opened by the user, so make focus
						win->Activate(true);
					};
				} else
				{
					LOG("im_client", liHigh, "This is a fatal error that should never occur. Lock fail on new win.");
				}
			} else
			{
				if (_msgr.LockTarget() )
				{
					BMessenger(win).SendMessage(msg);
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
					LOG("im_client", liHigh, "This is a fatal error that should never occur. Lock fail on old win.");
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
	printf("We should be teh flasher!\n");
	fMan->FlashDeskbar(msgr);
}

void
ChatApp::NoFlash( BMessenger msgr )
{
	fMan->StopFlashingDeskbar(msgr);
}

status_t
ChatApp::StoreRunView(const char *id, RunView *rv) {
	LOG("im_client", liLow, "Setting RunView for %s to %p", id, rv);
	
	BString nick = id;
	fRunViews[nick] = rv;

	return B_OK;
}

RunView *
ChatApp::GetRunView(const char *id) {
	BString nick = id;
	
	LOG("im_client", liLow, "RunView for %s is %p", id, fRunViews[nick]);
	
	return fRunViews[nick];
}
