#include "DeskbarIcon.h"

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

BView *
instantiate_deskbar_item()
{
	LOG("deskbar", liMedium, "IM: Instantiating Deskbar item");
	return new IM_DeskbarIcon();
}

BArchivable *
IM_DeskbarIcon::Instantiate( BMessage * archive )
{
	if ( !validate_instantiation(archive,"IM_DeskbarIcon") )
	{
		LOG("deskbar", liHigh, "IM_DeskbarIcon::Instantiate(): Invalid archive");
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
	delete fTip;
	delete fAwayIcon;
	delete fOnlineIcon;
	delete fOfflineIcon;
	delete fFlashIcon;
	delete fMenu;
}

void
IM_DeskbarIcon::_init()
{
	BPath userDir;
	find_directory(B_USER_SETTINGS_DIRECTORY, &userDir, true);

	BPath iconDir = userDir;
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
	
	fShouldBlink = true;
	
	fTip = new BubbleHelper();
	
	fDirtyStatus = true;
	fDirtyMenu = true;
	fMenu = NULL;
	
	SetDrawingMode(B_OP_OVER);
	
	BMessage protStatus;
	fStatuses.clear();
	IM::Manager man;
	man.SendMessage(new BMessage(IM::GET_OWN_STATUSES), &protStatus);

	fTipText = "Online Status:";
	
	for ( int i=0; protStatus.FindString("protocol",i); i++ ) {
		const char *protocol = protStatus.FindString("protocol",i);
		const char *status = protStatus.FindString("status", i);

		fStatuses[protocol] = status;

		fTipText << "\n  " << protocol << ": " << status << "";
		
		if ((fStatus > 0) && (strcmp(status, ONLINE_TEXT) == 0)) fStatus = 0;
		if ((fStatus > 1) && (strcmp(status, AWAY_TEXT) == 0)) fStatus = 1;
	}

	LOG("deskbar", liDebug, "Initial status: %i	", fStatus);
	
	switch (fStatus) {
//		Online
		case 0: {
			fCurrIcon = fModeIcon = fOnlineIcon;
		} break;
//		Away
		case 1: {
			fCurrIcon = fModeIcon = fAwayIcon;
		} break;
		default: {
			fCurrIcon = fModeIcon =  fOfflineIcon;
		};
	};
}

void
IM_DeskbarIcon::Draw( BRect rect )
{
	SetHighColor( Parent()->ViewColor() );
	FillRect( Bounds() );
	
	if ( fCurrIcon )
	{
		DrawBitmap( fCurrIcon, BPoint(0,0) );
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
		case IM::SETTINGS_UPDATED:
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
			LOG("deskbar", liDebug, "IM: fFlashCount: %ld\n", fFlashCount);
		}	break;
		case IM::STOP_FLASHING:
		{	
			LOG("deskbar", liLow, "Stopping teh flash\n");
			BMessenger msgr;
			if ( msg->FindMessenger("messenger", &msgr) == B_OK )
			{
				fMsgrs.remove( msgr );
			}
			
			fFlashCount--;
			LOG("deskbar", liDebug, "IM: fFlashCount: %ld\n", fFlashCount);
			
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
				LOG("deskbar", liMedium, "IM: fFlashCount below zero, fixing\n");
			}
		}	break;
		
		case SET_STATUS:
		{
			BMenuItem *item = NULL;
			msg->FindPointer("source", reinterpret_cast<void **>(&item));
			if (item == NULL) 
				return;
			
			const char *protocol = msg->FindString("protocol");
			
			if (strcmp("Away", item->Label()) == 0) {
				AwayMessageWindow *w = new AwayMessageWindow(protocol);
				w->Show();
				return;
			}
			
			BMessage newmsg(IM::MESSAGE);
			newmsg.AddInt32("im_what", IM::SET_STATUS);
			
			if ( protocol != NULL ) newmsg.AddString("protocol", protocol);

			newmsg.AddString("status", item->Label());
			
			fCurrIcon = fModeIcon; 
			Invalidate();
			
			IM::Manager man;
			man.SendMessage(&newmsg);
		}	break;
		
		case CLOSE_IM_SERVER: {
			LOG("deskbar", liHigh, "Got Quit message");
			BMessenger msgr(IM_SERVER_SIG);
			msgr.SendMessage(B_QUIT_REQUESTED);
		} break;
		
		case OPEN_SETTINGS:
		{
			be_roster->Launch("application/x-vnd.beclan-IMKitPrefs");
		}	break;
		
		case IM::MESSAGE: {
			int32 im_what;
			msg->FindInt32("im_what", &im_what);
			LOG("deskbar", liLow, "Got IM what of %i", im_what);
			
			switch (im_what) {
				case IM::STATUS_SET: {			
					fDirtyStatus = true;
					fDirtyMenu = true;
				
					const char *status = msg->FindString("total_status");
								
					LOG("deskbar", liMedium, "Status set to %s", status);
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

		case B_NODE_MONITOR: {
			int32 opcode;
			if (msg->FindInt32("opcode", &opcode) == B_OK) {
				switch (opcode) {
					case B_ENTRY_CREATED: {
						BVolumeRoster volRoster;
						BVolume bootVol;
						BMessenger target(this, NULL, NULL);
						entry_ref ref;
						const char *name;
						volRoster.GetBootVolume(&bootVol);
						
//						XXX For some reason the ref name won't set correctly :/
//						So, new queries will show up in the menu, but not work
						msg->FindInt32("device", &ref.device);
						msg->FindInt64("directory", &ref.directory);
						msg->FindString("name", &name);
						printf("Set name: %s\n", strerror(ref.set_name(name)));

printf("Ref from: %i/%i/%s (%s)\n", ref.device, ref.directory, ref.name, name);
						
						BNode node(&ref);
						BQuery *query = new BQuery();
						
						int32 length = 0;
						char *queryFormula = ReadAttribute(node, "_trk/qrystr", &length);
						queryFormula = (char *)realloc(queryFormula, sizeof(char) * (length + 1));
						queryFormula[length] = '\0';

						query->SetPredicate(queryFormula);
						query->SetVolume(&bootVol);
						query->SetTarget(target);
								
						fQueries[ref] = query;
						free(queryFormula);
				
						
						printf("Fetch node: %s\n", strerror(query->Fetch()));
					} break;
					
//					We currently only handle the case of the user *adding* a new query

//					case B_ENTRY_RENAMED: {
//					} break;			
//					case B_ENTRY_MOVED:
//					case B_ENTRY_REMOVED: {
//					} break;
				};

			};
		} break;

		case B_QUERY_UPDATE: {
//			msg->PrintToStream();
			fDirtyMenu = true;
		} break;
		
		case LAUNCH_FILE: {
			entry_ref fileRef;
			msg->FindRef("fileRef", &fileRef);
			
			const char *handler = "application/x-vnd.Be-TRAK";
			int32 length = 0;
			char *mime = ReadAttribute(BNode(&fileRef), "BEOS:TYPE", &length);
			mime = (char *)realloc(mime, (length + 1) * sizeof(char));
			mime[length] = '\0';
			
			if (strcmp(mime, "application/x-person") == 0) {
				handler = fPeopleApp;
			};
			
			BMessage openMsg(B_REFS_RECEIVED);
			openMsg.AddRef("refs", &fileRef);
			
			be_roster->Launch(handler, &openMsg);
			
		} break;
		
		case OPEN_QUERY: {
//			For great justice, take off every query!
			entry_ref queryRef;
			msg->FindRef("queryRef", &queryRef);

			BMessage open_msg(B_REFS_RECEIVED);
			open_msg.AddRef("refs", &queryRef);
			
			be_roster->Launch("application/x-vnd.Be-TRAK", &open_msg);
		} break;

		default:
			BView::MessageReceived(msg);
	}
}

void IM_DeskbarIcon::MouseMoved(BPoint point, uint32 transit, const BMessage *msg) {
	// make sure that the im_server is still running
	if ( ( transit == B_ENTERED_VIEW ) && !BMessenger(IM_SERVER_SIG).IsValid() )
	{
		BDeskbar db;
		
		db.RemoveItem( DESKBAR_ICON_NAME );
	}

	fTip->SetHelp(Parent(), NULL);

	if ((transit == B_OUTSIDE_VIEW) || (transit == B_EXITED_VIEW)) {
		fTip->SetHelp(Parent(), NULL);
	} else {
		IM::Manager man;

		if ((fDirtyStatus == true) || (fStatuses.size() == 0)) {
			fStatuses.clear();
			
			BMessage protStatus;
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
	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	
	if ( buttons & B_SECONDARY_MOUSE_BUTTON )
	{
		if ((fDirtyMenu) || (fMenu->CountItems() == 2)) {
			delete fMenu;
			fMenu = new BPopUpMenu("im_db_menu", false, false);
			fMenu->SetFont(be_plain_font);
			
			// set status
			BMenu * status = new BMenu("Set status");
			BMenu *total = new BMenu("All Protocols");
			BMessage msg(SET_STATUS);
			total->AddItem(new BMenuItem(ONLINE_TEXT, new BMessage(msg)) );	
			total->AddItem(new BMenuItem(AWAY_TEXT, new BMessage(msg)) );	
			total->AddItem(new BMenuItem(OFFLINE_TEXT, new BMessage(msg)) );	
			total->ItemAt(fStatus)->SetMarked(true);
			total->SetTargetForItems(this);
			status->AddItem(total);
			status->AddSeparatorItem();
			
			status->SetFont(be_plain_font);
			total->SetFont(be_plain_font);
			
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
				
				protocol->SetFont(be_plain_font);
				
				status->AddItem(protocol);
			};
			
			status->SetTargetForItems( this );
	
			fMenu->AddItem(status);

			if (fQueries.size() > 0 ) {
				BMenu *queries = new BMenu("Queries");
				querymap::iterator qIt;
				
				BVolumeRoster volRoster;
				BVolume bootVol;		
				BMessenger target(this, NULL, NULL);
				
				volRoster.GetBootVolume(&bootVol);
				
				for (qIt = fQueries.begin(); qIt != fQueries.end(); qIt++) {
					BMenu *menu = new BMenu(qIt->first.name);
					BQuery *query = qIt->second;
					entry_ref ref;
					
					BBitmap *icon = ReadNodeIcon(BPath(&qIt->first).Path());
					BMessage *queryMsg = new BMessage(OPEN_QUERY);
					queryMsg->AddRef("queryRef", &qIt->first);

					IconMenuItem *item = new IconMenuItem(icon, "Open In Tracker",
						"Empty", queryMsg);

					menu->AddItem(item);
					menu->AddSeparatorItem();
					
					while (query->GetNextRef(&ref) == B_OK) {
						BMessage *itemMsg = new BMessage(LAUNCH_FILE);
						itemMsg->AddRef("fileRef", &ref);
						
						icon = ReadNodeIcon(BPath(&ref).Path());
						item = new IconMenuItem(icon , ref.name, "", itemMsg);

						menu->AddItem(item);						
					};
					
//					Recreate query
					int32 predLength = query->PredicateLength();
					char *predicate = (char *)calloc(predLength + 1, sizeof(char));
					query->GetPredicate(predicate, predLength);
					predicate[predLength] = 0;

					query->Clear();
					query->SetVolume(&bootVol);
					query->SetTarget(target);
					query->SetPredicate(predicate);

					free(predicate);
					query->Fetch();
					
					menu->SetFont(be_plain_font);
					menu->SetTargetForItems(this);
					
					queries->AddItem(menu);
				};
				
				queries->SetFont(be_plain_font);
				fMenu->AddSeparatorItem();
				fMenu->AddItem(queries);
				
			};
				
//			settings
			fMenu->AddSeparatorItem();
			fMenu->AddItem( new BMenuItem("Settings", new BMessage(OPEN_SETTINGS)) );
		
//			Quit
			fMenu->AddSeparatorItem();
			fMenu->AddItem(new BMenuItem("Quit", new BMessage(CLOSE_IM_SERVER)));
	
			fMenu->SetTargetForItems( this );
			
//			fDirtyMenu = false;
		};
		
		ConvertToScreen(&p);
		BRect r(p, p);
		r.InsetBySelf(-2, -2);
		
		fMenu->Go(p, true, true, r, true);	
	}
	
	if ( buttons & B_PRIMARY_MOUSE_BUTTON )
	{
		list<BMessenger>::iterator i = fMsgrs.begin();
		
		if ( i != fMsgrs.end() )
		{
			(*i).SendMessage( IM::DESKBAR_ICON_CLICKED );
		}
	}
	

	if ((buttons & B_TERTIARY_MOUSE_BUTTON) || (modifiers() & B_COMMAND_KEY)) {
		entry_ref ref;
		if (get_ref_for_path("/boot/home/people/", &ref) != B_OK) return;
		
		BMessage openPeople(B_REFS_RECEIVED);
		openPeople.AddRef("refs", &ref);
		
		BMessenger tracker("application/x-vnd.Be-TRAK");
		tracker.SendMessage(&openPeople);
	}
}

void
IM_DeskbarIcon::AttachedToWindow()
{
	// give im_server a chance to start up
	snooze(500*1000);	
	reloadSettings();
	
	// register with im_server
	LOG("deskbar", liDebug, "Registering with im_server");
	BMessage msg(IM::REGISTER_DESKBAR_MESSENGER);
	msg.AddMessenger( "msgr", BMessenger(this) );
	
	BMessenger(IM_SERVER_SIG).SendMessage(&msg);
	
	BVolumeRoster volRoster;
	BVolume bootVol;
	BMessenger target(this, NULL, NULL);
	querymap::iterator qIt;
	BPath queryPath;
	entry_ref queryRef;

	find_directory(B_USER_SETTINGS_DIRECTORY, &queryPath, true);
	queryPath.Append("im_kit/queries");

	BDirectory queryDir(queryPath.Path());
	queryDir.Rewind();
	volRoster.GetBootVolume(&bootVol);


	node_ref nref;
	if (queryDir.InitCheck() == B_OK) {
		queryDir.GetNodeRef(&nref);
		watch_node(&nref, B_WATCH_DIRECTORY, target);
	};

	for (int32 i = 0; queryDir.GetNextRef(&queryRef) == B_OK; i++) {
		BNode node(&queryRef);
		BQuery *query = new BQuery();
		
		int32 length = 0;
		char *queryFormula = ReadAttribute(node, "_trk/qrystr", &length);
		queryFormula = (char *)realloc(queryFormula, sizeof(char) * (length + 1));
		queryFormula[length] = '\0';

		query->SetPredicate(queryFormula);
		query->SetVolume(&bootVol);
		query->SetTarget(target);
				
		fQueries[queryRef] = query;
		free(queryFormula);
	
		query->Fetch();	

	};
	
}

void
IM_DeskbarIcon::DetachedFromWindow()
{
}

void
IM_DeskbarIcon::reloadSettings()
{
	LOG("deskbar", liLow, "IM: Requesting settings");
	
	BMessage settings;
	
	im_load_client_settings("im_server", &settings );

	settings.FindBool("blink_db", &fShouldBlink );
			
	if (settings.FindString("person_handler", &fPeopleApp) != B_OK) {
		fPeopleApp = "application/x-vnd.Be-TRAK";
	};
			
	LOG("deskbar", liMedium, "IM: Settings applied");
}
