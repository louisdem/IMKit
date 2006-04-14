#include "GeneralView.h"

#include <Bitmap.h>
#include <Box.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Node.h>
#include <Path.h>
#include <TextControl.h>

#ifdef B_BEOS_VERSION_ZETA
#include <locale/Locale.h>
#else
#define _T(str) (str)
#endif

#include <stdio.h>
#include <stdlib.h>

#include "main.h"

//#pragma mark Constants

const float kEdgeOffset = 5.0;
const float kControlOffset = 5.0;

// Ung! This should all come from an InfoPopper header
const char *kWidthName = "windowWidth";
const char *kIconName = "iconSize";
const char *kTimeoutName = "displayTime";
const char *kLayoutName = "titlePosition";

enum infoview_layout {
	TitleAboveIcon = 0,
	AllTextRightOfIcon = 1
};


const float kDefaultWidth = 300.0f;
const int16 kDefaultIconSize = 16;
const int32 kDefaultDisplayTime = 10;
const int16 kDefaultLayout = TitleAboveIcon;

//#pragma mark Constructor

GeneralView::GeneralView(BRect bounds)
	: SettingView(bounds, "GeneralView", B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM,
		B_WILL_DRAW),
	fTitlePositionField(NULL),
	fTitlePosition(NULL),
	fIconSize(NULL),
	fWindowWidth(NULL),
	fDisplayTime(NULL),
	fExample(NULL),
	fExampleBox(NULL),
	fSettings(NULL) {
};

GeneralView::~GeneralView(void) {
	fTitlePositionField->RemoveSelf();
	fIconSize->RemoveSelf();
	fWindowWidth->RemoveSelf();
	fDisplayTime->RemoveSelf();
	fExampleBox->RemoveSelf();
	
	delete fTitlePositionField;
	delete fIconSize;
	delete fWindowWidth;
	delete fDisplayTime;
	delete fExampleBox;
	delete fExample;
	
	delete fSettings;
};

//#pragma mark BView Hooks

void GeneralView::AttachedToWindow(void) {
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
#ifdef B_BEOS_VERSION_BETA
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR))
#else
	SetHighColor(0, 0, 0);
#endif

	char *labels[] = {
		_T("Window Width"),
		_T("Icon Size"),
		_T("Display Time"),
		_T("Title Position"),
		NULL
	};
	float divider = 0;
	
	for (int32 i = 0; labels[i] != NULL; i++) {
		divider = max_c(divider, be_bold_font->StringWidth(labels[i]));
	};
	divider += kEdgeOffset;

	BRect rect = Bounds();

	fWindowWidth = new BTextControl(rect, "WindowWidth", _T("Window Width"),
		"300", new BMessage(SETTINGS_CHANGED));
	AddChild(fWindowWidth);

	fWindowWidth->ResizeToPreferred();
	fWindowWidth->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fWindowWidth->SetDivider(divider);
	fWindowWidth->SetTarget(Window());
	
	fIconSize = new BTextControl(rect, "IconSize", _T("Icon Size"), "64",
		new BMessage(SETTINGS_CHANGED));
	AddChild(fIconSize);
	
	fIconSize->ResizeToPreferred();
	fIconSize->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fIconSize->SetDivider(divider);
	fIconSize->MoveTo(0, fWindowWidth->Frame().bottom + kControlOffset);
	fIconSize->SetTarget(Window());
	
	fDisplayTime = new BTextControl(rect, "DisplayTime", _T("Display Time"),
		"10", new BMessage(SETTINGS_CHANGED));
	AddChild(fDisplayTime);
		
	fDisplayTime->ResizeToPreferred();
	fDisplayTime->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fDisplayTime->SetDivider(divider);
	fDisplayTime->MoveTo(0, fIconSize->Frame().bottom + kControlOffset);
	fDisplayTime->SetTarget(Window());
	
	fTitlePosition = new BMenu("TitlePosition");
	fTitlePosition->AddItem(new BMenuItem(_T("Above icon"), new BMessage(SETTINGS_CHANGED)));
	fTitlePosition->AddItem(new BMenuItem(_T("Right of icon"), new BMessage(SETTINGS_CHANGED)));
	fTitlePosition->SetLabelFromMarked(true);
	
	fTitlePositionField = new BMenuField(rect, "TitlePositionField",
		_T("Title Position"), fTitlePosition);
	AddChild(fTitlePositionField);
		
	fTitlePositionField->ResizeToPreferred();
	fTitlePositionField->SetAlignment(B_ALIGN_RIGHT);
	fTitlePositionField->SetDivider(divider);
	fTitlePositionField->MoveTo(0, fDisplayTime->Frame().bottom + kControlOffset);
	fTitlePosition->SetTargetForItems(Window());
	
	fExampleBox = new BBox(rect, "ExampleBox", B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM,
		B_WILL_DRAW);
	AddChild(fExampleBox);
	
	fExampleBox->SetLabel(_T("Example Notification"));
	fExampleBox->ResizeToPreferred();
	fExampleBox->MoveTo(0, fTitlePositionField->Frame().bottom + kControlOffset);
	fExampleBox->ResizeTo(fExampleBox->Bounds().Width(),
		rect.bottom - fExampleBox->Frame().top - kEdgeOffset);
		
	Load();
};

void GeneralView::MessageReceived(BMessage *msg) {
	BView::MessageReceived(msg);
};

void GeneralView::Draw(BRect rect) {
};

//#pragma mark SettingView Hooks

status_t GeneralView::Save(void) {
	if (fSettings == NULL) fSettings = GetSettingsNode();
	if (fSettings == NULL) return B_ERROR;
	
	status_t err = fSettings->InitCheck();
	if (err != B_OK) return err;
	
	float width = atof(fWindowWidth->Text());
	fSettings->WriteAttr(kWidthName, B_FLOAT_TYPE, 0, (void *)&width, sizeof(width));
	
	int16 iconSize = atoi(fIconSize->Text());
	fSettings->WriteAttr(kIconName, B_INT16_TYPE, 0, (void *)&iconSize, sizeof(iconSize));
	
	int32 display = atol(fDisplayTime->Text());
	fSettings->WriteAttr(kTimeoutName, B_INT32_TYPE, 0, (void *)&display, sizeof(display));
	
	int16 layout = fTitlePosition->IndexOf(fTitlePosition->FindMarked());
	if (layout == B_ERROR) layout = 0;
	fSettings->WriteAttr(kLayoutName, B_INT16_TYPE, 0, (void *)&layout, sizeof(layout));

	return B_OK;
};

status_t GeneralView::Load(void) {
	if (fSettings == NULL) fSettings = GetSettingsNode();
	if (fSettings == NULL) return B_ERROR;

	char buffer[255];
	status_t err = fSettings->InitCheck();
	if (err != B_OK) return err;

	float width;
	if (fSettings->ReadAttr(kWidthName, B_FLOAT_TYPE, 0, (void *)&width,
		sizeof(width)) < B_OK) width = kDefaultWidth;
		
	sprintf(buffer, "%.2f", width);
	fWindowWidth->SetText(buffer);
	
	int16 iconSize;
	if (fSettings->ReadAttr(kIconName, B_INT16_TYPE, 0, (void *)&iconSize,
		sizeof(iconSize)) < B_OK) iconSize = kDefaultIconSize;
		
	sprintf(buffer, "%i", iconSize);
	fIconSize->SetText(buffer);
	
	int32 display;
	if (fSettings->ReadAttr(kTimeoutName, B_INT32_TYPE, 0, (void *)&display,
		sizeof(display)) < B_OK) display = kDefaultDisplayTime;
		
	sprintf(buffer, "%i", display);
	fDisplayTime->SetText(buffer);

	int16 layout;
	if (fSettings->ReadAttr(kLayoutName, B_INT16_TYPE, 0, (void *)&layout,
		sizeof(layout)) < B_OK) layout = kDefaultLayout;
	
	BMenuItem *item = fTitlePosition->ItemAt(layout);
	if (item) item->SetMarked(true);

	return B_OK;
};

//#pragma mark Private
BNode *GeneralView::GetSettingsNode(void) {
	BPath path;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true, NULL) != B_OK) {
		printf("GeneralView::GetSettingsNode()\tUnable to find settings path\n");
		return NULL;
	};

	path.Append("BeClan/InfoPopper/settings");
	return new BNode(path.Path());
};
