#include "PWindow.h"

#include <libim/Helpers.h>
#include <Entry.h>
#include <Roster.h>
#include <ScrollView.h>

#ifdef ZETA
#include <locale/Locale.h>
#else
#define _T(str) (str)
#endif

#include "SettingView.h"
#include "GeneralView.h"
#include "NotificationView.h"

//#pragma mark Constants

const float kControlOffset = 5.0;
const float kEdgeOffset = 5.0;
const float kDividerWidth = 100;

BubbleHelper gHelper;

//#pragma mark Constructor

PWindow::PWindow(void)
	: BWindow(BRect(5, 25, 600, 385), "InfoPopper Preflet", B_TITLED_WINDOW,
	 B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
	 fView(NULL),
	 fSave(NULL),
	 fRevert(NULL),
	 fListView(NULL),
	 fBox(NULL),
	 fCurrentView(NULL) {

#ifdef ZETA
	app_info ai;
	BPath path;
	
	be_app->GetAppInfo(&ai);

	BEntry entry( &ai.ref, true );
	entry.GetPath(&path);
	path.GetParent(&path);
	path.Append("Language/Dictionaries/InfoPopperPreflet" );
	BString path_string;
	
	if(path.InitCheck() != B_OK) {
		path_string.SetTo("Language/Dictionaries/InfoPopperPreflet");
	} else {
		path_string.SetTo(path.Path());
	};
	
	be_locale.LoadLanguageFile(path_string.String());
#endif
	
	fCurrentIndex = 0;

	BRect frame = Bounds();

	fView = new BView(frame, "PrefView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);

	AddChild(fView);
#if B_BEOS_VERSION > B_BEOS_VERSION_5
	fView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fView->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fView->SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
#else
	fView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fView->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fView->SetHighColor(0, 0, 0, 0);
#endif

	frame.left = kEdgeOffset;
	frame.top = kEdgeOffset;
	frame.bottom = Bounds().bottom - kEdgeOffset;
	frame.right = 120;
	fListView = new BOutlineListView(frame, "LISTVIEW", B_SINGLE_SELECTION_LIST);

	font_height fontHeight;
	be_bold_font->GetHeight(&fontHeight);
	fFontHeight = fontHeight.descent + fontHeight.leading + fontHeight.ascent;

	fBox = new BBox(BRect(fListView->Bounds().right + (kEdgeOffset * 3) + 
		B_V_SCROLL_BAR_WIDTH, kEdgeOffset, 	fView->Bounds().right - kEdgeOffset,
		fView->Bounds().bottom - kEdgeOffset), "BOX", B_FOLLOW_ALL_SIDES);
	fBox->SetLabel(_T("General"));
	
	fView->AddChild(fBox);

	frame = fBox->Bounds();
	frame.InsetBy(kEdgeOffset, kEdgeOffset);
	frame.top += fFontHeight;
	frame.right -= B_V_SCROLL_BAR_WIDTH + 2;

	// Add the preflet options
	IconTextItem *generalItem = new IconTextItem(_T("General"), NULL);
	fListView->AddItem(generalItem);

	IconTextItem *notifyItem = new IconTextItem(_T("Notifications"), NULL);
	fListView->AddItem(notifyItem);
	
	fListView->Select(0);
	
	BScrollView *scroller = new BScrollView("list scroller", fListView, B_FOLLOW_LEFT |
		B_FOLLOW_BOTTOM, 0, false, true);
	fView->AddChild(scroller);

	frame = fBox->Bounds();

	fSave = new BButton(frame, "Save", _T("Save"), new BMessage(SAVE));
	fSave->ResizeToPreferred();
	fBox->AddChild(fSave);

	fSave->MoveTo(frame.right - kEdgeOffset - fSave->Bounds().Width(),
		frame.bottom - kEdgeOffset - fSave->Bounds().Height());

	fRevert = new BButton(frame, "Revert", _T("Revert"), new BMessage(REVERT));
	fRevert->ResizeToPreferred();
	fBox->AddChild(fRevert);
	fRevert->SetEnabled(false);

	frame = fSave->Frame();
	fRevert->MoveTo(frame.left - kControlOffset - fRevert->Bounds().Width(),
		frame.top);

	BRect childFrame = fBox->Bounds();
	childFrame.bottom = frame.top;
	childFrame.top += fontHeight.descent + fontHeight.leading + kEdgeOffset;
	childFrame.InsetBy(kEdgeOffset, kEdgeOffset);

	GeneralView *general = new GeneralView(childFrame);
	fBox->AddChild(general);
	general->Load();
	fCurrentView = general;
	fPrefViews[_T("General")] = general;
//	general->Show();

	NotificationView *notify = new NotificationView(childFrame);
	fBox->AddChild(notify);
	notify->Hide();
	notify->Load();
	fPrefViews[_T("Notifications")] = notify;

	fListView->MakeFocus();
	fListView->SetSelectionMessage(new BMessage(LISTCHANGED));
	fListView->SetTarget(this);
	
	Show();
	fView->Show();	
};

bool PWindow::QuitRequested(void) {
	view_map::iterator vIt;
	for (vIt = fPrefViews.begin(); vIt != fPrefViews.end(); vIt++) {
		BView *view = vIt->second;
		if (!view) continue;
				
		view->RemoveSelf();
		delete view;
	};
	fPrefViews.clear();

	if (fSave) fSave->RemoveSelf();
	delete fSave;
	
	if (fRevert) fRevert->RemoveSelf();
	delete fRevert;
		
	if (fBox) fBox->RemoveSelf();
	delete fBox;
	
	if (fListView) fListView->RemoveSelf();
	delete fListView;
	
	if (fView != NULL) fView->RemoveSelf();
	delete fView;
	
	be_app_messenger.SendMessage(B_QUIT_REQUESTED);
	
	return true;
};

//#pragma mark Hooks

void PWindow::DispatchMessage(BMessage *msg, BHandler *target) {
//	switch (msg->what) {
//		case B_MOUSE_WHEEL_CHANGED: {
//			if (target != fListView) {
//				float delta_y=0.0f;
//				
//				msg->FindFloat("be:wheel_delta_y", &delta_y);
//				
//				fCurrentView->ScrollBy(0, delta_y * 10);
//				return;
//			};
//		} break;
//			
//		default: {
//		}; break;
//	};

	BWindow::DispatchMessage(msg, target);
};

void PWindow::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case LISTCHANGED: {
			int32 index = msg->FindInt32("index");
			if (index < 0) return;
			
			IconTextItem *item = (IconTextItem *)fListView->ItemAt(index);
			if (item == NULL) return;
			
			view_map::iterator vIt = fPrefViews.find(item->Text());
			if (vIt == fPrefViews.end()) {
				fListView->Select(fCurrentIndex);
				return;
			};
			
			if ((fCurrentView) && (fCurrentView->IsHidden() == false)) {
				fCurrentView->Hide();
			};

			fCurrentView = vIt->second;
			if (fCurrentView->IsHidden() == true) fCurrentView->Show();
			fCurrentIndex = index;
			fBox->SetLabel(item->Text());
			
			fView->Invalidate();
		} break;

		case SETTINGS_CHANGED: {
			fRevert->SetEnabled(true);
		} break;

		case REVERT: {
			if (fCurrentView) {
				fCurrentView->Load();
				fRevert->SetEnabled(false);
			};
		} break;
		
		case SAVE: {
			if (fCurrentView) {
				fCurrentView->Save();
				fRevert->SetEnabled(false);
			};
		} break;
		
		default: {
			BWindow::MessageReceived(msg);
		};
	};
};
