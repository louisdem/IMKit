#include "SettingsWindow.h"
#include <libim/Constants.h>
#include <libim/Helpers.h>

#include <Messenger.h>
#include <View.h>
#include <Menu.h>
#include <TextControl.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <stdlib.h>
#include <Button.h>
#include <stdio.h>
#include <string>
#include <CheckBox.h>

//// SETTINGS WINDOW

SettingsWindow::SettingsWindow( BMessenger deskbaricon )
:	BWindow(
		BRect(150,150,350,100), "IM Settings", 
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS|B_NOT_RESIZABLE|B_NOT_ZOOMABLE
	),
	fDeskbarIcon( deskbaricon ),
	fMan( new IM::Manager(BMessenger(this)) )
{
	BView * bg = new BView( Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW );
	bg->SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	
	AddChild(bg);

	BRect bounds = Bounds();
	bounds.InsetBy(3,3);
	
	fBox = new BBox( bounds, NULL, B_FOLLOW_ALL );
	
	bg->AddChild( fBox );
	
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
	// tell Deskbar icon we're closing
	fDeskbarIcon.SendMessage('swcl');
	
	return true;
}

void
SettingsWindow::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{	
		case SELECT_PROTOCOL:
		{
			if ( msg->FindString("protocol") == NULL || strlen(msg->FindString("protocol")) == 0 )
			{
				strcpy(fProtocol,"");
			} else
			{
				strcpy( fProtocol, msg->FindString("protocol") );
			}
			
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
							LOG("settings window", LOW, "Unhandled settings type!");
							return;
					}
				} else
				if ( dynamic_cast<BMenuField*>(FindView(name)) )
				{ // one-of-provided
					BMenuField * ctrl = (BMenuField*)FindView(name);
					
					BMenuItem * item = ctrl->Menu()->FindMarked();
					
					if ( !item )
					{
						LOG("settings window", MEDIUM, "Error: No selection in setting");
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
							LOG("settings window", LOW, "Unhandled settings type!");
							return;
					}
				} else
				if ( dynamic_cast<BCheckBox*>(FindView(name)) )
				{ // bool
					BCheckBox * box = (BCheckBox*)FindView(name);
					
					if ( box->Value() == B_CONTROL_ON )
						settings.AddBool(name,true);
					else
						settings.AddBool(name,false);
				}
			}
			
			LOG("settings window", DEBUG, "SETTINGS message", &settings);
			
			BMessage to_send(IM::SET_SETTINGS), reply;
			to_send.AddString("protocol", fProtocol);
			to_send.AddMessage("settings", &settings);
			
			LOG("settings window", DEBUG, "SET_SETTINGS message:",&to_send);
			
			fMan->SendMessage( &to_send, &reply );
			
			if ( reply.what == IM::ACTION_PERFORMED )
			{ // settings ok, notify deskbar icon
				fDeskbarIcon.SendMessage( 'upse' );
			}
			LOG("settings window", DEBUG, "apply settings, reply",&reply);
		}	break;
		
		default:
			BWindow::MessageReceived(msg);
	}
}

void
SettingsWindow::rebuildUI()
{
	LOG("settings window", MEDIUM, "Rebuilding GUI with protocol [%s]", fProtocol);
	
	// delete old UI
	LOG("settings window", DEBUG, "  deleting old views");
	fBox->SetLabel( (BView*)NULL );
	while ( fBox->CountChildren() > 0 )
	{
		BView * view = fBox->ChildAt(0);
		
		if ( view != fBox->LabelView() )
		{
			fBox->RemoveChild(view);
			delete view;
		}
	}
	
	// get list of available protocols
	BMessage req_protocols(IM::GET_LOADED_PROTOCOLS), protocols;
	fMan->SendMessage( &req_protocols, &protocols );
	
	BMenu * protocolMenu = NULL;
	
	if ( protocols.what == IM::ACTION_PERFORMED )
	{ // got list of protocols. yay!
		LOG("settings window", DEBUG, "  Creating protocol menu");
		
		protocolMenu = new BMenu("Select module");
		
		for ( int i=0; protocols.FindString("protocol",i); i++ )
		{
			const char * p = protocols.FindString("protocol",i);
			
			BMessage * msg = new BMessage(SELECT_PROTOCOL);
			msg->AddString("protocol",p);
			
			string item_text( string(p) + string(" settings") );
			
			protocolMenu->AddItem( new BMenuItem(item_text.c_str(),msg) );
		}
		
		protocolMenu->AddSeparatorItem();
		
		// done with protocols, add im_server too
		BMessage * msg = new BMessage(SELECT_PROTOCOL);
		// don't add protocol name => im_server settings
		protocolMenu->AddItem( new BMenuItem("IM Server", msg) );
	} else
	{
		LOG("settings window", LOW, "Error: Failed to get list of protocols");
		return;
	}
	
	BMenuField * protoSelect = new BMenuField(
		BRect(0,0,200,20), "", NULL, protocolMenu
	);
	
//	AddChild( protoSelect );
	
	fBox->SetLabel( protoSelect );
	
	// get template from im_server
	BMessage req_template(IM::GET_SETTINGS_TEMPLATE);
	req_template.AddString("protocol", fProtocol);
	
	BMessage req_settings(IM::GET_SETTINGS);
	req_settings.AddString("protocol", fProtocol);
	
	BMessage templ, settings;
	
	LOG("settings window", DEBUG, "  Getting template and settings from im_server");
	fMan->SendMessage( &req_template, &fTemplate );
	fMan->SendMessage( &req_settings, &settings );
	
	if ( fTemplate.what == IM::ERROR )
	{ // got template, construct GUI
		LOG("settings window", LOW, "SettingsWindow construction failed: Couldn't get template");
		return;
	}
	
/*	LOG("Template", &fTemplate);
		
	LOG("Settings", &settings);
*/		
	BMessage curr;
	
	LOG("settings window", DEBUG, "  Creating views");
	for ( int i=0; fTemplate.FindMessage("setting",i,&curr) == B_OK; i++ )
	{
		char temp[512];
			
		// get text etc from template
		const char * name = curr.FindString("name");
		const char * desc = curr.FindString("description");
		const char * value = NULL;
		int32 type=-1;
		curr.FindInt32("type",&type);
		
		LOG("settings window", DEBUG, "Setting %s [%s]", desc, name);
		
		bool is_free_text = true;
		bool is_secret = false;
		BMenu * menu = NULL;
		BView * control = NULL;
		
		switch ( type )
		{ // get value from settings if available
			case B_STRING_TYPE:
			{
				LOG("settings window", DEBUG, "  string setting");
				if ( curr.FindString("valid_value") )
				{ // one-of-provided
					LOG("settings window", DEBUG, "  one-of-provided");
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
					LOG("settings window", DEBUG, "  free-text");
					value = settings.FindString(name);
					if ( !value )
						value = curr.FindString("default");
					if ( curr.FindBool("is_secret",&is_secret) != B_OK )
						is_secret = false;
				}
			}	break;
			case B_INT32_TYPE:
			{
				LOG("settings window", DEBUG, "  int32 setting");
				if ( curr.FindInt32("valid_value") )
				{ // one-of-provided
					LOG("settings window", DEBUG, "  one-of-provided");
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
					LOG("settings window", DEBUG, "  free-text");
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
			case B_BOOL_TYPE:
			{
				LOG("settings window", DEBUG, "  bool setting");
				
				bool active;
				
				if ( settings.FindBool(name, &active) != B_OK )
					curr.FindBool("default", &active);
				
				control = new BCheckBox( BRect(0,0,200,20), name, desc, NULL );
				
				if ( active )
					((BCheckBox*)control)->SetValue(B_CONTROL_ON);
			}	break;
		}
		
		if ( !value )
			value = "";
		
		LOG("settings window", DEBUG, "  creating control");
		// create control if needed
		if ( !control )
		{
			if ( is_free_text )
			{ // free-text setting
				control = new BTextControl( BRect(0,0,200,20), name, desc, value, NULL	);
			} else
			{ // select-one-of-provided setting
				control = new BMenuField( BRect(0,0,200,20), name, desc, menu );
			}
			if (is_secret)
			{
				((BTextControl *)control)->TextView()->HideTyping(true);
				((BTextControl *)control)->SetText(value); // err.. Why does HideTyping remove the text?
			}
		}
		
		fBox->AddChild( control );
		control->MoveTo(10, 25+i*21);
		ResizeTo( 220, 45+(i+1)*21 );
		
		LOG("settings window", DEBUG, "  done.");
	}
	
	// add space for buttons
	ResizeTo( Bounds().Width(), Bounds().Height()+30 );
	
	// apply settings
	BButton * apply = new BButton(
		BRect(0,0,40,25), "", "Apply", new BMessage(APPLY_SETTINGS)
	);
	apply->MoveTo( 10, Bounds().Height()-apply->Bounds().Height()-10 );
	fBox->AddChild( apply );
	
	// "set status" menu
	if ( strlen(fProtocol) > 0 )
	{
		BMenu * statusMenu = new BPopUpMenu("");
		BMessage * onlineMsg = new BMessage(ALTER_STATUS);	onlineMsg->AddString("status","available");
		BMessage * awayMsg = new BMessage(ALTER_STATUS);	awayMsg->AddString("status","away");
		BMessage * offlineMsg = new BMessage(ALTER_STATUS);	offlineMsg->AddString("status","offline");
	
		statusMenu->AddItem( new BMenuItem("online", onlineMsg) );
		statusMenu->AddItem( new BMenuItem("away", awayMsg) );
		statusMenu->AddItem( new BMenuItem("offline", offlineMsg) );
	
		BMenuField * online = new BMenuField( BRect(0,0,100,25), "_new_status", "Set status:", statusMenu );
		online->MoveTo( Bounds().Width()-online->Bounds().Width()+10, Bounds().Height()-apply->Bounds().Height()-10 );
		fBox->AddChild( online );
	}
	
	// update window title to reflect selected page
	if ( strlen(fProtocol) > 0 )
	{
		char title[512];
		sprintf(title,"IM: %s settings", fProtocol);
		SetTitle( title );
	} else
	{
		SetTitle("IM: IM Server settings");
	}
}
