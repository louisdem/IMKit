#include "DeskbarIcon.h"
#include "SettingsWindow.h"

#include <libim/Constants.h>
#include <libim/Manager.h>
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
	return new IM_DeskbarIcon();
}

BArchivable *
IM_DeskbarIcon::Instantiate( BMessage * archive )
{
	if ( !validate_instantiation(archive,"IM_DeskbarIcon") )
	{
		printf("IM_DeskbarIcon::Instantiate(): Invalid archive\n");
		return NULL;
	}
	
	return new IM_DeskbarIcon(archive);
}


IM_DeskbarIcon::IM_DeskbarIcon()
:	BView( 
		BRect(0,0,15,15), 
		DESKBAR_ICON_NAME, 
		B_FOLLOW_NONE, 
		B_WILL_DRAW | B_PULSE_NEEDED 
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
	printf("IM: _init\n");
	
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
}

void
IM_DeskbarIcon::Draw( BRect rect )
{
	SetHighColor( Parent()->ViewColor() );
	FillRect( Bounds() );
	
	SetDrawingMode(B_OP_OVER);
	
	DrawBitmap( fCurrIcon, BPoint(0,0) );
	
	SetDrawingMode(B_OP_COPY);
}

void
IM_DeskbarIcon::Pulse()
{
	BBitmap * oldIcon = fCurrIcon;
	
	if ( (fFlashCount > 0) && (fBlink++ % 2) )
	{
		fCurrIcon = fFlashIcon;
	} else
	{
		fCurrIcon = fStdIcon;
	}
	
	if ( oldIcon != fCurrIcon )
		Invalidate();
}

status_t
IM_DeskbarIcon::Archive( BMessage * msg, bool deep )
{
	BView::Archive(msg,deep);
	
	msg->AddString("add_on", IM_SERVER_SIG );
	
	return B_NO_ERROR;
}

void
IM_DeskbarIcon::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case 'flsh':
			fFlashCount++;
			fBlink = 0;
			Pulse();
			break;
		case 'stop':
			fFlashCount--;
			Pulse();
			break;
		
		case SET_ONLINE:
		case SET_AWAY:
		case SET_OFFLINE:
		{
			printf("IM: set status\n");
			BMessage newmsg(IM::MESSAGE);
			newmsg.AddInt32("im_what", IM::SET_STATUS);
			
			switch ( msg->what )
			{
				case SET_ONLINE:  newmsg.AddString("status","online"); break;
				case SET_AWAY:    newmsg.AddString("status","away"); break;
				case SET_OFFLINE: newmsg.AddString("status","offline"); break;
			}
			
			IM::Manager man;
			man.SendMessage(&newmsg);
		}	break;
		
		case OPEN_SETTINGS:
		{
			printf("IM: open settings\n");
			bool settings_found = false;
			
			for ( int i=0; be_app->WindowAt(i); i++ )
				if ( strcmp(be_app->WindowAt(i)->Title(), "IM Settings") == 0 )
					settings_found = true;
			
			if ( !settings_found )
			{
				SettingsWindow * win = new SettingsWindow;
				win->Show();
			}
		}	break;
		
		default:
			BView::MessageReceived(msg);
	}
}

void
IM_DeskbarIcon::MouseDown( BPoint p )
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
