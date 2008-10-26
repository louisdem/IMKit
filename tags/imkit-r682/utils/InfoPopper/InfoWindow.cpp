#include "InfoWindow.h"

#include <NodeMonitor.h>
#include <algorithm>
#include <stdio.h>
#include <Debug.h>

#include "AppGroupView.h"
#include "AppUsage.h"
#include "common/SettingsFile.h"

//#pragma mark Constants

property_info main_prop_list[] = {
	{ "message", {B_GET_PROPERTY, 0},{B_INDEX_SPECIFIER, 0}, "get a message"},
	{ "message", {B_COUNT_PROPERTIES, 0}, {B_DIRECT_SPECIFIER, 0}, "count messages"},
	{ "message", {B_CREATE_PROPERTY, 0},{B_DIRECT_SPECIFIER, 0}, "create a message"},
	{ "message", {B_SET_PROPERTY, 0}, {B_INDEX_SPECIFIER, 0 }, "modify a message" },
	0 // terminate list
};

const float kDefaultWidth = 300.0f;
const int16 kDefaultIconSize = 16;
const int32 kDefaultDisplayTime = 10;
const int16 kDefaultLayout = TitleAboveIcon;
const char *kWidthName = "windowWidth";
const char *kIconName = "iconSize";
const char *kTimeoutName = "displayTime";
const char *kLayoutName = "titlePosition";

const float kCloseSize = 8;
const float kExpandSize = 8;
const float kPenSize = 1;
const float kEdgePadding = 5;
const float kSmallPadding = 2;

/* undocumented */
#define B_THIN_BORDER_WINDOW_LOOK ((window_look)2)
#define B_LEFT_TAB_WINDOW_LOOK ((window_look)25) /* only in R5 */

InfoWindow::InfoWindow()
:	BWindow(BRect(10,10,20,20), "InfoWindow", B_BORDERED_WINDOW,
	B_AVOID_FRONT|B_AVOID_FOCUS) {

	SetWorkspaces( 0xffffffff );
	
	fBorder = new BorderView(Bounds(), "InfoPopper");

	SetTitle("InfoPopper");
	//SetLook(B_FLOATING_WINDOW_LOOK);
	SetLook(B_THIN_BORDER_WINDOW_LOOK);
	SetFlags(Flags() /*| B_NOT_MOVABLE*/ | B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE | B_NOT_RESIZABLE );
	
	AddChild( fBorder );	

	Show();
	Hide();
	
	fDeskbarLocation = BDeskbar().Location();
	
	LoadSettings(true);
	LoadAppFilters(true);
};

InfoWindow::~InfoWindow(void) {
	appfilter_t::iterator aIt;
	for (aIt = fAppFilters.begin(); aIt != fAppFilters.end(); aIt++) {
		delete aIt->second;
	};
};

bool InfoWindow::QuitRequested(void) {
	SaveAppFilters();

	appview_t::iterator aIt;
	for (aIt = fAppViews.begin(); aIt != fAppViews.end(); aIt++) {
		aIt->second->RemoveSelf();
		delete aIt->second;
	};
	

	BMessenger(be_app).SendMessage( B_QUIT_REQUESTED );
	return BWindow::QuitRequested();
}

void InfoWindow::WorkspaceActivated(int32 /*workspace*/, bool active) {
//	Ensure window is in the correct position
	if ( active ) ResizeAll();
};

void InfoWindow::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case B_NODE_MONITOR: {
			LoadSettings();
			LoadAppFilters();
		} break;
		
		case ResizeToFit: {
			ResizeAll();
		} break;

		case B_COUNT_PROPERTIES: {
			BMessage reply(B_REPLY);
			BMessage specifier;
			const char *property = NULL;
			bool msgOkay = true;
			
			if (msg->FindMessage("specifiers", 0, &specifier) != B_OK) msgOkay = false;
			if (specifier.FindString("property", &property) != B_OK) msgOkay = false;
			if (strcmp(property, "message") != 0) msgOkay = false;
			
			if (msgOkay) {
				reply.AddInt32("result", fInfoViews.size());
			} else {
				reply.what = B_MESSAGE_NOT_UNDERSTOOD;
				reply.AddInt32("error", B_ERROR);
			};
				
			msg->SendReply(&reply);
		} break;
		
		case B_CREATE_PROPERTY:
		case InfoPopper::AddMessage: {		
			int8 type;
			const char *message = NULL;
			const char *title = NULL;
			const char *app = NULL;
			BMessage reply(B_REPLY);
			bool msgOkay = true;
			
			if (msg->FindInt8("type", &type) != B_OK) type = InfoPopper::Information;
			if (msg->FindString("content", &message) != B_OK) msgOkay = false;
			if (msg->FindString("title", &title) != B_OK) msgOkay = false;
			if (msg->FindString("app", &app) != B_OK && msg->FindString("appTitle", &app) != B_OK) {
				msgOkay = false;
			};
			
			if (msgOkay) {
				InfoView *view = new InfoView(this, (InfoPopper::info_type)type, app,
					title, message, new BMessage(*msg));
//				
//				fInfoViews.push_back(view);			
//				fBorder->AddChild(view);

				appfilter_t::iterator fIt = fAppFilters.find(app);
				bool allow = false;
				if (fIt == fAppFilters.end()) {
					app_info info;
					BMessenger msgr = msg->ReturnAddress();
					if (msgr.IsValid() == true) {
						be_roster->GetRunningAppInfo(msgr.Team(), &info);
					} else {
						be_roster->GetAppInfo("application/x-vnd.Be-SHEL", &info);
					};
					
					AppUsage *appUsage = new AppUsage(info.ref, app, true);
					fAppFilters[app] = appUsage;
					
					appUsage->Allowed(title, (info_type)type);

					allow = true;
				} else {
					allow = fIt->second->Allowed(title, (info_type)type);
				};
				
				if (allow == true) {
					appview_t::iterator aIt = fAppViews.find(app);
					AppGroupView *group = NULL;
					if (aIt == fAppViews.end()) {
						group = new AppGroupView(this, app);
						fAppViews[app] = group;
						fBorder->AddChild(group);
					} else {
						group = aIt->second;
					};
					group->AddInfo(view);
						
					ResizeAll();
					
					reply.AddInt32("error", B_OK);
				} else {
					reply.AddInt32("Error", B_ERROR);
				};
			} else {
				reply.what = B_MESSAGE_NOT_UNDERSTOOD;
				reply.AddInt32("error", B_ERROR);
			};
			
			msg->SendReply(&reply);
		} break;
		
		case REMOVE_VIEW: {
			void *_ptr;
			msg->FindPointer("view", &_ptr);
			
			InfoView *info = reinterpret_cast<InfoView*>(_ptr);
			
			fBorder->RemoveChild(info);
			
			vector<InfoView*>::iterator i = find(fInfoViews.begin(), fInfoViews.end(), info);
			if ( i != fInfoViews.end() )
				fInfoViews.erase(i);
			
			delete info;
			
			ResizeAll();
		} break;
		
		default: {
			BWindow::MessageReceived(msg);
		};
	};
};

BHandler * InfoWindow::ResolveSpecifier(BMessage *msg, int32 index, BMessage *spec, int32 form, const char *prop) {
	BPropertyInfo prop_info(main_prop_list);
	BHandler *handler = NULL;
	
	if ( strcmp(prop,"message") == 0 ) {
		
		printf("Matching specifier..\n");
		
		switch (msg->what) {
			case B_CREATE_PROPERTY: {
				printf("Create\n");
				msg->PopSpecifier();
				handler = this;
			} break;
			
			case B_SET_PROPERTY: 
			case B_GET_PROPERTY: {
				int32 i;
				if ( spec->FindInt32("index",&i) != B_OK ) i = -1;
			
				if ( i >= 0 && i < (int32)fInfoViews.size() ) {
					printf("Found message\n");
					msg->PopSpecifier();
					
					handler = fInfoViews[i];
				} else {
					printf("Index out of range: %ld\n",i);
					msg->PrintToStream();
					
					handler = NULL;
				};
			} break;

			case B_COUNT_PROPERTIES: {
				msg->PopSpecifier();
				handler = this;
			} break;
			
			default: {
				printf("Specifier not supported\n");
				msg->PrintToStream();
			};
		};
	};
	
	if (handler == NULL) {
		handler = BWindow::ResolveSpecifier(msg, index, spec, form, prop);
	};

	return handler;
};

int16 InfoWindow::IconSize(void) {
	return fIconSize;
};

int32 InfoWindow::DisplayTime(void) {
	return fDisplayTime;
};

infoview_layout InfoWindow::Layout(void) {
	return fLayout;
};

float InfoWindow::ViewWidth(void) {
	return fWidth;
};

BPath InfoWindow::SettingsPath(void) {
	return fSettingsPath;
};

//#pragma mark -

void InfoWindow::ResizeAll(void) {
	if (fAppViews.empty() == true) {
		if (IsHidden() == false) Hide();
		return;
	};
	
	appview_t::iterator aIt;
	bool shouldHide = true;
	
	for (aIt = fAppViews.begin(); aIt != fAppViews.end(); aIt++) {
		AppGroupView *app = aIt->second;
		if (app->HasChildren() == true) {
			shouldHide = false;
			break;
		};
	};

	if (shouldHide) {
		if (IsHidden() == false) Hide();
		return;
	};
	
	if (IsHidden() == true) Show();
	
	float width = 0;
	float height = 0;
	float offset = 0;
	
	for (aIt = fAppViews.begin(); aIt != fAppViews.end(); aIt++) {
		AppGroupView *view = aIt->second;
		float w = -1;
		float h = -1;

		if (view->HasChildren() == false) {
			if (view->IsHidden() == false) view->Hide();
		} else {
			view->GetPreferredSize(&w, &h);
			width = max_c(width, h);
			
			view->ResizeToPreferred();
			view->MoveTo(0, height);

			height += h;
			
			if (view->IsHidden() == true) view->Show();
		};
	};
	
	ResizeTo(ViewWidth(), height);
	PopupAnimation(Bounds().Width(), Bounds().Height());
};

void InfoWindow::PopupAnimation(float width, float height) {
	float x,y,sx,sy;
	float pad = 0;
	BDeskbar deskbar;
	BRect frame = deskbar.Frame();
	
	switch ( deskbar.Location() ) {
		case B_DESKBAR_TOP:
			// put it just under, top right corner
			sx = frame.right;
			sy = frame.bottom+pad;
			y = sy;
			x = sx-width-pad;
			break;
		case B_DESKBAR_BOTTOM:
			// put it just above, lower left corner
			sx = frame.right;
			sy = frame.top-height-pad;
			y = sy;
			x = sx - width-pad;
			break;
		case B_DESKBAR_LEFT_TOP:
			// put it just to the right of the deskbar
			sx = frame.right+pad;
			sy = frame.top-height;
			x = sx;
			y = frame.top+pad;
			break;
		case B_DESKBAR_RIGHT_TOP:
			// put it just to the left of the deskbar
			sx = frame.left-width-pad;
			sy = frame.top-height;
			x = sx;
			y = frame.top+pad;
			break;
		case B_DESKBAR_LEFT_BOTTOM:
			// put it to the right of the deskbar.
			sx = frame.right+pad;
			sy = frame.bottom;
			x = sx;
			y = sy-height-pad;
			break;
		case B_DESKBAR_RIGHT_BOTTOM:
			// put it to the left of the deskbar.
			sx = frame.left-width-pad;
			sy = frame.bottom;
			y = sy-height-pad;
			x = sx;
			break;	
		default: break;
	}
	
	MoveTo(x, y);
	
	if (IsHidden() && fInfoViews.size() != 0) {
		Show();
	};
	//Activate();// it hides floaters from apps :-(
};

void InfoWindow::WriteDefaultSettings(BNode *node, bool writeWidth = true,
	bool writeIcon = true, bool writeTimeout = true, bool writeLayout = true) {

	if (writeWidth) {
		node->WriteAttr(kWidthName, B_FLOAT_TYPE, 0, (void *)&kDefaultWidth,
			sizeof(kDefaultWidth));
	};
	
	if (writeIcon) {
		node->WriteAttr(kIconName, B_INT16_TYPE, 0, (void *)&kDefaultIconSize,
			sizeof(kDefaultIconSize));
	};
	
	if (writeTimeout) {
		node->WriteAttr(kTimeoutName, B_INT32_TYPE, 0, (void *)&kDefaultDisplayTime,
			sizeof(kDefaultDisplayTime));
	};
	
	if (writeLayout) {
		node->WriteAttr(kLayoutName, B_INT16_TYPE, 0, (void *)&kDefaultLayout,
			sizeof(kDefaultLayout));
	};
};

void InfoWindow::LoadSettings(bool start_monitor) {
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &fSettingsPath, true, NULL) != B_OK) {
		BAlert *alert = new BAlert("InfoPopper", "Couldn't find the settings "
			" directory. This is very bad.", "Carp!");
		alert->Go();
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		return;
	};
	
	fWidth = kDefaultWidth;
	fIconSize = kDefaultIconSize;
	fDisplayTime = kDefaultDisplayTime;
	fLayout = (infoview_layout)kDefaultLayout;
	
	fSettingsPath.Append("BeClan/InfoPopper/");
	BPath path(fSettingsPath);
	path.Append("settings");
	
	BNode node(path.Path());
	if (node.InitCheck() == B_OK) {
		bool writeWidth = false;
		bool writeIcon = false;
		bool writeTimeout = false;
		bool writeLayout = false;
		
		if (node.ReadAttr(kWidthName, B_FLOAT_TYPE, 0, (void *)&fWidth,
			sizeof(fWidth)) < B_OK) {
			writeWidth = true;
			fWidth = kDefaultWidth;
		};
			
		if (node.ReadAttr(kIconName, B_INT16_TYPE, 0, (void *)&fIconSize,
			sizeof(fIconSize)) < B_OK) {
			writeIcon = true;
			fIconSize = kDefaultIconSize;
		};
		
		if (node.ReadAttr(kTimeoutName, B_INT32_TYPE, 0, (void *)&fDisplayTime,
			sizeof(fDisplayTime)) < B_OK) {
			writeTimeout = true;
			fDisplayTime = kDefaultDisplayTime;
		};
		
		if (node.ReadAttr(kLayoutName, B_INT16_TYPE, 0, (void *)&fLayout,
			sizeof(fLayout)) < B_OK) {
			writeLayout = true;
			fLayout = (infoview_layout)kDefaultLayout;
		};
		
		WriteDefaultSettings(&node, writeWidth, writeIcon, writeTimeout, writeLayout);
		
	} else {
//		Lets just assume it's because the file doesn't exist.
		create_directory(fSettingsPath.Path(), 0777);
		
		BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE);
		
		node.SetTo(path.Path()); // so we can monitor changes on this new file
		
		WriteDefaultSettings(reinterpret_cast<BNode *>(&file));
		
	}

	if ( start_monitor )
	{
		node_ref nref;
		node.GetNodeRef(&nref);
		
		if ( watch_node(&nref, B_WATCH_ATTR, BMessenger(this)) != B_OK )
		{
			BAlert *alert = new BAlert("InfoPopper", "Couldn't start settings "
				" monitor. Live settings changes disabled.", "Darn.");
			alert->Go();
		}
	}
}

void InfoWindow::LoadAppFilters(bool startmonitor = false) {
	SettingsFile settings("appsettings", "BeClan/InfoPopper", B_USER_SETTINGS_DIRECTORY);
	status_t error = settings.InitCheck();
	if (settings.InitCheck() != B_OK) {
		printf("Error initialising App Filters: %s (%i)\n", strerror(error), error);
		return;
	};
	
	error = settings.Load();
	printf("Error loading settings: %s (%i)\n", strerror(error), error);
	
	if (error != B_OK) return;
	
	settings.PrintToStream();

	type_code type;
	int32 count = 0;
	
	error = settings.GetInfo("app_usage", &type, &count);
	if (error != B_OK) return;
	
	for (int32 i = 0; i < count; i++) {
		AppUsage *app = new AppUsage();
		settings.FindFlat("app_usage", i, app);
		fAppFilters[app->Name()] = app;
	};
	
	if (startmonitor) {
		if (watch_node(settings.NodeRef(), B_WATCH_ALL, BMessenger(this)) != B_OK) {
			BAlert *alert = new BAlert("InfoPopper", "Couldn't start filter "
				" monitor. Live filter changes disabled.", "Darn.");
			alert->Go();
		};
	};
};

void InfoWindow::SaveAppFilters(void) {
	SettingsFile settings("appsettings", "BeClan/InfoPopper", B_USER_SETTINGS_DIRECTORY);
	status_t error = settings.InitCheck();

	appfilter_t::iterator fIt;
	for (fIt = fAppFilters.begin(); fIt != fAppFilters.end(); fIt++) {
		settings.AddFlat("app_usage", fIt->second);
	};

	settings.Save();
};
