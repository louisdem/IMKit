#include "PWindow.h"

#include <libim/Helpers.h>
#include <Entry.h>
#include <Roster.h>

#ifdef ZETA
#include <locale/Locale.h>
#else
#define _T(str) (str)
#endif

const float kControlOffset = 5.0;
const float kEdgeOffset = 5.0;
const float kDividerWidth = 100;

PWindow::PWindow(void)
	: BWindow(BRect(25, 25, 460, 260), "Instant Messaging", B_TITLED_WINDOW,
	 B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS) {
#ifdef ZETA
	app_info ai;
	be_app->GetAppInfo( &ai );
	BPath path;
	BEntry entry( &ai.ref, true );
	entry.GetPath( &path );
	path.GetParent( &path );
	path.Append( "Language/Dictionaries/InstantMessaging" );
	BString path_string;
	
	if( path.InitCheck() != B_OK )
		path_string.SetTo( "Language/Dictionaries/InstantMessaging" );
	else
		path_string.SetTo( path.Path() );
	
	be_locale.LoadLanguageFile( path_string.String() );
#endif
	
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
	
	// PROTOCOLS
	
	BMessage protocols;
	im_get_protocol_list(&protocols);
	
	protocols.PrintToStream();
	
	if (protocols.FindString("protocol")) {
//		FIX ME: Find the location of the im_server programmatically (By app signature?)
		for (int16 i = 0; protocols.FindString("protocol", i); i++) {
			const char *protocol = protocols.FindString("protocol", i);
			entry_ref ref;
			//protocols.FindRef("ref", i, &ref);	
			
//			XXX Fix Me: Change to use find_directory()
			BString protoPath = "/boot/home/config/add-ons/im_kit/protocols/";
			protoPath << protocol;
			
			BMessage protocol_settings;
			BMessage protocol_template;
			
			im_load_protocol_settings( protocol, &protocol_settings );
			im_load_protocol_template( protocol, &protocol_template );
			
			//printf("Getting icon from %s\n", BPath(&ref).Path() );

			BBitmap *icon = ReadNodeIcon(protoPath.String(), kSmallIcon, true);

			fListView->AddItem(new IconTextItem(protocol, icon));
			
			protocol_template.AddString("protocol", protocol); // for identification when saving
			
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
			if ( fBox->CountChildren() > 1 )
				view->Hide();
			else
				fBox->SetLabel(protocol);
		};
	}
	
	
	// CLIENTS
	
	BMessage clients;
	im_get_client_list(&clients);
	
	clients.PrintToStream();
	
	if (clients.FindString("client")) {
		printf("Adding clients\n");
//		FIX ME: Find the location of the im_server programmatically (By app signature?)
		for (int16 i = 0; clients.FindString("client", i); i++) {
			const char *client = clients.FindString("client", i);
			printf("Adding client %s\n", client);
			entry_ref ref;
			//protocols.FindRef("ref", i, &ref);	
			
			BMessage client_settings;
			BMessage client_template;
			
			im_load_client_settings( client, &client_settings );
			im_load_client_template( client, &client_template );
			
			if ( client_settings.FindString("app_sig") ) {
				be_roster->FindApp( client_settings.FindString("app_sig"), &ref );
				printf("Client path: %s\n", BPath(&ref).Path() );
			}

			BBitmap *icon = ReadNodeIcon(BPath(&ref).Path(), kSmallIcon, true);
			printf("Loading icon from %s\n", BPath(&ref).Path());
			
			fListView->AddItem(new IconTextItem(client, icon));
			
			client_template.AddString("client", client); // for identification when saving
			
			pair <BMessage, BMessage> p(client_settings, client_template);
			fAddOns[client] = p; //pair<BMessage, BMessage>(settings, tmplate);
			
			BView *view = new BView(frame, client, B_FOLLOW_ALL_SIDES,
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

			BuildGUI(client_template, client_settings, view);
			fPrefView.AddItem(view);
			fBox->AddChild(view);
			if ( fBox->CountChildren() > 1 )
				view->Hide();
			else
				fBox->SetLabel(client);
		};
	}

	BScrollView *scroller = new BScrollView("list scroller", fListView, B_FOLLOW_LEFT |
		B_FOLLOW_BOTTOM, 0, false, true);
	fView->AddChild(scroller);

	fListView->Select(0);
	fListView->MakeFocus();

	frame = fView->Frame();
	frame.InsetBy(kEdgeOffset, kEdgeOffset);
	frame.bottom -= (kEdgeOffset * 2);
	frame.top = frame.bottom - ((fontHeight.descent + fontHeight.leading + fontHeight.ascent));
	frame.left = frame.right - (be_plain_font->StringWidth(_T("Save")) +
		(kControlOffset * 2));

	fSave = new BButton(frame, "Save", _T("Save"), new BMessage(SAVE));
	fView->AddChild(fSave);

	frame.right = frame.left - kControlOffset;
	frame.left = frame.right - (be_plain_font->StringWidth(_T("Revert")) +
		(kControlOffset * 2));

	fRevert = new BButton(frame, "Revert", _T("Revert"), new BMessage(REVERT));
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
//			BMessage outSettings(IM::SET_SETTINGS);
			BMessage cur;
			BMessage tmplate;
			BMessage settings;
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
			/*
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

			*/
			status_t res = B_ERROR;
			BMessage updMessage(IM::SETTINGS_UPDATED);
			
			if ( tmplate.FindString("protocol") )
			{
				res = im_save_protocol_settings( tmplate.FindString("protocol"), &settings );
				updMessage.AddString("protocol", tmplate.FindString("protocol") );
			} else
			if ( tmplate.FindString("client") )
			{
				res = im_save_client_settings( tmplate.FindString("client"), &settings );
				updMessage.AddString("client", tmplate.FindString("client") );
			} else
			{
				LOG("Preflet", liHigh, "Failed to determine type of settings");
			}
			
			if ( res != B_OK )
			{
				LOG("Preflet", liHigh, "Error when saving settings");
			} else
			{
				fManager->SendMessage( &updMessage );
			}
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
		
		if ( name != NULL && strcmp(name,"app_sig") == 0 ) {
			// skip app-sig setting
			continue;
		}
		
		if (curr.FindInt32("type", &type) != B_OK) {
			printf("Error getting type for %s, skipping\n", name);
			continue;
		};
		
		switch (type) {
			case B_STRING_TYPE: {
				if (curr.FindString("valid_value")) {
					// It's a "select one of these" setting
					
					freeText = false;
			
					menu = new BPopUpMenu(name);
//					menu->SetDivider(be_plain_font->StringWidth(name) + 10);
					
					for (int j = 0; curr.FindString("valid_value", j); j++) {
						menu->AddItem(new BMenuItem(curr.FindString("valid_value", j),NULL));
					};
					
					value = settings.FindString(name);
					
					if (value) menu->FindItem(value)->SetMarked(true);
				} else {
					// It's a free-text setting
					
					if (curr.FindBool("multi_line", &multiLine) != B_OK) multiLine = false;
					value = settings.FindString(name);
					if (!value) value = curr.FindString("default");
					if (curr.FindBool("is_secret",&secret) != B_OK) secret = false;
				}
			} break;
			case B_INT32_TYPE: {
				if (curr.FindInt32("valid_value")) {
					// It's a "select one of these" setting
					
					freeText = false;
					
					menu = new BPopUpMenu(name);
					
					int32 v = 0;
					for ( int j = 0; curr.FindInt32("valid_value",j,&v) == B_OK; j++ ) {
						sprintf(temp,"%ld", v);
						menu->AddItem(new BMenuItem(temp, NULL));
					};
				} else {
					// It's a free-text (but number) setting
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
					name, _T(desc), NULL);
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
						_T(desc), value, NULL);
					if (secret) {
						((BTextControl *)control)->TextView()->HideTyping(true);
						((BTextControl *)control)->SetText(_T(value));
					};
					((BTextControl *)control)->SetDivider(kDividerWidth);
				} else {
					BRect labelRect(0, 0, kDividerWidth, fFontHeight);
					BStringView *label = new BStringView(labelRect, "NA", _T(desc),
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
					textView->SetText(_T(value));			
				};
			} else {
				control = new BMenuField(BRect(0, 0, kControlWidth, fFontHeight),
					name, _T(desc), menu);
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
