#include "PWindow.h"

const float kControlOffset = 5.0;
//const float kEdgeOffset = 2.0;
const float kEdgeOffset = 5.0;
//const float kControlWidth = 300;
const float kDividerWidth = 100;

PWindow::PWindow(void)
	: BWindow(BRect(25, 25, 460, 260), "Instant Messaging", B_TITLED_WINDOW,
	 B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS) {
	
	fManager = new IM::Manager(BMessenger(this));
	
	fLastIndex = 0;
	fBox = NULL;
	fView = NULL;
	fSave = NULL;
	fRevert = NULL;
	fListView = NULL;
	fBox = NULL;

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
	frame.right = 100;
	fListView = new BListView(frame, "LISTVIEW", B_SINGLE_SELECTION_LIST);

	font_height fontHeight;
	be_bold_font->GetHeight(&fontHeight);
	fFontHeight = fontHeight.descent + fontHeight.leading + fontHeight.ascent;

	fBox = new BBox(BRect(fListView->Bounds().right + (kEdgeOffset * 3) + 
		B_V_SCROLL_BAR_WIDTH, kEdgeOffset, 	fView->Bounds().right - kEdgeOffset,
		fView->Bounds().bottom - ((fFontHeight * 2) + kEdgeOffset)), "BOX",
		B_FOLLOW_ALL_SIDES);
	fBox->SetLabel("IM Server");
	
	fView->AddChild(fBox);

	frame = fBox->Bounds();
	frame.InsetBy(kEdgeOffset, kEdgeOffset);
	frame.top += fFontHeight;
	
	BMessage requestProts(IM::GET_LOADED_PROTOCOLS);
	BMessage protocols;
	fManager->SendMessage(&requestProts, &protocols);
	protocols.PrintToStream();

	if (protocols.what == IM::ACTION_PERFORMED) {
//		FIX ME: Find the location of the im_server programmatically (By app signature?)
		fListView->AddItem(new IconTextItem("IM Server",
			GetBitmapFromAttribute("/boot/home/config/servers/im_server", "BEOS:M:STD_ICON",
			'ICON')));
	
		BMessage settings;
		BMessage tmplate;
		BMessage reqSettings(IM::GET_SETTINGS);
		reqSettings.AddString("protocol", "");
		BMessage reqTemplate(IM::GET_SETTINGS_TEMPLATE);
		reqTemplate.AddString("protocol", "");

		fManager->SendMessage(&reqSettings, &settings);
		fManager->SendMessage(&reqTemplate, &tmplate);

		fAddOns["IM Server"] = pair<BMessage, BMessage>(settings, tmplate);

		BView *view = new BView(frame, "IM Server", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
#if B_BEOS_VERSION > B_BEOS_VERSION_5
		view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		view->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		view->SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
#else
		view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		view->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		view->SetHighColor(0, 0, 0, 0);
#endif

		BuildGUI(tmplate, settings, view);
		fPrefView.AddItem(view);
		fBox->AddChild(view);

		for (int16 i = 0; protocols.FindString("protocol", i); i++) {
			const char *protocol = protocols.FindString("protocol", i);
			entry_ref ref;
			protocols.FindRef("ref", i, &ref);	
			
			fListView->AddItem(new IconTextItem(protocol,
				GetBitmapFromAttribute(BPath(&ref).Path(), "BEOS:M:STD_ICON", 'ICON')));
			
			BMessage protocol_settings;
			BMessage protocol_template;
			BMessage reqSettings(IM::GET_SETTINGS);
			reqSettings.AddString("protocol", protocol);
			BMessage reqTemplate(IM::GET_SETTINGS_TEMPLATE);
			reqTemplate.AddString("protocol", protocol);
			
			fManager->SendMessage(&reqSettings, &protocol_settings);
			fManager->SendMessage(&reqTemplate, &protocol_template);
		
			pair <BMessage, BMessage> p(protocol_settings, protocol_template);
			fAddOns[protocol] = p; //pair<BMessage, BMessage>(settings, tmplate);

			BView *view = new BView(frame, protocol, B_FOLLOW_ALL_SIDES,
				B_WILL_DRAW);
#if B_BEOS_VERSION > B_BEOS_VERSION_5
			view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			view->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			view->SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
#else
			view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			view->SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			view->SetHighColor(0, 0, 0, 0);
#endif

			BuildGUI(protocol_template, protocol_settings, view);
			fPrefView.AddItem(view);
			fBox->AddChild(view);
			view->Hide();
			
		};
	} else {
		printf("Fatal error - could not get protocols\n");
		BMessenger(be_app).SendMessage(B_QUIT_REQUESTED);
		
		return;
	};

	BScrollView *scroller = new BScrollView("list scroller", fListView, B_FOLLOW_LEFT |
		B_FOLLOW_BOTTOM, 0, false, true);
	fView->AddChild(scroller);

	fListView->Select(0);
	fListView->MakeFocus();

	frame = fView->Frame();
	frame.InsetBy(kEdgeOffset, kEdgeOffset);
	frame.bottom -= (kEdgeOffset * 2); // * 5);
	frame.top = frame.bottom - ((fontHeight.descent + fontHeight.leading + fontHeight.ascent));
	frame.left = frame.right - (be_plain_font->StringWidth("APPLY") +
		(kControlOffset * 2));

	fSave = new BButton(frame, "Save", "Save", new BMessage(SAVE));
	fView->AddChild(fSave);

	frame.right = frame.left - kControlOffset;
	frame.left = frame.right - (be_plain_font->StringWidth("REVERT") +
		(kControlOffset * 2));

	fRevert = new BButton(frame, "Revert", "Revert", new BMessage(REVERT));
	fView->AddChild(fRevert);

	Show();
	fView->Show();
	
	fListView->SetSelectionMessage(new BMessage(LISTCHANGED));
	fListView->SetTarget(this);

};

bool PWindow::QuitRequested(void) {
	for (int32 i = 0; i < fPrefView.CountItems(); i++) {
		BView *view = fPrefView.RemoveItemAt(i);
		if (!view) continue;
		
		for (int32 j = 0; j < view->CountChildren(); j++) {
			BView *child = view->ChildAt(j);
			if (!child) continue;
			
			child->RemoveSelf();
			delete child;
		};
		
		view->RemoveSelf();
		delete view;
	};

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
	
	BMessenger(be_app).SendMessage(B_QUIT_REQUESTED);
	
	return true;
};

void PWindow::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case LISTCHANGED: {
			int32 index = msg->FindInt32("index");
			if (index < 0) return;
			
			BView *last = fPrefView.ItemAt(fLastIndex);
			last->Hide();
			
			BView *current = fPrefView.ItemAt(index);
			current->Show();
			
			IconTextItem *item = ((IconTextItem *)fListView->ItemAt(index));
			fBox->SetLabel(item->Text());

			fLastIndex = index;
			fView->Invalidate();			
		} break;

		case REVERT: {
//			fOrigSettings->Save();
//			fSettings->Load();
				
			
//			fSpeedBox->Revert();
//			fButtonBox->Revert();
//			fSettings->SetSave(false);
		} break;
		
		case SAVE: {
			BMessage outSettings(IM::SET_SETTINGS);
			BMessage cur;
			BMessage tmplate;
			BMessage settings(IM::SETTINGS);
			BMessage reply;
			
			int current = fListView->CurrentSelection();
			if (current < 0) {
				printf("Error, no selection when trying to update\n");
				return;
			};
			
			IconTextItem *item = (IconTextItem *)fListView->ItemAt(current);
			pair<BMessage, BMessage> p = fAddOns[item->Text()];
			
			tmplate = p.second;
			tmplate.PrintToStream();
			
			BView * panel = FindView(item->Text());
			
			for (int i = 0; tmplate.FindMessage("setting", i, &cur) == B_OK; i++) {
				const char *name = cur.FindString("name");
				int32 type = -1;
				
				cur.FindInt32("type", &type);
				
				if ( dynamic_cast<BTextControl*>(panel->FindView(name))) { 
//					Free text
					BTextControl * ctrl = (BTextControl*)panel->FindView(name);
				
					switch (type) {
						case B_STRING_TYPE: {
							settings.AddString(name, ctrl->Text() );
						} break;
						case B_INT32_TYPE: {
							settings.AddInt32(name, atoi(ctrl->Text()) );
						} break;
						default: {
							return;
						};
					};
				} else if (dynamic_cast<BMenuField*>(panel->FindView(name))) {
//					Provided option
					BMenuField * ctrl = (BMenuField*)panel->FindView(name);
					BMenuItem * item = ctrl->Menu()->FindMarked();
					
					if (!item) return;
					
					switch (type) {
						case B_STRING_TYPE: {
							settings.AddString(name, item->Label() );
						} break;
						case  B_INT32_TYPE: {
							settings.AddInt32(name, atoi(item->Label()) );
						} break;
						default: {
							return;
						};
					}
				} else
				if (dynamic_cast<BCheckBox*>(panel->FindView(name))) {
// 					Boolean setting
					BCheckBox * box = (BCheckBox*)panel->FindView(name);
					
					if ( box->Value() == B_CONTROL_ON ) {
						settings.AddBool(name,true);
					} else {
						settings.AddBool(name,false);
					}
				} else if (dynamic_cast<BTextView *>(panel->FindView(name))) {
					BTextView *view = (BTextView *)panel->FindView(name);
					settings.AddString(name, view->Text());
				};
				
				settings.PrintToStream();
			};
			
			if (current == 0) {
				outSettings.AddString("protocol", "");
			} else {
				outSettings.AddString("protocol", item->Text());
			};
			outSettings.AddMessage("settings", &settings);
			outSettings.PrintToStream();
			
			fManager->SendMessage(&outSettings, &reply);
			reply.PrintToStream();
			if (reply.what != IM::ACTION_PERFORMED) {
				printf("Error applying settings\n");
			};


		} break;
		default: {
			BWindow::MessageReceived(msg);
		};
	};
};

status_t PWindow::BuildGUI(BMessage viewTemplate, BMessage settings, BView *view) {
	BMessage curr;
	float yOffset = kEdgeOffset + kControlOffset;
	float xOffset = 0;
	
	const float kControlWidth = view->Bounds().Width() - (kEdgeOffset * 2);
	
	for (int i=0; viewTemplate.FindMessage("setting",i,&curr) == B_OK; i++ ) {
		char temp[512];
		
		// get text etc from template
		const char * name = curr.FindString("name");
		const char * desc = curr.FindString("description");
		const char * value = NULL;
		int32 type = -1;
		bool secret = false;
		bool freeText = true;
		bool multiLine = false;
		BView *control = NULL;
		BMenu *menu = NULL;
		
		if (curr.FindInt32("type", &type) != B_OK) {
			printf("Error getting type for %s, skipping\n");
			continue;
		};

		switch (type) {
			case B_STRING_TYPE: {
				if (curr.FindString("valid_value")) {
					freeText = false;
			
					menu = new BPopUpMenu(name);
//					menu->SetDivider(be_plain_font->StringWidth(name) + 10);
					
					for (int j = 0; curr.FindString("valid_value", j); j++) {
						menu->AddItem(new BMenuItem(curr.FindString("valid_value", j),NULL));
					};
					
					value = settings.FindString(name);
					
					if (value) menu->FindItem(value)->SetMarked(true);
				} else {
					if (curr.FindBool("multi_line", &multiLine) != B_OK) multiLine = false;
					value = settings.FindString(name);
					if (!value) value = curr.FindString("default");
					if (curr.FindBool("is_secret",&secret) != B_OK) secret = false;
				}
			} break;
			case B_INT32_TYPE: {
				if (curr.FindInt32("valid_value")) {
					freeText = false;
					
					menu = new BPopUpMenu(name);
					
					int32 v = 0;
					for ( int j = 0; curr.FindInt32("valid_value",j,&v) == B_OK; j++ ) {
						sprintf(temp,"%ld", v);
						menu->AddItem(new BMenuItem(temp, NULL));
					};
				} else {
					int32 v = 0;
					if (settings.FindInt32(name,&v) == B_OK) {
						sprintf(temp,"%ld",v);
						value = temp;
					} else if ( curr.FindInt32("default",&v) == B_OK ) {
						sprintf(temp,"%ld",v);
						value = temp;
					}
					if (curr.FindBool("is_secret",&secret) != B_OK) secret = false;
				}
			} break;
			case B_BOOL_TYPE: {
				bool active;
				
				if (settings.FindBool(name, &active) != B_OK) {
					if (curr.FindBool("default", &active) != B_OK) {
						active = false;
					};
				};
			
				control = new BCheckBox(BRect(0, 0, kControlWidth, fFontHeight),
					name, desc, NULL);
				printf("%s is Active? %i\n", name, active);
				if (active) ((BCheckBox*)control)->SetValue(B_CONTROL_ON);
				settings.PrintToStream();
			} break;			
			default: {
				continue;
			};
		};
		
		if (!value) value = "";
		
		if (!control) {
			if (freeText) {
				if (multiLine == false) {
					control = new BTextControl(
						BRect(0, 0, kControlWidth, fFontHeight), name,
						desc, value, NULL);
					if (secret) {
						((BTextControl *)control)->TextView()->HideTyping(true);
						((BTextControl *)control)->SetText(value);
					};
					((BTextControl *)control)->SetDivider(kDividerWidth);
				} else {
					BRect labelRect(0, 0, kDividerWidth, fFontHeight);
					BStringView *label = new BStringView(labelRect, "NA", desc,
						B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
					view->AddChild(label);
					label->MoveTo(kEdgeOffset, yOffset);

					BRect rect(0, 0, kControlWidth - kDividerWidth, fFontHeight * 4);
					rect.right -= B_V_SCROLL_BAR_WIDTH + kEdgeOffset + kControlOffset;
					BRect textRect = rect;
					textRect.InsetBy(kEdgeOffset, kEdgeOffset);
					textRect.OffsetTo(1.0, 1.0);

					xOffset = kEdgeOffset + kDividerWidth;
					BTextView *textView = new BTextView(rect, name, textRect,
						B_FOLLOW_ALL_SIDES, B_WILL_DRAW);

					control = new BScrollView("NA", textView, B_FOLLOW_ALL_SIDES,
						B_WILL_DRAW | B_NAVIGABLE, false, true);
					textView->SetText(value);			
				};
			} else {
				control = new BMenuField(BRect(0, 0, kControlWidth, fFontHeight),
					name, desc, menu);
				((BMenuField *)control)->SetDivider(kDividerWidth);
			};
		};
		
		view->AddChild(control);
			
		float h, w = 0;
		control->GetPreferredSize(&w, &h);
		control->MoveTo(kEdgeOffset + xOffset, yOffset);
		yOffset += kControlOffset + h;
		xOffset = 0;
	};
	
};
