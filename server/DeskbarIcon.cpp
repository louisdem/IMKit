#include "DeskbarIcon.h"
#include "SettingsWindow.h"

#include <libim/Constants.h>
#include <libim/Manager.h>
#include <libim/Helpers.h>
#include <Message.h>
#include <stdio.h>
#include <Roster.h>
#include <File.h>
#include <MenuItem.h>
#include <Application.h>
#include <Window.h>
#include <Deskbar.h>

#include <Path.h>
#include <FindDirectory.h>

//#include "SettingsWindow.h"

BView *
instantiate_deskbar_item()
{
	LOG("deskbar", MEDIUM, "IM: Instantiating Deskbar item");
	return new IM_DeskbarIcon();
}

BArchivable *
IM_DeskbarIcon::Instantiate( BMessage * archive )
{
	if ( !validate_instantiation(archive,"IM_DeskbarIcon") )
	{
		LOG("deskbar", LOW, "IM_DeskbarIcon::Instantiate(): Invalid archive");
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
	delete fAwayIcon;
	delete fOnlineIcon;
	delete fOfflineIcon;
	delete fFlashIcon;
	delete fTip;
	delete fMenu;
}

void
IM_DeskbarIcon::_init()
{
	BPath iconDir;
	find_directory(B_USER_SETTINGS_DIRECTORY, &iconDir, true);
	iconDir.Append("im_kit/icons");
	
//	Load the Offline, Away, Online and Flash icons from disk
	BString iconPath = iconDir.Path();
	iconPath << "/DeskbarAway";
	fAwayIcon = GetBitmapFromAttribute(iconPath.String(), BEOS_SMALL_ICON_ATTRIBUTE,
		'ICON');

	iconPath = iconDir.Path();
	iconPath << "/DeskbarOnline";
	fOnlineIcon = GetBitmapFromAttribute(iconPath.String(), BEOS_SMALL_ICON_ATTRIBUTE,
		'ICON');
		
	iconPath = iconDir.Path();
	iconPath << "/DeskbarOffline";
	fOfflineIcon = GetBitmapFromAttribute(iconPath.String(), BEOS_SMALL_ICON_ATTRIBUTE,
		'ICON');

	iconPath = iconDir.Path();
	iconPath << "/DeskbarFlash";
	fFlashIcon = GetBitmapFromAttribute(iconPath.String(), BEOS_SMALL_ICON_ATTRIBUTE,
		'ICON');

//	Initial icon is the Offline icon
	fCurrIcon = fModeIcon = fOfflineIcon;
	fStatus = 2;

	fFlashCount = 0;
	fBlink = 0;
	fMsgRunner = NULL;
	
	fSettingsWindow = NULL;
	
	fShouldBlink = true;
	
	fTip = new BubbleHelper();
	
	fDirtyStatus = true;
	fDirtyMenu = true;
	fMenu = NULL;
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
//		SetDrawingMode(B_OP_COPY);
	} else
	{
		SetHighColor(255,0,0);
		FillRect( Bounds() );
	}
}

status_t
IM_DeskbarIcon::Archive( BMessage * msg, bool deep )
{
	status_t res = BView::Archive(msg,deep);
	
	msg->AddString("add_on", IM_SERVER_SIG );
	msg->AddString("add-on", IM_SERVER_SIG );
	
	msg->AddString("class", "IM_DeskbarIcon");
	
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
				fCurrIcon = fModeIcon;
			}
			
			if ( oldIcon != fCurrIcon )
				Invalidate();
		}	break;
		
		case IM::FLASH_DESKBAR:
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
			LOG("deskbar", HIGH, "IM: fFlashCount: %ld\n", fFlashCount);
		}	break;
		case IM::STOP_FLASHING:
		{	
			LOG("deskbar", HIGH, "Stopping teh flash\n");
			BMessenger msgr;
			if ( msg->FindMessenger("messenger", &msgr) == B_OK )
			{
				fMsgrs.remove( msgr );
			}
			
			fFlashCount--;
			LOG("deskbar", HIGH, "IM: fFlashCount: %ld\n", fFlashCount);
			
			if ( fFlashCount == 0 )
			{
				delete fMsgRunner;
				fMsgRunner = NULL;
				fCurrIcon = fModeIcon;
				Invalidate();
			}
			
			if ( fFlashCount < 0 )
			{
				fFlashCount = 0;
				LOG("deskbar", MEDIUM, "IM: fFlashCount below zero, fixing\n");
			}
		}	break;
		
		case SET_STATUS:
		{
			BMenuItem *item = NULL;
			msg->FindPointer("source", reinterpret_cast<void **>(&item));
			if (item == NULL) return;
		
			BMessage newmsg(IM::MESSAGE);
			newmsg.AddInt32("im_what", IM::SET_STATUS);

			const char *protocol = msg->FindString("protocol");
			if (protocol) {
				newmsg.AddString("protocol", strdup(protocol));
			};
			newmsg.AddString("status", item->Label());
			
			fCurrIcon = fModeIcon; 
			Invalidate();
			
			IM::Manager man;
			man.SendMessage(&newmsg);
		}	break;
		
		case CLOSE_IM_SERVER: {
			LOG("deskbar", LOW, "Got Quit message");
			BMessenger msgr(IM_SERVER_SIG);
			msgr.SendMessage(B_QUIT_REQUESTED);
		} break;
		
		case OPEN_SETTINGS:
		{
			if ( !fSettingsWindow )
			{ // create new settings window
				fSettingsWindow = new SettingsWindow( BMessenger(this) );
				fSettingsWindow->Show();
			} else
			{ // show existing settings window
				fSettingsWindow->SetWorkspaces( 1 << current_workspace() );
				fSettingsWindow->Activate();
			}
		}	break;
		
		case IM::SETTINGS:
		{
			msg->FindBool("blink_db", &fShouldBlink );
			
			LOG("deskbar", MEDIUM, "IM: Settings applied");
		}	break;
		
		case IM::MESSAGE: {
			int32 im_what;
			msg->FindInt32("im_what", &im_what);
			LOG("deskbar", LOW, "Got IM what of %i", im_what);
			
			switch (im_what) {
				case IM::STATUS_SET: {			
					fDirtyStatus = true;
					fDirtyMenu = true;
				
					const char *status = msg->FindString("total_status");
								
					LOG("deskbar", LOW, "Status set to %s", status);
					if (strcmp(status, ONLINE_TEXT) == 0) {
						fStatus = 0;
						fModeIcon = fOnlineIcon;
					}
					if (strcmp(status, AWAY_TEXT) == 0) {
						fStatus = 1;
						fModeIcon = fAwayIcon;
					};
					if (strcmp(status, OFFLINE_TEXT) == 0) {
						fStatus = 2;
						fModeIcon = fOfflineIcon;
					};
					
					fCurrIcon = fModeIcon;
					Invalidate();
				} break; 
			};
		} break;

		default:
			BView::MessageReceived(msg);
	}
}

void IM_DeskbarIcon::MouseMoved(BPoint point, uint32 transit, const BMessage *msg) {
	fTip->SetHelp(Parent(), NULL);

	if ((transit == B_OUTSIDE_VIEW) || (transit == B_EXITED_VIEW)) {
		fTip->SetHelp(Parent(), NULL);
	} else {
		if (fDirtyStatus == true) {
			BMessage protStatus;
			IM::Manager man;
			man.SendMessage(new BMessage(IM::GET_OWN_STATUSES), &protStatus);
	
			fTipText = "Online Status:";
			for ( int i=0; protStatus.FindString("protocol",i); i++ ) {
				const char *protocol = protStatus.FindString("protocol",i);
				const char *status = protStatus.FindString("status", i);
	
				fStatuses[protocol] = status;
	
				fTipText << "\n  " << protocol << ": " << status << "";
			}
			
			fDirtyStatus = false;
		};
	
		fTip->SetHelp(Parent(), (char *)fTipText.String());
		fTip->EnableHelp();
	};		
};


void
IM_DeskbarIcon::MouseDown( BPoint p )
{
	// make sure that the im_server is still running
	if ( !BMessenger(IM_SERVER_SIG).IsValid() )
	{
		BDeskbar db;
	
		db.RemoveItem( DESKBAR_ICON_NAME );
	}

	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	
	if ( buttons & B_SECONDARY_MOUSE_BUTTON )
	{
		if (fDirtyMenu) {
			delete fMenu;
			fMenu = new BPopUpMenu("im_db_menu");
			
			// set status
			BMenu * status = new BMenu("Set status");
			BMenu *total = new BMenu("All Protocols");
			BMessage msg(SET_STATUS);
			msg.AddString("protocol", "");
			total->AddItem(new BMenuItem(ONLINE_TEXT, new BMessage(msg)) );	
			total->AddItem(new BMenuItem(AWAY_TEXT, new BMessage(msg)) );	
			total->AddItem(new BMenuItem(OFFLINE_TEXT, new BMessage(msg)) );	
			total->ItemAt(fStatus)->SetMarked(true);
			total->SetTargetForItems(this);
			status->AddItem(total);
			status->AddSeparatorItem();
			
			map <string, string>::iterator it;
			
			for (it = fStatuses.begin(); it != fStatuses.end(); it++) {
				string name = (*it).first;
				BMenu *protocol = new BMenu((*it).first.c_str());
				BMessage protMsg(SET_STATUS);
				protMsg.AddString("protocol", name.c_str());			
				protocol->AddItem(new BMenuItem(ONLINE_TEXT, new BMessage(protMsg)));
				protocol->AddItem(new BMenuItem(AWAY_TEXT, new BMessage(protMsg)));
				protocol->AddItem(new BMenuItem(OFFLINE_TEXT, new BMessage(protMsg)));
				protocol->SetTargetForItems(this);
				if ((*it).second == ONLINE_TEXT) {
					protocol->ItemAt(0)->SetMarked(true);
				} else if ((*it).second == AWAY_TEXT) {
					protocol->ItemAt(1)->SetMarked(true);
				} else {
					protocol->ItemAt(2)->SetMarked(true);
				};
				
				status->AddItem(protocol);
			};
			
			status->SetTargetForItems( this );
				
			fMenu->AddItem(status);
			fMenu->AddSeparatorItem();
	
	//		settings
			fMenu->AddItem( new BMenuItem("Settings", new BMessage(OPEN_SETTINGS)) );
		
	//		Quit
			fMenu->AddSeparatorItem();
			fMenu->AddItem(new BMenuItem("Quit", new BMessage(CLOSE_IM_SERVER)));
	
			fMenu->SetTargetForItems( this );
			
			fDirtyMenu = false;
		};
		
		fMenu->Go(
			ConvertToScreen(p),
			true // delivers message
		);
		
	}
	
	if ( buttons & B_PRIMARY_MOUSE_BUTTON )
	{
		list<BMessenger>::iterator i = fMsgrs.begin();
		
		if ( i != fMsgrs.end() )
		{
			(*i).SendMessage( IM::DESKBAR_ICON_CLICKED );
		}
	}
	
	if (buttons & B_TERTIARY_MOUSE_BUTTON) {
		entry_ref ref;
		if (get_ref_for_path("/boot/home/people/", &ref) != B_OK) return;
		
		BMessage openPeople(B_REFS_RECEIVED);
		openPeople.AddRef("refs", &ref);
		
		BMessenger tracker("application/x-vnd.Be-TRAK");
		tracker.SendMessage(&openPeople);
	};

		
}

void
IM_DeskbarIcon::AttachedToWindow()
{
	// give im_server a chance to start up
	snooze(500*1000);	
	reloadSettings();
	
	// register with im_server
	LOG("deskbar", DEBUG, "Registering with im_server");
	BMessage msg(IM::REGISTER_DESKBAR_MESSENGER);
	msg.AddMessenger( "msgr", BMessenger(this) );
	
	BMessenger(IM_SERVER_SIG).SendMessage(&msg);
}

void
IM_DeskbarIcon::DetachedFromWindow()
{
	if ( fSettingsWindow )
	{
		fSettingsWindow->PostMessage( B_QUIT_REQUESTED );
	}
}

void
IM_DeskbarIcon::reloadSettings()
{
	LOG("deskbar", HIGH, "IM: Requesting settings");
	
	BMessage request( IM::GET_SETTINGS ), settings;
	request.AddString("protocol","");
	
	BMessenger msgr(IM_SERVER_SIG);
	
	msgr.SendMessage(&request, this );
	
	LOG("deskbar", HIGH, "IM: Settings requested");
}
