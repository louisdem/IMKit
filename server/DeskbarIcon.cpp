#include "DeskbarIcon.h"
#include "SettingsWindow.h"

#include <libim/Constants.h>
#include <libim/Manager.h>
#include <libim/Helpers.h>
#include <Message.h>
#include <stdio.h>
#include <Roster.h>
#include <File.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Application.h>
#include <Window.h>

//#include "SettingsWindow.h"

BView *
instantiate_deskbar_item()
{
	LOG("IM: Instantiating Deskbar item");
	return new IM_DeskbarIcon();
}

BArchivable *
IM_DeskbarIcon::Instantiate( BMessage * archive )
{
	if ( !validate_instantiation(archive,"IM_DeskbarIcon") )
	{
		LOG("IM_DeskbarIcon::Instantiate(): Invalid archive");
		return NULL;
	}
	
	return new IM_DeskbarIcon(archive);
}


IM_DeskbarIcon::IM_DeskbarIcon()
:	BView( 
		BRect(0,0,15,15), 
		DESKBAR_ICON_NAME, 
		B_FOLLOW_NONE, 
		B_WILL_DRAW 
	)
{
	_init();
}

IM_DeskbarIcon::IM_DeskbarIcon( BMessage * archive )
:	BView( archive )
{
	_init();
}

IM_DeskbarIcon::~IM_DeskbarIcon()
{
}

void
IM_DeskbarIcon::_init()
{
//	printf("IM: _init\n");
	
	// load resources
	entry_ref ref;
	BFile file;
	
	if ( be_roster->FindApp(IM_SERVER_SIG,&ref) == B_OK )
	{
		file.SetTo( &ref, B_READ_ONLY );
		
		fResource.SetTo(&file);
	}
	// ~load resources
	
	fStdIcon = GetBitmap("IM db icon");
	fFlashIcon = GetBitmap("IM db icon flash");
	
	fCurrIcon = fStdIcon;
	
	fFlashCount = 0;
	fBlink = 0;
	fMsgRunner = NULL;
	
	fSettingsWindow = NULL;
	
	fShouldBlink = true;
}

void
IM_DeskbarIcon::Draw( BRect rect )
{
	SetHighColor( Parent()->ViewColor() );
	FillRect( Bounds() );
	
	if ( fCurrIcon )
	{
		SetDrawingMode(B_OP_OVER);
		DrawBitmap( fCurrIcon, BPoint(0,0) );
		SetDrawingMode(B_OP_COPY);
	} else
	{
		SetHighColor(255,0,0);
		FillRect( Bounds() );
	}
}

status_t
IM_DeskbarIcon::Archive( BMessage * msg, bool deep )
{
//	printf("IM_DeskbarIcon::Archive()\n");
	status_t res = BView::Archive(msg,deep);
	
	msg->AddString("add_on", IM_SERVER_SIG );
	msg->AddString("add-on", IM_SERVER_SIG );
	
	msg->AddString("class", "IM_DeskbarIcon");
	
//	printf("~IM_DeskbarIcon::Archive() returns %ld\n", res);
	
	return res;
}

void
IM_DeskbarIcon::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case SETTINGS_WINDOW_CLOSED:
		{
			fSettingsWindow = NULL;
		}	break;
		
		case RELOAD_SETTINGS:
		{ // settings have been updated, reload from im_server
			reloadSettings();
		}	break;
		
		case 'blnk':
		{ // blink icon
			BBitmap * oldIcon = fCurrIcon;
			
			if ( (fFlashCount > 0) && ((fBlink++ % 2) || !fShouldBlink))
			{
				fCurrIcon = fFlashIcon;
			} else
			{
				fCurrIcon = fStdIcon;
			}
			
			if ( oldIcon != fCurrIcon )
				Invalidate();
		}	break;
		
		case 'flsh':
		{
			BMessenger msgr;
			if ( msg->FindMessenger("messenger", &msgr) == B_OK )
			{
				fMsgrs.push_back( msgr );
			}
			
			fFlashCount++;
			fBlink = 0;
			if ( !fMsgRunner )
			{
				BMessage msg('blnk');
				fMsgRunner = new BMessageRunner( BMessenger(this), &msg, 200*1000 );
			}
			LOG("IM: fFlashCount: %ld\n", fFlashCount);
		}	break;
		case 'stop':
		{	
			BMessenger msgr;
			if ( msg->FindMessenger("messenger", &msgr) == B_OK )
			{
				fMsgrs.remove( msgr );
			}
			
			fFlashCount--;
			LOG("IM: fFlashCount: %ld\n", fFlashCount);
			
			if ( fFlashCount == 0 )
			{
				delete fMsgRunner;
				fMsgRunner = NULL;
				fCurrIcon = fStdIcon;
				Invalidate();
			}
			
			if ( fFlashCount < 0 )
			{
				fFlashCount = 0;
				LOG("IM: fFlashCount below zero, fixing\n");
			}
		}	break;
		
		case SET_ONLINE:
		case SET_AWAY:
		case SET_OFFLINE:
		{
			BMessage newmsg(IM::MESSAGE);
			newmsg.AddInt32("im_what", IM::SET_STATUS);
			
			switch ( msg->what )
			{
				case SET_ONLINE:  newmsg.AddString("status",ONLINE_TEXT); break;
				case SET_AWAY:    newmsg.AddString("status",AWAY_TEXT); break;
				case SET_OFFLINE: newmsg.AddString("status",OFFLINE_TEXT); break;
			}
			
			IM::Manager man;
			man.SendMessage(&newmsg);
		}	break;
		
		case OPEN_SETTINGS:
		{
			if ( !fSettingsWindow )
			{ // create new settings window
				fSettingsWindow = new SettingsWindow( BMessenger(this) );
				fSettingsWindow->Show();
			} else
			{ // show existing settings window
				fSettingsWindow->SetWorkspaces( 1 << current_workspace() );
				fSettingsWindow->Show();
			}
		}	break;
		
		case IM::SETTINGS:
		{
			msg->FindBool("blink_db", &fShouldBlink );
			
			LOG("IM: Settings applied");
		}	break;
		
		default:
			BView::MessageReceived(msg);
	}
}

void
IM_DeskbarIcon::MouseDown( BPoint p )
{
	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	
	if ( buttons & B_SECONDARY_MOUSE_BUTTON )
	{
		BPopUpMenu * menu = new BPopUpMenu("im_db_menu");
		
		// set status
		BMenu * status = new BMenu("Set status");
		status->AddItem( new BMenuItem("Online", new BMessage(SET_ONLINE)) );	
		status->AddItem( new BMenuItem("Away", new BMessage(SET_AWAY)) );	
		status->AddItem( new BMenuItem("Offline", new BMessage(SET_OFFLINE)) );	
		status->SetTargetForItems( this );
		
		menu->AddItem(status);
		
		menu->AddSeparatorItem();
		
		// settings
		menu->AddItem( new BMenuItem("Settings", new BMessage(OPEN_SETTINGS)) );
		
		menu->SetTargetForItems( this );
		
		menu->Go(
			ConvertToScreen(p),
			true // delivers message
		);
		
		delete menu;
	}
	
	if ( buttons & B_PRIMARY_MOUSE_BUTTON )
	{
		list<BMessenger>::iterator i = fMsgrs.begin();
		
		if ( i != fMsgrs.end() )
		{
			(*i).SendMessage( IM::DESKBAR_ICON_CLICKED );
		}
	}
}

void
IM_DeskbarIcon::AttachedToWindow()
{
	// give im_server a chance to start up
	snooze(500*1000);
	
	reloadSettings();
}

void
IM_DeskbarIcon::DetachedFromWindow()
{
	if ( fSettingsWindow )
	{
		fSettingsWindow->PostMessage( B_QUIT_REQUESTED );
	}
}


// Some code borrowed from CKJ <cedric-vincent@wanadoo.fr>
// originally in USB Deskbar View <http://www.bebits.com/app/3497>
BBitmap *
IM_DeskbarIcon::GetBitmap( const char * name )
{
	BBitmap 	*bitmap = NULL;
	size_t 		len = 0;
	status_t 	error;	

	// load resource
	const void *data = fResource.LoadResource('BBMP', name, &len);
	
	BMemoryIO stream(data, len);
	
	// unflatten it
	BMessage archive;
	error = archive.Unflatten(&stream);
	if (error != B_OK)
		return NULL;

	// make a bbitmap from it
	bitmap = new BBitmap(&archive);
	if(!bitmap)
		return NULL;

	// make sure it's ok
	if(bitmap->InitCheck() != B_OK)
	{
		delete bitmap;
		return NULL;
	}
	
	// done!
	return bitmap;
}

void
IM_DeskbarIcon::reloadSettings()
{
	LOG("IM: Requesting settings");
	
	BMessage request( IM::GET_SETTINGS ), settings;
	request.AddString("protocol","");
	
	BMessenger msgr(IM_SERVER_SIG);
	
	msgr.SendMessage(&request, this );
	
	LOG("IM: Settings requested");
}
