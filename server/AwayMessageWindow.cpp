#include "AwayMessageWindow.h"

#include <String.h>

#include <stdio.h>
#include <string.h>

const float kPadding = 5.0;

AwayMessageWindow::AwayMessageWindow(const char *protocol = NULL)
	: BWindow(BRect(100, 100, 325, 220), "IM Kit: Set Away Message", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
	
	fProtocol(NULL) {

	if (protocol != NULL) {
		fProtocol = strdup(protocol);
		BString t = "IM Kit: Set Away Message (";
		t << fProtocol;
		t << ")";
		SetTitle(t.String());
	};

	font_height height;
	be_plain_font->GetHeight(&height);
	fFontHeight = height.ascent + height.descent + height.leading;

	BRect rect = Bounds();

	fView = new BView(rect, "AwayView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
#if B_BEOS_VERSION > B_BEOS_VERSION_5
	fView->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	fView->SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	fView->SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
#else
	fView->SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	fView->SetLowColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	// XXX bga : R5 has no B_PANEL_TEXT_COLOR, I am guessing it is supposed
	// to be black as default so I am setting this to black. 
	fView->SetHighColor(0, 0, 0, 255);
#endif	
	AddChild(fView);

	rect.InsetBy(kPadding, kPadding);
	rect.bottom -= (fFontHeight + (kPadding * 3));
	rect.right -= B_V_SCROLL_BAR_WIDTH;
	
	BRect textRect = rect.InsetBySelf(2.0, 2.0);
	textRect.OffsetTo(0, 0);
	
	fTextView = new BTextView(rect, "AwayMsg", textRect, B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW);
#if B_BEOS_VERSION > B_BEOS_VERSION_5
	fTextView->SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	fTextView->SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	fTextView->SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
#else
	fTextView->SetViewColor(245, 245, 245, 0);
	fTextView->SetLowColor(245, 245, 245, 0);
	fTextView->SetHighColor(0, 0, 0, 0);
#endif

	fScroller = new BScrollView("AwayScroller", fTextView, B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW, false, true);
	fView->AddChild(fScroller);
	
	rect = Bounds();
	rect.InsetBy(kPadding, kPadding);
	
	rect.left = rect.right - (be_plain_font->StringWidth("Set Away") + (kPadding * 2));
	rect.bottom -= kPadding;
	rect.top = rect.bottom - (fFontHeight + kPadding);
	
	fOkay = new BButton(rect, "OkayButton", "Set Away", new BMessage(SET_AWAY),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fView->AddChild(fOkay);
	
	rect.right = rect.left - kPadding;
	rect.left = rect.right - (be_plain_font->StringWidth("Cancel") + (kPadding * 2));
	
	fCancel = new BButton(rect, "CancelButton", "Cancel", new BMessage(CANCEL_AWAY),
		B_FOLLOW_LEFT | B_FOLLOW_BOTTOM);
	fView->AddChild(fCancel);

	IM::Manager man;
	BMessage settings;
	BMessage reqSettings(IM::GET_SETTINGS);
	reqSettings.AddString("protocol", "");

	man.SendMessage(&reqSettings, &settings);	

	fTextView->SetText(settings.FindString("default_away"));	

	fTextView->MakeFocus(true);
}

AwayMessageWindow::~AwayMessageWindow(void) 
{
	if ( fProtocol )
		free(fProtocol);

/*	if (fScroller) 
		fScroller->RemoveSelf();
	delete fScroller;
	
	delete fTextView;
	
	if (fOkay) 
		fOkay->RemoveSelf();
	delete fOkay;
	
	if (fCancel) 
		fCancel->RemoveSelf();
	delete fCancel;
	
	if (fView) 
		fView->RemoveSelf();
	
	delete fView;
*/
};

bool AwayMessageWindow::QuitRequested(void) {
	//return BWindow::QuitRequested();
	return true;
};

void AwayMessageWindow::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case CANCEL_AWAY: {
			//Quit();
			PostMessage(B_QUIT_REQUESTED);
		} break;
		
		case SET_AWAY: {
			BMessage status(IM::MESSAGE);
			status.AddInt32("im_what", IM::SET_STATUS);
			if (fProtocol) status.AddString("protocol", fProtocol);
			status.AddString("away_msg", fTextView->Text());
			status.AddString("status", AWAY_TEXT);
			
			IM::Manager man;
			
			man.OneShotMessage(&status);
			
			//Quit();
			PostMessage(B_QUIT_REQUESTED);
		} break;
	
		default: {
			BWindow::MessageReceived(msg);
		};
	};
};
