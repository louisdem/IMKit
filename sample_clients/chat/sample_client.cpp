#include "sample_client.h"

#include <ScrollView.h>
#include <NodeMonitor.h>
#include <stdio.h>
#include <Button.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Beep.h>

#include <libim/Constants.h>
#include <libim/Contact.h>
#include <libim/Helpers.h>

const char *kImNewMessageSound = "IM Message Received";

int main(void)
{
	MyApp app;
	
	app.Run();
	
	return 0;
}

void
setAttributeIfNotPresent( entry_ref ref, const char * attr, const char * value )
{
//	printf("Setting attribute %s to %s\n", attr, value );
	
	BNode node(&ref);
	char data[512];
	
	if ( node.InitCheck() != B_OK )
	{
		LOG("Invalid entry_ref");
		return;
	}
	
	if ( node.ReadAttr(attr,B_STRING_TYPE,0,data,sizeof(data)) > 1 )
	{
//		LOG("  value already present");
		return;
	}
	
	int32 num_written = node.WriteAttr(
		attr, B_STRING_TYPE, 0,
		value, strlen(value)+1
	);
	
	if ( num_written != (int32)strlen(value) + 1 )
	{
		printf("Error writing attribute %s (%s)\n",attr,value);
	} else
	{
		//LOG("Attribute set");
	}
}

MyApp::MyApp()
:	BApplication("application/x-vnd.m_eiman.sample_im_client"),
	fMan( new IM::Manager(BMessenger(this)) ),
	fIsQuiting(false)
{
	add_system_beep(kImNewMessageSound, 0);

	fMan->StartListening();
	
	SettingsWindow * settings = new SettingsWindow("ICQ");
	
	settings->Show();
}

MyApp::~MyApp()
{
	fMan->Lock();
	fMan->Quit();
}

bool
MyApp::QuitRequested()
{
	fIsQuiting = true;
	
	fMan->StopListening();
	
	return BApplication::QuitRequested();
}

bool
MyApp::IsQuiting()
{
	return fIsQuiting;
}

void
MyApp::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case 'newc':
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
//					LOG("Got contact info:",msg);
					
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
			
			if ( !win && (im_what == IM::MESSAGE_RECEIVED || msg->what == 'newc') )
			{ // open new window on message received or user request
				LOG("Creating new window to handle message");
				win = new ChatWindow(ref);
				win->Lock();
				win->Show();
				win->PostMessage(msg);
				win->Unlock();
			} else
			{
				win->Lock();
				win->PostMessage(msg);
				if ( win->IsHidden() )
				{ // window is hidden, move to this workspace and show it
					win->SetWorkspaces( B_CURRENT_WORKSPACE );
					win->Show();
				}
				win->Unlock();
			}
		}	break;
		default:
			BApplication::MessageReceived(msg);
	}
}

ChatWindow *
MyApp::findWindow( entry_ref & ref )
{
	for ( int i=0; i<CountWindows(); i++ )
	{
		ChatWindow * win = (ChatWindow*)WindowAt(i);
		
		if ( win->handlesRef(ref) )
			return win;
	}
	
	return NULL;
}

/// CHAT WINDOW

ChatWindow::ChatWindow( entry_ref & ref )
:	BWindow( 
		BRect(100,100,400,300), 
		"unknown contact - unknown status", 
		B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS
	),
	fEntry(ref),
	fMan( new IM::Manager(BMessenger(this))),
	fChangedNotActivated(false)
{
	// create views
	BRect textRect = Bounds();
	BRect text2Rect;
	BRect inputRect = Bounds();

	textRect.InsetBy(2,2);
	textRect.bottom -= 20;
	textRect.right -= B_V_SCROLL_BAR_WIDTH;
	
	text2Rect = textRect;
	text2Rect.OffsetTo(0,0);
	
	inputRect.top = inputRect.bottom - 20;
	
	fInput = new BTextControl(
		inputRect, "input", "Say", "",
		new BMessage(SEND_MESSAGE),
		B_FOLLOW_LEFT_RIGHT|B_FOLLOW_BOTTOM
	);
	fInput->SetDivider(30);
	fInput->SetViewColor( 215,215,215 );
	
	AddChild( fInput );
	
	fText = new BTextView(
		textRect, "text", text2Rect,
		B_FOLLOW_ALL,B_WILL_DRAW
	);
	fText->SetWordWrap(true);
	fText->MakeEditable(false);
	fText->SetStylable(true);
	BScrollView * scroll = new BScrollView(
		"scroller", fText,
		B_FOLLOW_ALL, 0,
		false, // horiz
		true // vert
	);
	AddChild( scroll );

	fInput->MakeFocus();
	
	// monitor node so we get updates to status etc
	BEntry entry(&ref);
	node_ref node;
	
	entry.GetNodeRef(&node);
	watch_node( &node, B_WATCH_ALL, BMessenger(this) );
	
	// get contact info
	reloadContact();
}

ChatWindow::~ChatWindow()
{
	stop_watching( BMessenger(this) );

	fMan->Lock();
	fMan->Quit();
}

bool
ChatWindow::QuitRequested()
{
	if ( ((MyApp*)be_app)->IsQuiting() )
		return true;
	
	Hide();
	
	return false;
}

void
ChatWindow::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case IM::MESSAGE:
		{
			entry_ref contact;
			
			if ( msg->FindRef("contact",&contact) != B_OK )
				return;
				
			if ( contact != fEntry )
				return;
			
			// this message is related to our Contact
			
			int32 im_what=0;
			
			msg->FindInt32("im_what",&im_what);
			
			char timestr[64];
			time_t now = time(NULL);
			
			rgb_color own_nick_color		= (rgb_color){  0,  0,255};
			rgb_color contact_nick_color	= (rgb_color){255,  0,  0};
			rgb_color own_text_color		= (rgb_color){  0,  0,  0};
			rgb_color contact_text_color	= (rgb_color){  0,  0,  0};
			
			int32 old_sel_start, old_sel_end;
			
			fText->GetSelection(&old_sel_start, &old_sel_end);
			fText->Select(fText->TextLength(),fText->TextLength());
			
			switch ( im_what )
			{
				case IM::MESSAGE_SENT:
				{
					fText->SetFontAndColor( be_plain_font, B_FONT_ALL, &own_nick_color);
					strftime(timestr,sizeof(timestr),"[%H:%M] ", localtime(&now) );
					fText->Insert(timestr);
					fText->Insert("You say: ");
					fText->SetFontAndColor( be_plain_font, B_FONT_ALL, &own_text_color);
					fText->Insert(msg->FindString("message"));
					fText->Insert("\n");
					fText->ScrollToSelection();
				}	break;
				
				case IM::MESSAGE_RECEIVED:
				{
					fText->SetFontAndColor( be_plain_font, B_FONT_ALL, &contact_nick_color);
					strftime(timestr,sizeof(timestr),"[%H:%M] ",  localtime(&now) );
					fText->Insert(timestr);
					fText->Insert(fName);
					fText->Insert(": ");
					fText->SetFontAndColor( be_plain_font, B_FONT_ALL, &contact_text_color);
					fText->Insert(msg->FindString("message"));
					fText->Insert("\n");
					fText->ScrollToSelection();

					if (!IsActive()) {
						fChangedNotActivated = true;
						char str[256];
						sprintf(str, "√ %s", fTitleCache);
						SetTitle(str);
						system_beep(kImNewMessageSound);
					}
				}	break;
			}
			
			if ( old_sel_start != old_sel_end )
			{ // restore selection
				fText->Select( old_sel_start, old_sel_end );
			} else
			{
				fText->Select( fText->TextLength(), fText->TextLength() );
			}
			
			fText->ScrollToSelection();
			
		}	break;
		
		case SEND_MESSAGE:
		{
			BMessage im_msg(IM::MESSAGE);
			im_msg.AddInt32("im_what",IM::SEND_MESSAGE);
			im_msg.AddRef("contact",&fEntry);
			im_msg.AddString("message", fInput->Text() );
			
			if ( fMan->SendMessage(&im_msg) == B_OK )
				fInput->SetText("");
			else
				LOG("Error sending message to im_server");
		}	break;
		
		case B_NODE_MONITOR:
		{
			int32 opcode=0;
			
			if ( msg->FindInt32("opcode",&opcode) != B_OK )
				return;
			
			switch ( opcode )
			{
				case B_ENTRY_REMOVED:
					// oops. should we close down this window now?
					// Nah, we'll just disable everything.
					fInput->SetEnabled(false);
					break;
				case B_ENTRY_MOVED:
				{
					entry_ref ref;
					
					msg->FindInt32("device", &ref.device);
					msg->FindInt64("to directory", &ref.directory);
					ref.set_name( msg->FindString("name") );
					
					fEntry = ref;
					
					BEntry entry(&fEntry);
					if ( !entry.Exists() )
					{
						LOG("Error: New entry invalid");
					}
				}	break;
				case B_STAT_CHANGED:
				case B_ATTR_CHANGED:
					reloadContact();
					break;
			}
		}	break;
		default:
			BWindow::MessageReceived(msg);
	}
}

void
ChatWindow::FrameResized( float w, float h )
{
	fText->SetTextRect( fText->Bounds() );
	fText->ScrollToSelection();
}

void
ChatWindow::WindowActivated(bool active)
{
	if (active && fChangedNotActivated) {
		fChangedNotActivated = false;	
		SetTitle(fTitleCache);

	}
	BWindow::WindowActivated(active);
}

bool
ChatWindow::handlesRef( entry_ref & ref )
{
	return ( fEntry == ref );
}

void
ChatWindow::reloadContact()
{
	IM::Contact c(&fEntry);
	
	char status[512];
	char name[512];
	char nick[512];
	
	int32 num_read;
	
	// read name
	if ( c.GetName(name,sizeof(name)) != B_OK )
		strcpy(name,"Unknown name");
	
	if ( c.GetNickname(nick,sizeof(nick)) != B_OK )
		strcpy(nick,"no nick");
	
	sprintf(fName,"%s (%s)", name, nick );
	
	BNode node(&fEntry);
	
	// read status
	num_read = node.ReadAttr(
		"IM:status", B_STRING_TYPE, 0,
		status, sizeof(status)-1
	);
	
	if ( num_read <= 0 )
		strcpy(status,"Unknown status");
	else
		status[num_read] = 0;
	
	// rename window
	sprintf(fTitleCache,"%s - %s", fName, status);
	
	SetTitle(fTitleCache);
}

//// SETTINGS WINDOW

SettingsWindow::SettingsWindow( const char * protocol )
:	BWindow(
		BRect(150,150,350,100), "Settings", 
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS|B_NOT_RESIZABLE|B_NOT_ZOOMABLE
	),
	fMan( new IM::Manager(BMessenger(this)) )
{
	strcpy(fProtocol,"");
	rebuildUI();
}

SettingsWindow::~SettingsWindow()
{
	fMan->Lock();
	fMan->Quit();
}

bool
SettingsWindow::QuitRequested()
{
	if ( ((MyApp*)be_app)->IsQuiting() )
		return true;
	
	return false;
}

void
SettingsWindow::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{	
		case SELECT_PROTOCOL:
		{
			if ( msg->FindString("protocol") == NULL || strlen(msg->FindString("protocol")) == 0 )
				return;
			
			strcpy( fProtocol, msg->FindString("protocol") );
			
			rebuildUI();
		}	break;
		
		case ALTER_STATUS:
		{
			BMessage smsg(IM::MESSAGE);
			smsg.AddInt32("im_what",IM::SET_STATUS);
			smsg.AddString("protocol",fProtocol);
			smsg.AddString("status",msg->FindString("status"));
			
			fMan->SendMessage(&smsg);
		}	break;
		
		case APPLY_SETTINGS:
		{
			// get template message, walk through it updating a settings msg
			// then send the settings
			
			BMessage settings(IM::SETTINGS), curr;
			
			for ( int i=0; fTemplate.FindMessage("setting",i,&curr) == B_OK; i++ )
			{
				const char * name = curr.FindString("name");
//				const char * desc = curr.FindString("description");
				int32 type=-1;
				curr.FindInt32("type",&type);
				
				if ( dynamic_cast<BTextControl*>(FindView(name)) )
				{ // free-text
					BTextControl * ctrl = (BTextControl*)FindView(name);
				
					switch ( type )
					{
						case B_STRING_TYPE:
							settings.AddString(name, ctrl->Text() );
							break;
						case B_INT32_TYPE:
							settings.AddInt32(name, atoi(ctrl->Text()) );
							break;
						default:
							LOG("Unhandled settings type!");
							return;
					}
				} else
				{
					BMenuField * ctrl = (BMenuField*)FindView(name);
					
					BMenuItem * item = ctrl->Menu()->FindMarked();
					
					if ( !item )
					{
						LOG("Error: No selection in setting");
						return;
					}
					
					switch ( type )
					{
						case B_STRING_TYPE:
							settings.AddString(name, item->Label() );
							break;
						case B_INT32_TYPE:
							settings.AddInt32(name, atoi(item->Label()) );
							break;
						default:
							LOG("Unhandled settings type!");
							return;
					}
				}
			}
			
			//LOG("SETTINGS message", &settings);
			
			BMessage to_send(IM::SET_SETTINGS), reply;
			to_send.AddString("protocol", fProtocol);
			to_send.AddMessage("settings", &settings);
			
			//LOG("SET_SETTINGS message:",&to_send);
			
			fMan->SendMessage( &to_send, &reply );
			
			//LOG("apply settings, reply",&reply);
		}	break;
		
		default:
			BWindow::MessageReceived(msg);
	}
}

void
SettingsWindow::rebuildUI()
{
	char some_text[512];
	sprintf(some_text,"Rebuilding GUI with protocol [%s]", fProtocol);
	LOG(some_text);
	
	// delete old UI
	while ( CountChildren() > 0 )
	{
		BView * view = ChildAt(0);
		RemoveChild(view);
		delete view;
	}
	
	// get list of available protocols
	BMessage req_protocols(IM::GET_LOADED_PROTOCOLS), protocols;
	fMan->SendMessage( &req_protocols, &protocols );
	
	BMenu * protocolMenu = NULL;
	
	if ( protocols.what == IM::ACTION_PERFORMED )
	{ // got list of protocols. yay!
		protocolMenu = new BMenu("Loaded protocols");
		
		for ( int i=0; protocols.FindString("protocol",i); i++ )
		{
			const char * p = protocols.FindString("protocol",i);
			
			BMessage * msg = new BMessage(SELECT_PROTOCOL);
			msg->AddString("protocol",p);
			
			protocolMenu->AddItem( new BMenuItem(p,msg) );
		}
	} else
	{
		LOG("Error: Failed to get list of protocols");
		return;
	}
	
	BMenuField * protoSelect = new BMenuField(
		BRect(0,0,200,20), "", "Select protocol:", protocolMenu
	);
	
	AddChild( protoSelect );
	
	// get template from im_server
	BMessage req_template(IM::GET_SETTINGS_TEMPLATE);
	req_template.AddString("protocol", fProtocol);
	
	BMessage req_settings(IM::GET_SETTINGS);
	req_settings.AddString("protocol", fProtocol);
	
	BMessage templ, settings;
	
	fMan->SendMessage( &req_template, &fTemplate );
	fMan->SendMessage( &req_settings, &settings );
	
	if ( fTemplate.what == IM::ERROR )
	{ // got template, construct GUI
		LOG("SettingsWindow construction failed: Couldn't get template");
		return;
	}
	
/*	LOG("Template", &fTemplate);
		
	LOG("Settings", &settings);
*/		
	BMessage curr;
		
	for ( int i=0; fTemplate.FindMessage("setting",i,&curr) == B_OK; i++ )
	{
		char temp[512];
			
		// get text etc from template
		const char * name = curr.FindString("name");
		const char * desc = curr.FindString("description");
		const char * value = NULL;
		int32 type=-1;
		curr.FindInt32("type",&type);
		
		sprintf(some_text,"Setting %s [%s]", desc, name );
		LOG(some_text);
		
		bool is_free_text = true;
		bool is_secret = false;
		BMenu * menu = NULL;
		
		switch ( type )
		{ // get value from settings if available
			case B_STRING_TYPE:
			{
				LOG("  string setting");
				if ( curr.FindString("valid_value") )
				{ // one-of-provided
					LOG("  one-of-provided");
					is_free_text = false;
					
					menu = new BPopUpMenu(name);
					
					for ( int x=0; curr.FindString("valid_value",x); x++ )
					{
						menu->AddItem( new BMenuItem(curr.FindString("valid_value",x),NULL) );
					}
					
					value = settings.FindString(name);
					
					if ( value )
						menu->FindItem(value)->SetMarked(true);
				} else
				{ // free-text
					LOG("  free-text");
					value = settings.FindString(name);
					if ( !value )
						value = curr.FindString("default");
					if ( curr.FindBool("is_secret",&is_secret) != B_OK )
						is_secret = false;
				}
			}	break;
			case B_INT32_TYPE:
			{
				LOG("  int32 setting");
				if ( curr.FindInt32("valid_value") )
				{ // one-of-provided
					LOG("  one-of-provided");
					is_free_text = false;
					
					menu = new BPopUpMenu(name);
					
					int32 v=0;
					for ( int x=0; curr.FindInt32("valid_value",x,&v) == B_OK; x++ )
					{
						sprintf(temp,"%ld", v);
						menu->AddItem( new BMenuItem(temp,NULL) );
					}
				} else
				{ // free-text
					LOG("  free-text");
					int32 v=0;
					if ( settings.FindInt32(name,&v) == B_OK )
					{
						sprintf(temp,"%ld",v);
						value = temp;
					} else
					if ( curr.FindInt32("default",&v) == B_OK )
					{
						sprintf(temp,"%ld",v);
						value = temp;
					}
					if ( curr.FindBool("is_secret",&is_secret) != B_OK )
						is_secret = false;
				}
			}	break;
		}
		
		if ( !value )
			value = "";
		
		LOG("  creating control");
		// create control
		BView * ctrl;
		if ( is_free_text )
		{ // free-text setting
			ctrl = new BTextControl( BRect(0,0,200,20), name, desc, value, NULL	);
		} else
		{ // select-one-of-provided setting
			ctrl = new BMenuField( BRect(0,0,200,20), name, desc, menu );
		}
		if (is_secret)
		{
			((BTextControl *)ctrl)->TextView()->HideTyping(true);
			((BTextControl *)ctrl)->SetText(value); // err.. Why does HideTyping remove the text?
		}
		AddChild( ctrl );
		ctrl->MoveTo(0, 25+i*21);
		ResizeTo( 200, 25+(i+1)*21 );
		
		LOG("  done.");
	}
	
	// add space for buttons
	ResizeTo( Bounds().Width(), Bounds().Height()+30 );
	
	// apply settings
	BButton * apply = new BButton(
		BRect(0,0,40,25), "", "Apply", new BMessage(APPLY_SETTINGS)
	);
	apply->MoveTo( 0, Bounds().Height()-apply->Bounds().Height() );
	AddChild( apply );
	
	// set online
	/*
	BButton * online = new BButton(
		BRect(0,0,40,25), "", "Online", new BMessage(SET_ONLINE)
	);
	*/
	BMenu * statusMenu = new BPopUpMenu("");
	BMessage * onlineMsg = new BMessage(ALTER_STATUS);	onlineMsg->AddString("status","available");
	BMessage * awayMsg = new BMessage(ALTER_STATUS);	awayMsg->AddString("status","away");
	BMessage * offlineMsg = new BMessage(ALTER_STATUS);	offlineMsg->AddString("status","offline");
	
	statusMenu->AddItem( new BMenuItem("online", onlineMsg) );
	statusMenu->AddItem( new BMenuItem("away", awayMsg) );
	statusMenu->AddItem( new BMenuItem("offline", offlineMsg) );
	
	BMenuField * online = new BMenuField( BRect(0,0,100,25), "_new_status", "Set status:", statusMenu );
	online->MoveTo( Bounds().Width()-online->Bounds().Width(), Bounds().Height()-apply->Bounds().Height() );
	AddChild( online );
}
