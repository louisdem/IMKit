#include "ChatWindow.h"

#include "ImageButton.h"

#include "../../common/IMKitUtilities.h"

#include <libim/Contact.h>
#include <libim/Constants.h>
#include <libim/Helpers.h>
#include <Mime.h>
#include <Path.h>

const char *kImNewMessageSound = "IM Message Received";
const float kPadding = 2.0;

const char *kWidestButtonText = "Show Info";

float kButtonWidth = 50;
float kButtonHeight = 50;
float  kButtonDockHeight = 50;

ChatWindow::ChatWindow(entry_ref & ref, int32 iconBarSize = kLargeIcon, bool command = true)
:	BWindow( 
		BRect(100,100,400,300), 
		"unknown contact - unknown status", 
		B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AVOID_FOCUS
	),
	fEntry(ref),
	fMan( new IM::Manager(BMessenger(this))),
	fChangedNotActivated(false),
	fStatusBar(NULL)
{
	font_height height;
	be_plain_font->GetHeight(&height);
	fFontHeight = height.ascent + height.descent + height.leading;

	kButtonWidth = iconBarSize;
	float temp = be_plain_font->StringWidth(kWidestButtonText);

	if (temp > kButtonWidth) kButtonWidth = temp;
	kButtonWidth += kPadding * 2;

	kButtonHeight = iconBarSize + fFontHeight + (kPadding * 3);
	kButtonDockHeight = kButtonHeight + (kPadding * 3);

	BRect windowRect(100, 100, 400, 300);
	BPoint inputDivider(0, 150);

	if (LoadSettings() == B_OK) {
		bool was_ok = true;
		
		if (fWindowSettings.FindRect("windowrect", &windowRect) != B_OK) {
			was_ok = false;
		}
		if (fWindowSettings.FindPoint("inputdivider", &inputDivider) != B_OK) {
			was_ok = false;
		}
		
		if ( !was_ok )
		{
			windowRect = BRect(100, 100, 400, 300);
			inputDivider = BPoint(0, 200);
		}
	}
	
	if ( inputDivider.y > windowRect.Height() - 50 )
	{
		LOG("im_client", liLow, "Insane divider, fixed.");
		inputDivider.y = windowRect.Height() - 50;
	}
	
	MoveTo(windowRect.left, windowRect.top);
	ResizeTo(windowRect.Width(), windowRect.Height());

	// create views
	BRect textRect = Bounds();
	BRect inputRect = Bounds();
	BRect dockRect = Bounds();

	dockRect.bottom = kButtonDockHeight;
	fDock = new BView(dockRect, "Dock", B_FOLLOW_LEFT_RIGHT, 0);
#if B_BEOS_VERSION > B_BEOS_VERSION_5
	fDock->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	fDock->SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	fDock->SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
#else
	fDock->SetViewColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
	fDock->SetLowColor( ui_color(B_PANEL_BACKGROUND_COLOR) );
//	fDock->SetHighColor( ui_color(B_PANEL_TEXT_COLOR) );
#endif
	AddChild(fDock);

	// add buttons
	ImageButton * btn;
	BBitmap * icon;
	long err = 0;
	BPath iconDir;
	BPath iconPath;
	
	find_directory(B_USER_SETTINGS_DIRECTORY, &iconDir, true);
	iconDir.Append("im_kit/icons");
	
//	People icon
	iconPath = iconDir;
	iconPath.Append("People");
	icon = ReadNodeIcon(iconPath.Path(), iconBarSize);
	
	btn = new ImageButton(
		BRect(2,2,2+kButtonWidth,2+kButtonHeight),
		"open in people button",
		new BMessage(SHOW_INFO),
		B_FOLLOW_NONE,
		B_WILL_DRAW,
		icon,
		"Show info"
	);
	fDock->AddChild(btn);
	
	// email icon
	entry_ref emailAppRef;
	if ( be_roster->FindApp( "text/x-email", &emailAppRef ) != B_OK )
	{ // failed to get email icon, oopsie.
		LOG("im_client", liMedium, "Failed to get email app icon");
		emailAppRef = fEntry; // this isn't what we should be doing, but it might be better than nothing.
	}
	
	BPath emailPath(&emailAppRef);
	icon = ReadNodeIcon(emailPath.Path(), iconBarSize);

	btn = new ImageButton(
		btn->Frame().OffsetByCopy(kButtonWidth+1,0),
		"open in people button",
		new BMessage(EMAIL),
		B_FOLLOW_NONE,
		B_WILL_DRAW,
		icon,
		"E-mail"
	);
	fDock->AddChild(btn);
	
//	Block icon
	iconPath = iconDir;
	iconPath.Append("Block");

	icon = ReadNodeIcon(iconPath.Path(), iconBarSize, true);
	btn = new ImageButton(
		btn->Frame().OffsetByCopy(kButtonWidth+1,0),
		"email button",
		new BMessage(BLOCK),
		B_FOLLOW_NONE,
		B_WILL_DRAW,
		icon,
		"Block"
	);
	fDock->AddChild(btn);
	
	// Auth icon
	iconPath = iconDir;
	iconPath.Append("GetAuth");
	icon = ReadNodeIcon(iconPath.Path(), iconBarSize, true);
	
	btn = new ImageButton(
		btn->Frame().OffsetByCopy(kButtonWidth+1,0),
		"request_auth button",
		new BMessage(AUTH),
		B_FOLLOW_NONE,
		B_WILL_DRAW,
		icon,
		"Get auth"
	);
	fDock->AddChild(btn);

//	Done adding buttons
	
	textRect.top = fDock->Bounds().bottom+1;
	textRect.InsetBy(2,2);
	textRect.bottom = inputDivider.y;
	textRect.right -= B_V_SCROLL_BAR_WIDTH;
	
	inputRect.InsetBy(2.0, 2.0);
	inputRect.top = inputDivider.y + 5;
	inputRect.right -= B_V_SCROLL_BAR_WIDTH;
	inputRect.bottom -= fFontHeight + (kPadding * 4);
		
	BRect inputTextRect = inputRect;
	inputTextRect.OffsetTo(kPadding, kPadding);
	inputTextRect.InsetBy(kPadding * 2, kPadding * 2);
	
	fInput = new BTextView(inputRect, "input", inputTextRect, B_FOLLOW_ALL,
		B_WILL_DRAW);

#if B_BEOS_VERSION > B_BEOS_VERSION_5
	fInput->SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	fInput->SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	fInput->SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
#else
	fInput->SetViewColor(245, 245, 245, 0);
	fInput->SetLowColor(245, 245, 245, 0);
	fInput->SetHighColor(0, 0, 0, 0);
#endif

	fInputScroll = new BScrollView(
		"input_scroller", fInput,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM, 0,
		false,
		true
	);

	AddChild(fInputScroll);	

	fInput->SetWordWrap(true);
	fInput->SetStylable(false);
	fInput->MakeSelectable(true);
	
	BRect statusRect = Bounds();
	statusRect.top = inputRect.bottom + kPadding;

	fStatusBar = new StatusBar(statusRect);

	AddChild(fStatusBar);
#if B_BEOS_VERSION > B_BEOS_VERSION_5
	fStatusBar->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	fStatusBar->SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	fStatusBar->SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
#else
	fStatusBar->SetViewColor(245, 245, 245, 0);
	fStatusBar->SetLowColor(245, 245, 245, 0);
	fStatusBar->SetHighColor(0, 0, 0, 0);
#endif

	BPopUpMenu *pop = new BPopUpMenu("Templates", true, true);
	fProtocolMenu = new BMenuField(
		BRect(kPadding, kPadding, Bounds().bottom - kPadding, 100),
		"Field", NULL, pop);
	fStatusBar->AddItem(fProtocolMenu);
	BuildProtocolMenu();
	pop->ItemAt(0)->SetMarked(true);

	fTypingView = new BStringView(BRect(200, 2,
		fStatusBar->Bounds().right - kPadding,
		fStatusBar->Bounds().bottom - kPadding), "adf",
		"", B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM, B_WILL_DRAW);
	fStatusBar->AddItem(fTypingView);
	
	BRect resizeRect = Bounds();
	resizeRect.top = inputDivider.y + 1;
	resizeRect.bottom = inputDivider.y + 4;

	fResize = new ResizeView(fInputScroll, resizeRect, "resizer",
		B_FOLLOW_BOTTOM | B_FOLLOW_LEFT | B_FOLLOW_RIGHT);
	AddChild(fResize);

	fFilter = new InputFilter(fInput, new BMessage(SEND_MESSAGE), command);
	fInput->AddFilter((BMessageFilter *)fFilter);
	
	Theme::TimestampFore = C_TIMESTAMP_DUMMY;
	Theme::TimestampBack = C_TIMESTAMP_DUMMY;
	Theme::TimespaceFore = MAX_COLORS;
	Theme::TimespaceBack = MAX_COLORS;
	Theme::TimespaceFont = MAX_FONTS;
	Theme::TimestampFont = F_TIMESTAMP_DUMMY;
	Theme::NormalFore = C_TEXT;
	Theme::NormalBack = C_TEXT;
	Theme::NormalFont = F_TEXT;
	Theme::SelectionBack = C_SELECTION;
	
	fTheme = new Theme("ChatWindow", MAX_COLORS + 1, MAX_COLORS + 1, MAX_FONTS + 1);

	fTheme->WriteLock();
	fTheme->SetForeground(C_URL, 5, 5, 150);
	fTheme->SetBackground(C_URL, 255, 255, 255);
	fTheme->SetFont(C_URL, be_plain_font);
	
	fTheme->SetForeground(C_TIMESTAMP, 130, 130, 130);
	fTheme->SetBackground(C_TIMESTAMP, 255, 255, 255);
	fTheme->SetFont(F_TIMESTAMP, be_plain_font);

	fTheme->SetForeground(C_TEXT, 0, 0, 0);
	fTheme->SetBackground(C_TEXT, 255, 255, 255);
	fTheme->SetFont(F_TEXT, be_plain_font);
	
	fTheme->SetForeground(C_ACTION, 0, 0, 0);
	fTheme->SetBackground(C_ACTION, 255, 255, 255);
	fTheme->SetFont(F_ACTION, be_plain_font);
	
	fTheme->SetForeground(C_SELECTION, 255, 255, 255);
	fTheme->SetBackground(C_SELECTION, 0, 0, 0);

	fTheme->SetForeground(C_OWNNICK, 0, 0, 255);
	fTheme->SetBackground(C_OWNNICK, 255, 255, 255);
	
	fTheme->SetForeground(C_OTHERNICK, 255, 0, 0);
	fTheme->SetBackground(C_OTHERNICK, 255, 255, 255);

	fTheme->WriteUnlock();
	
	IM::Contact con(&fEntry);
	char id[256];
	con.ConnectionAt(0, id);

	fText = ((ChatApp *)be_app)->GetRunView(id);
	if (fText == NULL) {
		fText = new RunView(
			textRect, "text", fTheme,
			B_FOLLOW_ALL, B_WILL_DRAW
		);
	};
	
	fText->SetTimeStampFormat(NULL);

#if B_BEOS_VERSION > B_BEOS_VERSION_5
	fText->SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	fText->SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	fText->SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
#else
	fText->SetViewColor(245, 245, 245, 0);
	fText->SetLowColor(245, 245, 245, 0);
	fText->SetHighColor(0, 0, 0, 0);
#endif

	fTextScroll = new BScrollView(
		"scroller", fText,
		B_FOLLOW_ALL, 0,
		false, // horiz
		true // vert
	);
	AddChild(fTextScroll);
	fTextScroll->MoveTo(0,fDock->Bounds().bottom+1);
	
	fText->Show();
	fText->ScrollToBottom();

	fInput->MakeFocus();

	// monitor node so we get updates to status etc
	BEntry entry(&ref);
	node_ref node;
	
	entry.GetNodeRef(&node);
	watch_node( &node, B_WATCH_ALL, BMessenger(this) );
	
	// get contact info
	reloadContact();

	// set up timer for clearing typing view
	fTypingTimer = NULL;
//	startTypingTimer();
}

ChatWindow::~ChatWindow()
{
	stopNotify();
	SaveSettings();
	
	stop_watching( BMessenger(this) );

	if (fInput) {
		if (fInput->RemoveFilter(fFilter)) delete fFilter;
		fInput->RemoveSelf();
		
		delete fInput;
	};

	if (fResize) {
		fResize->RemoveSelf();
		delete fResize;
	};

	if (fDock) {
		fDock->RemoveSelf();
		delete fDock;
	};

	if (fStatusBar) fStatusBar->RemoveSelf();
	delete fStatusBar;

	fMan->Lock();
	fMan->Quit();
}

bool
ChatWindow::QuitRequested()
{
	if (fTextScroll != NULL) {
		fTextScroll->RemoveSelf();
//		Deleting the fTextScroll here causes a crash when you re open the window.
//		This makes Baby Mikey cry
//		delete fTextScroll;
	};

	fText->RemoveSelf();

	IM::Contact con(&fEntry);
	char id[256];
	con.ConnectionAt(0, id);
	((ChatApp *)be_app)->StoreRunView(id, fText);


	return true;
}

status_t
ChatWindow::SaveSettings(void) {
	if (fWindowSettings.ReplaceRect("windowrect", Frame()) != B_OK) {
		fWindowSettings.AddRect("windowrect", Frame());
	};
	
	BRect resizeRect = fResize->Frame();
	BPoint resize(0, resizeRect.top-1);
	
	if (fWindowSettings.ReplacePoint("inputdivider", resize) != B_OK) {
		fWindowSettings.AddPoint("inputdivider", resize);
	};

	ssize_t size = fWindowSettings.FlattenedSize();
	char *buffer = (char *)calloc(size, sizeof(char));

	if (fWindowSettings.Flatten(buffer, size) != B_OK) {
		LOG("im_client", liHigh, "Could not flatten window settings");
	} else {
		LOG("im_client", liLow, "Window settings flattened");
		BNode peopleNode(&fEntry);
		
		if (peopleNode.WriteAttr("IM:ChatSettings", B_MESSAGE_TYPE, 0, buffer, 
			(size_t)size) == size) {
			LOG("im_client", liLow, "Window Settings saved to disk");
		} else {
			LOG("im_client", liHigh, "Window settings could not be written to disk");
		};	
	};

	free(buffer);
}

status_t
ChatWindow::LoadSettings(void) {
	// read status
	
	BNode peopleNode(&fEntry);
	attr_info info;
	
	if (peopleNode.GetAttrInfo("IM:ChatSettings", &info) == B_OK) {	
		char *buffer = (char *)calloc(info.size, sizeof(char));
		
		if (peopleNode.ReadAttr("IM:ChatSettings", B_MESSAGE_TYPE, 0,
			buffer, (size_t)info.size) == info.size) {
			
			if (fWindowSettings.Unflatten(buffer) == B_OK) {
				return B_OK;
			} else {
				LOG("im_client", liLow, "Could not unflatten settings messsage");
				return B_ERROR;
			};

			free(buffer);
		} else {
			LOG("im_client", liLow, "Could not read chat attribute");
			return B_ERROR;
		};
	} else {
		LOG("im_client", liLow, "Could not load chat settings");
		return B_ERROR;
	};
	
	return B_OK;
}

void
ChatWindow::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case IM::USER_STOPPED_TYPING: {
			BMessage im_msg(IM::MESSAGE);
			im_msg.AddInt32("im_what",IM::USER_STOPPED_TYPING);
			im_msg.AddRef("contact",&fEntry);
			fMan->SendMessage(&im_msg);
		} break;
		case IM::USER_STARTED_TYPING: {
			BMessage im_msg(IM::MESSAGE);
			im_msg.AddInt32("im_what", IM::USER_STARTED_TYPING);
			im_msg.AddRef("contact", &fEntry);
			fMan->SendMessage(&im_msg);
		} break;
		case IM::DESKBAR_ICON_CLICKED:
		{ // deskbar icon clicked, move to current workspace and activate
			SetWorkspaces( 1 << current_workspace() );
			Activate();
		}	break;
		
		case IM::MESSAGE:
		{
			entry_ref contact;
			
			if ( msg->FindRef("contact",&contact) != B_OK )
				return;
				
			if ( contact != fEntry )
				return;
			
			int32 im_what=0;
			
			msg->FindInt32("im_what",&im_what);
				
			int32 old_sel_start, old_sel_end;
			
			char timestr[10];
			time_t now = time(NULL);
			strftime(timestr, sizeof(timestr),"[%H:%M]: ", localtime(&now) );
				
			switch ( im_what )
			{
				case IM::MESSAGE_SENT:
				{
					fText->Append(timestr, C_TIMESTAMP, C_TIMESTAMP, F_TIMESTAMP);

					BString message;
					msg->FindString("message", &message);
					if (message.Compare("/me ", 4) == 0) {
						fText->Append("* You ", C_ACTION, C_ACTION, F_ACTION);
						message.Remove(0, 4);
						fText->Append(message.String(), C_ACTION, C_ACTION, F_ACTION);
					} else {
						fText->Append("You say: ", C_OWNNICK, C_OWNNICK, F_TEXT);
						fText->Append(msg->FindString("message"), C_TEXT, C_TEXT, F_TEXT);
					}
					fText->Append("\n", C_TEXT, C_TEXT, F_TEXT);
					fText->ScrollToSelection();
				}	break;
				
				case IM::MESSAGE_RECEIVED:
				{
					if ( msg->FindString("message") == NULL )
					{ // no message to display, probably opened by user
						return;
					}
					
					fText->Append(timestr, C_TIMESTAMP, C_TIMESTAMP, F_TIMESTAMP);
					
					BString protocol = msg->FindString("protocol");
					BString message = msg->FindString("message");
					
					if (protocol.Length() > 0) {
						protocol.Prepend(" (");
						protocol << ") ";
					} else {
						protocol = " ";
					};
					
					if (message.Compare("/me ", 4) == 0) {
						fText->Append("* ", C_ACTION, C_ACTION, F_ACTION);
						fText->Append(fName, C_ACTION, C_ACTION, F_ACTION);
						fText->Append(protocol.String(), C_ACTION, C_ACTION, F_ACTION);
						message.Remove(0, 4);
						fText->Append(message.String(), C_ACTION, C_ACTION, F_ACTION);
					} else {
						fText->Append(fName, C_OTHERNICK, C_OTHERNICK, F_TEXT);
						fText->Append(protocol.String(), C_OTHERNICK, C_OTHERNICK, F_TEXT);
						fText->Append(": ", C_OTHERNICK, C_OTHERNICK, F_TEXT);
						fText->Append(msg->FindString("message"), C_TEXT, C_TEXT, F_TEXT);
					}
					fText->Append("\n", C_TEXT, C_TEXT, F_TEXT);
					fText->ScrollToSelection();

					if (!IsActive()) startNotify();
					
					stopTypingTimer();
				}	break;
				
				case IM::CONTACT_STARTED_TYPING: {	
					IM::Contact c(&fEntry);
	
/*					char nick[512];
					c.GetNickname(nick, sizeof(nick));
					
					BString text = nick;
					text << " is typing!";
					fTypingView->SetText(text.String());
					*/
					startTypingTimer();
				} break;
				
				case IM::CONTACT_STOPPED_TYPING: {
				//	fTypingView->SetText("");
					stopTypingTimer();
				} break;
				
			}
			
			fText->ScrollToSelection();
			
		}	break;
		
		case SEND_MESSAGE:
		{
			if (fInput->TextLength() == 0) return;
			BMessage im_msg(IM::MESSAGE);
			im_msg.AddInt32("im_what",IM::SEND_MESSAGE);
			im_msg.AddRef("contact",&fEntry);
			im_msg.AddString("message", fInput->Text() );
			
			BMenu *menu = fProtocolMenu->Menu();
			if (menu) {
				IconMenuItem *item = dynamic_cast<IconMenuItem*>(menu->FindMarked());
				if ( item )
				{
					BString protocol = item->Extra();
					if (protocol.Length() > 0) im_msg.AddString("protocol", protocol);
				}	
			};
			
			if ( fMan->SendMessage(&im_msg) == B_OK ) {
				fInput->SetText("");
			} else {
				LOG("im_client", liHigh, "Error sending message to im_server");
			};
		}	break;
		
		case SHOW_INFO:
		{
			BMessage open_msg(B_REFS_RECEIVED);
			open_msg.AddRef("refs", &fEntry);
			
			be_roster->Launch( "application/x-vnd.Be-PEPL", &open_msg );
		}	break;
		
		case EMAIL:
		{
			BMessage open_msg(B_REFS_RECEIVED);
			open_msg.AddRef("refs", &fEntry);
			
			be_roster->Launch( "application/x-vnd.Be-MAIL", &open_msg );
		}	break;
		
		case BLOCK:
		{
			IM::Contact contact(fEntry);
			
			char status[256];
			
			if ( contact.GetStatus( status, sizeof(status) ) != B_OK )
				status[0] = 0;
			
			if ( strcmp(status, BLOCKED_TEXT) == 0 )
			{ // already blocked, unblocked
				contact.SetStatus(OFFLINE_TEXT);
				
				BMessage update_msg(IM::UPDATE_CONTACT_STATUS);
				update_msg.AddRef("contact", &fEntry);
				
				fMan->SendMessage( &update_msg );
			} else
			{
				if ( contact.SetStatus(BLOCKED_TEXT) != B_OK )
				{
					LOG("im_client", liHigh, "Block: Error setting contact status");
				}
			}
		}	break;
		
		case AUTH:
		{
			BMessage auth_msg(IM::MESSAGE);
			auth_msg.AddInt32("im_what", IM::REQUEST_AUTH);
			auth_msg.AddRef("contact", &fEntry);
			
			fMan->SendMessage( &auth_msg );
		}	break;
		
		case B_NODE_MONITOR:
		{
			int32 opcode=0;
			
			if ( msg->FindInt32("opcode",&opcode) != B_OK )
				return;
			
			switch ( opcode )
			{
				case B_ENTRY_REMOVED:
					// oops. should we close down this window now?
					// Nah, we'll just disable everything.
					fInput->MakeEditable(false);
					break;
				case B_ENTRY_MOVED:
				{
					entry_ref ref;
					
					msg->FindInt32("device", &ref.device);
					msg->FindInt64("to directory", &ref.directory);
					ref.set_name( msg->FindString("name") );
					
					fEntry = ref;
					
					BEntry entry(&fEntry);
					if ( !entry.Exists() )
					{
						LOG("im_client", liHigh, "Entry moved: New entry invalid");
					}
				}	break;
				case B_STAT_CHANGED:
				case B_ATTR_CHANGED:
					reloadContact();
					BuildProtocolMenu();
					break;
			}
		}	break;
		case kResizeMessage: {
			BView *view = NULL;
			msg->FindPointer("view", reinterpret_cast<void**>(&view));
			if (dynamic_cast<BScrollView *>(view)) {
				BPoint point;
				msg->FindPoint("loc", &point);
				//printf("Point:\n");
				point.PrintToStream();
				//int rows = ceil((fTextScroll->Frame().Height() - point.y) / fFontHeight);
				//printf("Can have %i rows\n", rows);
				
				fResize->MoveTo(fResize->Frame().left, point.y);
//				fTextScroll->MoveTo(0,fDock->Bounds().bottom+1);
				fTextScroll->ResizeTo(fTextScroll->Frame().Width(), point.y - 1 - kButtonDockHeight - 1);

				fInputScroll->MoveTo(fInputScroll->Frame().left, point.y + 1);

//LOG("im_client", liLow, "%.2f - %.2f = %.2f", fInputScroll->Frame().top,
//	fStatusBar->Frame().top, fInputScroll->Frame().top - fStatusBar->Frame().top);
//				fInputScroll->ResizeTo(fInputScroll->Bounds().right,
//					fInputScroll->Frame().top - fStatusBar->Frame().top);
									
//					(point.y + fResize->Frame().bottom + 1) - fStatusBar->Frame().top);

//				fInputScroll->ResizeTo(fInputScroll->Bounds().right,
//					(point.y + fResize->Frame().bottom + 1) - fStatusBar->Frame().top);
//LOG("im_client", liLow, "(%.2f + 1) - %.2f = %.2f", point.y, fStatusBar->Frame().top,
//	(point.y + 1) - fStatusBar->Frame().top);

//
				fInputScroll->MoveTo(fInputScroll->Frame().left, point.y + 1);
				fInputScroll->ResizeTo(fInputScroll->Frame().Width(),
//					point.y  - fStatusBar->Frame().top);
					Bounds().bottom - point.y  - fStatusBar->Frame().top);

//				printf("status bar top: %.2f / %.2f\n", fStatusBar->Bounds().top, fStatusBar->Frame().top);
//				printf("Point: %.2f\n", point.y);
//				printf("Window: %2.f / %.2f\n", Bounds().bottom, Frame().bottom);
//				fInputScroll->ResizeTo(fInputScroll->Frame().Width(), Bounds().bottom - point.y);// - fStatusBar->Frame().top);
//				printf("Input: (%.2f, %.2f)\n", fInputScroll->Frame().top, fInputScroll->Frame().bottom);
//				printf("Input: (%.2f, %.2f)\n", fInputScroll->Bounds().top, fInputScroll->Bounds().bottom);
			};
		} break;
		
		case B_MOUSE_WHEEL_CHANGED: {
			fText->MessageReceived(msg);
		} break;
		
		case B_COPY: {
			int32 start = 0;
			int32 end = 0;
			
			fInput->GetSelection(&start, &end);
			
			printf("%i - > %i\n", start, end);
		} break;
		
		case B_SIMPLE_DATA: {
			entry_ref ref;
			BNode node;
			attr_info info;
			
			for (int i = 0; msg->FindRef("refs", i, &ref) == B_OK; i++) {
				node = BNode(&ref);
				
				char *type = ReadAttribute(node, "BEOS:TYPE");
				if (strcmp(type, "application/x-person") == 0) {
					char *name = ReadAttribute(node, "META:name");
					char *nickname = ReadAttribute(node, "META:nickname");
					char connection[100];
					IM::Contact con(ref);
					con.ConnectionAt(0, connection);

					if (fInput->TextLength() > 0) fInput->Insert("\n");
					fInput->Insert(name);
					fInput->Insert(" (");
					fInput->Insert(nickname);
					fInput->Insert("): ");
					fInput->Insert(connection);

					free(name);
					free(nickname);
				};
				free(type);
			};
			fInput->ScrollToOffset(fInput->TextLength());
		} break;
		
		case CLEAR_TYPING:
			//fTypingView->SetText("");
			stopTypingTimer();
			break;
		
		default:
			BWindow::MessageReceived(msg);
	}
}

void
ChatWindow::FrameResized( float w, float h )
{
	fText->ScrollToSelection();
	
	fInput->SetTextRect(fInput->Bounds());
	fInput->ScrollToSelection();
}

void
ChatWindow::WindowActivated(bool active)
{
	if (active) 
		stopNotify();
	
	BWindow::WindowActivated(active);
}

bool
ChatWindow::handlesRef( entry_ref & ref )
{
	return ( fEntry == ref );
}

void
ChatWindow::reloadContact()
{
	IM::Contact c(&fEntry);
	
	char status[512];
	char name[512];
	char nick[512];
	
	int32 num_read;
	
	// read name
	if ( c.GetName(name,sizeof(name)) != B_OK )
		strcpy(name,"Unknown name");
	
	if ( c.GetNickname(nick,sizeof(nick)) != B_OK )
		strcpy(nick,"no nick");
	
	sprintf(fName,"%s (%s)", name, nick );
	
	BNode node(&fEntry);
	
	// read status
	num_read = node.ReadAttr(
		"IM:status", B_STRING_TYPE, 0,
		status, sizeof(status)-1
	);
	
	if ( num_read <= 0 )
		strcpy(status,"Unknown status");
	else
		status[num_read] = 0;
	
	// rename window
	sprintf(fTitleCache,"%s - %s", fName, status);
	
	if ( !fChangedNotActivated )
	{
		SetTitle(fTitleCache);
	} else
	{
		char str[512];
		sprintf(str, "√ %s", fTitleCache);
		SetTitle(str);
	}
}

void
ChatWindow::startNotify()
{
	if ( fChangedNotActivated )
		return;
	
	fChangedNotActivated = true;
	char str[512];
	sprintf(str, "√ %s", fTitleCache);
	SetTitle(str);
	
	((ChatApp*)be_app)->Flash( BMessenger(this) );
	
	if ( (Workspaces() & (1 << current_workspace())) == 0)
	{
		// beep if on other workspace
		system_beep(kImNewMessageSound);
	}
}

void
ChatWindow::stopNotify()
{
	if ( !fChangedNotActivated )
		return;
	
	fChangedNotActivated = false;
	SetTitle(fTitleCache);
	((ChatApp*)be_app)->NoFlash( BMessenger(this) );
}


void ChatWindow::BuildProtocolMenu(void) {
	BMessage getStatus(IM::GET_CONTACT_STATUS);
	getStatus.AddRef("contact", &fEntry);
	BMessage statusMsg;
	
	fMan->SendMessage(&getStatus, &statusMsg);

	BPath iconDir;
	find_directory(B_USER_ADDONS_DIRECTORY, &iconDir, true);
	iconDir.Append("im_kit/protocols");

	BMenu *menu = fProtocolMenu->Menu();
	if (menu == NULL) return;
	
//	You have to do this twice... buggered if I know why...
	for (int32 i = 0; i < menu->CountItems(); i++) delete menu->RemoveItem(0L);
	for (int32 i = 0; i < menu->CountItems(); i++) delete menu->RemoveItem(0L);

	menu->AddItem(new IconMenuItem(NULL, "Any Protocol", NULL, NULL));
	menu->AddSeparatorItem();

	for (int32 i = 0; statusMsg.FindString("connection", i); i++) {
		BString connection = statusMsg.FindString("connection", i);
		BString status = statusMsg.FindString("status", i);
		BString protocol = "";
		int32 seperator = connection.FindFirst(":");
		if (seperator != B_ERROR) {
			connection.MoveInto(protocol, 0, seperator + 1);
			protocol.Truncate(seperator);
			
			BString iconPath = iconDir.Path();
			iconPath << "/" << protocol.String();
			
			BBitmap *icon = ReadNodeIcon(iconPath.String(), kSmallIcon, true);
			BString label = connection;
			label << " (" << status << ")";
			
			menu->AddItem(new IconMenuItem(icon, label.String(), protocol.String()));
		};
	};
	
	menu->SetFont(be_plain_font);
};

void
ChatWindow::startTypingTimer()
{
	if ( fTypingTimer )
		delete fTypingTimer;
	
	BMessage clearTyping(CLEAR_TYPING);
	fTypingTimer = new BMessageRunner( BMessenger(this), &clearTyping, 7*1000*1000, 1 );
	if ( fTypingTimer->InitCheck() != B_OK )
		LOG("im_client", liHigh, "InitCheck fail on typing timer");
	
	fTypingView->SetText("User is typing..");
}

void
ChatWindow::stopTypingTimer()
{
	fTypingView->SetText("");
	
	if ( fTypingTimer )
		delete fTypingTimer;
	
	fTypingTimer = NULL;
}
