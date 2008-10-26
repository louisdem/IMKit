#include "AppView.h"

#include <CheckBox.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <TextControl.h>

#ifdef B_BEOS_VERSION_ZETA
#include <locale/Locale.h>
#else
#define _T(str) (str)
#endif

#include <stdio.h>
#include <stdlib.h>

#include "CLV/ColumnListView.h"
#include "CLV/ColumnTypes.h"
#include "Filter/AppUsage.h"
#include "Filter/Notification.h"
#include "common/SettingsFile.h"
#include "main.h"

//#pragma mark Constants

const float kEdgePadding = 5.0;
const float kCLVTitlePadding = 8.0;
const int32 kTitleCLVIndex = 0;
const int32 kDateCLVIndex = 1;
const int32 kTypeCLVIndex = 2;
const int32 kAllowCLVIndex = 3;

const int32 kCLVInvoked = 'av01';
const int32 kCLVDeleteRow = 'av02';

//#pragma mark Filter Hooks

filter_result CatchDelete(BMessage *msg, BHandler **target, BMessageFilter *filter) {
	filter_result result = B_DISPATCH_MESSAGE;

	// If the delete key is pressed, fire off a message to the parent view
	
	int32 key;
	if (msg->FindInt32("raw_char", &key) == B_OK) {
		if (key == B_DELETE) {
			BView *parent = ((BView *)*target)->Parent();
			BMessenger(parent).SendMessage(new BMessage(kCLVDeleteRow));

			result = B_SKIP_MESSAGE;
		};
	};
	
	return result;
};

//#pragma mark Constructor

AppView::AppView(BRect bounds, AppUsage *usage)
	: BView(bounds, usage->Name(),
		B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM, B_WILL_DRAW),
	fUsage(usage),
	fBlockAll(NULL),
	fNotifications(NULL),
	fTitleCol(NULL),
	fDateCol(NULL),
	fTypeCol(NULL),
	fAllowCol(NULL) {
		
};

AppView::~AppView(void) {
	fBlockAll->RemoveSelf();
	fNotifications->RemoveSelf();
	
	delete fBlockAll;
	delete fNotifications;
};

//#pragma mark BView Hooks

void AppView::AttachedToWindow(void) {
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

	fBlockAll = new BCheckBox(rect, "BlockAll", _T("Block All Notifications"),
		new BMessage(SETTINGS_CHANGED));
	AddChild(fBlockAll);
	fBlockAll->ResizeToPreferred();
	fBlockAll->MoveTo(kEdgePadding, 0);
	fBlockAll->SetTarget(Window());
	
	fNotifications = new BColumnListView(rect, "Notifications",
		B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW, B_FANCY_BORDER, true);
	AddChild(fNotifications);
	fNotifications->ResizeToPreferred();
	fNotifications->MoveTo(kEdgePadding, fBlockAll->Frame().bottom + kEdgePadding);
	fNotifications->ResizeTo(rect.Width() - (kEdgePadding * 2),
		rect.bottom - kEdgePadding - fNotifications->Frame().top);
	
	fTitleCol = new BStringColumn(_T("Title"), 100,
		be_plain_font->StringWidth(_T("Title")) + (kCLVTitlePadding * 2),
		rect.Width(), B_TRUNCATE_END, B_ALIGN_LEFT);
	fDateCol = new BDateColumn(_T("Last Received"), 100,
		be_plain_font->StringWidth(_T("Last Received")) + (kCLVTitlePadding * 2),
		rect.Width(), B_ALIGN_LEFT);
	fTypeCol = new BStringColumn(_T("Type"), 100,
		be_plain_font->StringWidth(_T("Type")) + (kCLVTitlePadding * 2),
		rect.Width(), B_TRUNCATE_END, B_ALIGN_LEFT);
	fAllowCol = new BStringColumn(_T("Allowed"), 100,
		be_plain_font->StringWidth(_T("Allowed")) + (kCLVTitlePadding * 2),
		rect.Width(), B_TRUNCATE_END, B_ALIGN_LEFT);

	fNotifications->AddColumn(fTitleCol, kTitleCLVIndex);
	fNotifications->AddColumn(fDateCol, kDateCLVIndex);
	fNotifications->AddColumn(fTypeCol, kTypeCLVIndex);
	fNotifications->AddColumn(fAllowCol, kAllowCLVIndex);
	
	fNotifications->SetTarget(this);
	fNotifications->SetInvocationMessage(new BMessage(kCLVInvoked));
	fNotifications->SetSelectionMode(B_SINGLE_SELECTION_LIST);
		
	fNotifications->AddFilter(new BMessageFilter(B_ANY_DELIVERY,
		B_ANY_SOURCE, B_KEY_DOWN, CatchDelete));

	Populate();
};

void AppView::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case kCLVInvoked: {
			BRow *row = fNotifications->CurrentSelection();
			if (row == NULL) return;
			BStringField *enabled = dynamic_cast<BStringField *>(row->GetField(kAllowCLVIndex));
			if (strcmp(enabled->String(), _T("Yes")) == 0) {
				enabled->SetString(_T("No"));
			} else {
				enabled->SetString(_T("Yes"));
			};
			fNotifications->UpdateRow(row);
			
			BMessenger msgr(Window()->Looper());
			msgr.SendMessage(SETTINGS_CHANGED);
		} break;
				
		case kCLVDeleteRow: {
			BRow *row = fNotifications->CurrentSelection();
			if (row) {
				fNotifications->RemoveRow(row);
				delete row;

				BMessenger msgr(Window()->Looper());
				msgr.SendMessage(SETTINGS_CHANGED);
			};
		} break;
				
		default: {
			BView::MessageReceived(msg);
		};
	};
};

//#pragma mark Public

void AppView::Populate(void) {
	int32 size = fUsage->Notifications();
	
	if (fUsage->Allowed() == false) fBlockAll->SetValue(B_CONTROL_ON);
	
	for (int32 i = 0; i < size; i++) {
		Notification *notify = fUsage->NotificationAt(i);
		time_t updated = notify->LastReceived();
		const char *allow = notify->Allowed() ? _T("Yes") : _T("No");
		const char *type = "";
		
		switch (notify->InfoType()) {
			case InfoPopper::Information: type = _T("Information"); break;
			case InfoPopper::Important: type = _T("Important"); break;
			case InfoPopper::Error: type = _T("Error"); break;
			case InfoPopper::Progress: type = _T("Progress"); break;
			default: type = _T("Unknown"); break;
		};
		
		BRow *row = new BRow();
		row->SetField(new BStringField(notify->Title()), kTitleCLVIndex);
		row->SetField(new BDateField(&updated), kDateCLVIndex);
		row->SetField(new BStringField(type), kTypeCLVIndex);
		row->SetField(new BStringField(allow), kAllowCLVIndex);
				
		fNotifications->AddRow(row);
	};
};

AppUsage *AppView::Value(void) {
	AppUsage *usage = new AppUsage(fUsage->Ref(), fUsage->Name(),
		fBlockAll->Value() == B_CONTROL_OFF);
	
	int32 rows = fNotifications->CountRows();
	for (int32 i = 0; i < rows; i++) {
		BRow *row = fNotifications->RowAt(i);
		const char *title;
		info_type type;
		bool enabled = true;
		time_t when;
		
		BStringField *sField;
		BDateField *dField;
		
		// Grab the title
		sField = dynamic_cast<BStringField *>(row->GetField(kTitleCLVIndex));
		title = sField->String();

		// Allow
		sField = dynamic_cast<BStringField *>(row->GetField(kAllowCLVIndex));
		if (strcmp(sField->String(), _T("No")) == 0) enabled = false;
		
		// Date
		dField = dynamic_cast<BDateField *>(row->GetField(kDateCLVIndex));
		when = dField->UnixTime();
		
		// Type
		sField = dynamic_cast<BStringField *>(row->GetField(kTypeCLVIndex));
		if (strcmp(sField->String(), _T("Information")) == 0) type = InfoPopper::Information;
		if (strcmp(sField->String(), _T("Important")) == 0) type = InfoPopper::Important;
		if (strcmp(sField->String(), _T("Error")) == 0) type = InfoPopper::Error;
		if (strcmp(sField->String(), _T("Progress")) == 0) type = InfoPopper::Progress;

		Notification *notify = new Notification(title, type, enabled);
		notify->SetTimeStamp(when);
		
		usage->AddNotification(notify);
	};
	
	return usage;
};

//#pragma mark Private

