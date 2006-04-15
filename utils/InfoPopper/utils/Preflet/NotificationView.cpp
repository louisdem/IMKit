#include "NotificationView.h"

#include <Box.h>
#include <CheckBox.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <TextControl.h>

#ifdef B_BEOS_VERSION_ZETA
#include <locale/Locale.h>
#else
#define _T(str) (str)
#endif

#include <stdio.h>
#include <stdlib.h>

#include "AppView.h"
#include "CLV/ColumnListView.h"
#include "CLV/ColumnTypes.h"
#include "Filter/AppUsage.h"
#include "Filter/Notification.h"
#include "common/SettingsFile.h"

//#pragma mark Constants

const float kEdgePadding = 5.0;
const float kCLVTitlePadding = 8.0;
const int32 kTitleCLVIndex = 0;
const int32 kDateCLVIndex = 1;
const int32 kTypeCLVIndex = 2;
const int32 kAllowCLVIndex = 3;

const int32 kAppChanged = 'nv01';

//#pragma mark Constructor

NotificationView::NotificationView(BRect bounds)
	: SettingView(bounds, "NotificationView",
		B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW),
	fAppBox(NULL),
	fAppField(NULL),
	fAppMenu(NULL),
	fCurrentAppMenu(NULL),
	fCurrentAppView(NULL) {
		
};

NotificationView::~NotificationView(void) {
	fAppBox->RemoveSelf();
	fAppField->RemoveSelf();
	
	delete fAppBox;
	
	appview_t::iterator aIt;
	for (aIt = fAppView.begin(); aIt != fAppView.end(); aIt++) {
		aIt->second->RemoveSelf();
		delete aIt->second;
	};
};

//#pragma mark BView Hooks

void NotificationView::AttachedToWindow(void) {
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
#ifdef B_BEOS_VERSION_BETA
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR))
#else
	SetHighColor(0, 0, 0);
#endif

	float divider = 0;
	char *labels[] = {
		_T("Block All Notifications"),
		NULL
	};
	
	BRect rect = Bounds();
	
	fAppBox = new BBox(rect, "AppBox", B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM,
		B_WILL_DRAW);
	AddChild(fAppBox);

	fAppMenu = new BMenu("AppMenu");
	fAppMenu->SetLabelFromMarked(true);
	
	rect = fAppBox->Bounds();
	
	fAppField = new BMenuField(rect, "AppField", "", fAppMenu);
	fAppBox->SetLabel(fAppField);
	fAppField->SetDivider(0);
	
	fChildRect = fAppBox->Frame();
	fChildRect.top = fAppField->Bounds().bottom;
	fChildRect.InsetBy(kEdgePadding, kEdgePadding);
};

void NotificationView::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case kAppChanged: {
			int32 index = -1;
			if (msg->FindInt32("index", &index) != B_OK) return;
			
			fCurrentAppMenu = fAppMenu->ItemAt(index);
			
			fAppMenu->ResizeToPreferred();
			fAppField->ResizeToPreferred();
			
			SetupView(fCurrentAppMenu);
		} break;
		
		default: {
			BView::MessageReceived(msg);
		};
	};
};

void NotificationView::Draw(BRect rect) {
};	

//#pragma mark SettingView hooks

status_t NotificationView::Save(void) {
	SettingsFile settings("appsettings", "BeClan/InfoPopper", B_USER_SETTINGS_DIRECTORY);
	appview_t::iterator avIt;
	appusage_t::iterator auIt;

	for (auIt = fAppUsage.begin(); auIt != fAppUsage.end(); auIt++) {
		avIt = fAppView.find(auIt->first);
		AppUsage *usage = auIt->second;
		
		// If there is a view for this usage use that
		if (avIt != fAppView.end()) usage = avIt->second->Value();
		
		// If there's a usage, save it
		if (usage) settings.AddFlat("app_usage", usage);
	};
	
	return settings.Save();
};

status_t NotificationView::Load(void) {
	status_t err = LoadAppUsage();
	if (err != B_OK) return err;

	const char *app = "";
	if (fCurrentAppMenu) app = fCurrentAppMenu->Label();
	
	int32 items = fAppMenu->CountItems();
	for (int32 i = 0; i < items; i++) delete fAppMenu->RemoveItem(0L);
		
	appusage_t::iterator aIt;
	for (aIt = fAppUsage.begin(); aIt != fAppUsage.end(); aIt++) {
		fAppMenu->AddItem(new BMenuItem(aIt->first.String(), new BMessage(kAppChanged)));
	};

	fCurrentAppMenu = fAppMenu->FindItem(app);
	if (fCurrentAppMenu == NULL) fCurrentAppMenu = fAppMenu->ItemAt(0);	
	fCurrentAppMenu->SetMarked(true);
	
	fAppMenu->SetTargetForItems(this);
	
	fAppMenu->ResizeToPreferred();
	fAppField->ResizeToPreferred();
	
	SetupView(fCurrentAppMenu);
};

//#pragma mark Private

status_t NotificationView::LoadAppUsage(void) {
	SettingsFile settings("appsettings", "BeClan/InfoPopper", B_USER_SETTINGS_DIRECTORY);
	status_t error = settings.InitCheck();
	if (settings.InitCheck() != B_OK) return B_ERROR;

	error = settings.Load();
	
	if (error != B_OK) return B_ERROR;
	
	appview_t::iterator avIt;
	appusage_t::iterator auIt;
	
	for (avIt = fAppView.begin(); avIt != fAppView.end(); avIt++) {
		avIt->second->RemoveSelf();
		delete avIt->second;
	};
	fAppView.clear();
	fCurrentAppView = NULL;

	for (auIt = fAppUsage.begin(); auIt != fAppUsage.end(); auIt++) delete auIt->second;
	fAppUsage.clear();
	
	type_code type;
	int32 count = 0;	
	error = settings.GetInfo("app_usage", &type, &count);
	if (error != B_OK) return B_ERROR;
	
	for (int32 i = 0; i < count; i++) {
		AppUsage *app = new AppUsage();
		settings.FindFlat("app_usage", i, app);
		fAppUsage[app->Name()] = app;
	};
	
	return B_OK;
};

void NotificationView::SetupView(BMenuItem *item) {
	appview_t::iterator vIt = fAppView.find(item->Label());
	AppView *view = NULL;
	if (vIt == fAppView.end()) {
		appusage_t::iterator aIt = fAppUsage.find(item->Label());
		if (aIt == fAppUsage.end()) return;
		view = new AppView(fChildRect, aIt->second);
		fAppBox->AddChild(view);
		
		fAppView[item->Label()] = view;
	} else {
		view = vIt->second;
	};
	
	if ((fCurrentAppView) && (fCurrentAppView->IsHidden() == false)) {
		fCurrentAppView->Hide();
	};
	if (view->IsHidden() == true) view->Show();
	fCurrentAppView = view;
};

