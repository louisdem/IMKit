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
#include <String.h>

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
	add_system_beep_event(kImNewMessageSound, 0);

	fMan->StartListening();
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
MyApp::RefsReceived( BMessage * msg )
{
	entry_ref ref;
	for ( int i=0; msg->FindRef("refs", i, &ref ) == B_OK; i++ )
		msg->AddRef("contact", &ref);
	
	msg->what = IM::MESSAGE;
	msg->AddInt32("im_what", IM::MESSAGE_RECEIVED);
	
	PostMessage(msg);
}

void
MyApp::MessageReceived( BMessage * msg )
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
			
			if ( !win && (im_what == IM::MESSAGE_RECEIVED) )
			{ // open new window on message received or user request
				LOG("Creating new window to handle message");
				win = new ChatWindow(ref);
				if ( win->Lock() )
				{
					win->Show();
					win->PostMessage(msg);
					win->Unlock();
				} else
				{
					LOG("This is a fatal error that should never occur. Lock fail on new win.");
				}
				
			} else
			{
				if ( win->Lock() )
				{
					win->PostMessage(msg);
					if ( win->IsMinimized() )
					{ // window is hidden, move to this workspace and show it
						win->SetWorkspaces( B_CURRENT_WORKSPACE );
						win->Minimize(false);
					}
					win->Unlock();
				} else
				{
					LOG("This is a fatal error that should never occur. Lock fail on old win.");
				}
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

void
MyApp::Flash( BMessenger msgr )
{
	fMan->FlashDeskbar(msgr);
}

void
MyApp::NoFlash( BMessenger msgr )
{
	fMan->StopFlashingDeskbar(msgr);
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
	
	Minimize(true);
	
	return false;
}

void
ChatWindow::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case IM::DESKBAR_ICON_CLICKED:
		{ // deskbar icon clicked, move to current workspace and activate
			SetWorkspaces( 1 << current_workspace() );
			Activate();
		}	break;
		
		case IM::MESSAGE:
		{
			entry_ref contact;
			
			if ( msg->FindRef("contact",&contact) != B_OK )
				return;
				
			if ( contact != fEntry )
				return;
			
			if ( msg->FindString("message") == NULL )
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
			rgb_color time_text_color		= (rgb_color){130,130,130};
			
			int32 old_sel_start, old_sel_end;
			
			fText->GetSelection(&old_sel_start, &old_sel_end);
			fText->Select(fText->TextLength(),fText->TextLength());
			
			switch ( im_what )
			{
				case IM::MESSAGE_SENT:
				{
					fText->SetFontAndColor( be_plain_font, B_FONT_ALL, &time_text_color);
					strftime(timestr,sizeof(timestr),"[%H:%M] ", localtime(&now) );
					fText->Insert(timestr);
					fText->SetFontAndColor( be_plain_font, B_FONT_ALL, &own_nick_color);
					BString message;
					msg->FindString("message", &message);
					if (message.Compare("/me ", 4) == 0) {
						fText->Insert("* ");
						message.Remove(0, 4);
						fText->Insert(message.String());
					} else {
						fText->Insert("You say: ");
						fText->SetFontAndColor( be_plain_font, B_FONT_ALL, &own_text_color);
						fText->Insert(msg->FindString("message"));
					}
					fText->Insert("\n");
					fText->ScrollToSelection();
				}	break;
				
				case IM::MESSAGE_RECEIVED:
				{
					fText->SetFontAndColor( be_plain_font, B_FONT_ALL, &time_text_color);
					strftime(timestr,sizeof(timestr),"[%H:%M] ",  localtime(&now) );
					fText->Insert(timestr);
					fText->SetFontAndColor( be_plain_font, B_FONT_ALL, &contact_nick_color);
					BString message;
					msg->FindString("message", &message);
					if (message.Compare("/me ", 4) == 0) 
					{
						fText->Insert("* ");
						fText->Insert(fName);
						fText->Insert(" ");
						message.Remove(0, 4);
						fText->Insert(message.String());
					} else 
					{
						fText->Insert(fName);
						fText->Insert(": ");
						fText->SetFontAndColor( be_plain_font, B_FONT_ALL, &contact_text_color);
						fText->Insert(msg->FindString("message"));
					}
					fText->Insert("\n");
					fText->ScrollToSelection();

					if (!IsActive()) 
					{
						startNotify();
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
	if (active) 
		stopNotify();
	
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
	
	if ( !fChangedNotActivated )
	{
		SetTitle(fTitleCache);
	} else
	{
		char str[512];
		sprintf(str, "√ %s", fTitleCache);
		SetTitle(str);
	}
}

void
ChatWindow::startNotify()
{
	if ( fChangedNotActivated )
		return;
	
	fChangedNotActivated = true;
	char str[512];
	sprintf(str, "√ %s", fTitleCache);
	SetTitle(str);
	
	((MyApp*)be_app)->Flash( BMessenger(this) );
	
	if ( (Workspaces() & (1 << current_workspace())) == 0)
	{
		// beep if on other workspace
		system_beep(kImNewMessageSound);
	}
}

void
ChatWindow::stopNotify()
{
	if ( !fChangedNotActivated )
		return;
	
	fChangedNotActivated = false;
	SetTitle(fTitleCache);
	((MyApp*)be_app)->NoFlash( BMessenger(this) );
}
