#include "ChatWindow.h"

const char *kImNewMessageSound = "IM Message Received";

ChatWindow::ChatWindow( entry_ref & ref )
:	BWindow( 
		BRect(100,100,400,300), 
		"unknown contact - unknown status", 
		B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AVOID_FOCUS
	),
	fEntry(ref),
	fMan( new IM::Manager(BMessenger(this))),
	fChangedNotActivated(false)
{
	BRect windowRect(100, 100, 400, 300);
	BPoint inputDivider(0, 150);

	if (LoadSettings() != B_OK) {
		fWindowSettings.AddRect("windowrect", BRect(100, 100, 400, 400));
		fWindowSettings.AddPoint("inputdivider", BPoint(0, 300));
	} else {
		if (fWindowSettings.FindRect("windowrect", &windowRect) != B_OK) {
			windowRect = BRect(100, 100, 400, 300);
		};
		if (fWindowSettings.FindPoint("inputdivider", &inputDivider) != B_OK) {
			inputDivider = BPoint(0, 150);
		};
	};

	
	MoveTo(windowRect.left, windowRect.top);
	ResizeTo(windowRect.Width(), windowRect.Height());

	// create views
	BRect textRect = Bounds();
	BRect text2Rect;
	BRect inputRect = Bounds();

	textRect.InsetBy(2,2);
	textRect.bottom = inputDivider.y;
	textRect.right -= B_V_SCROLL_BAR_WIDTH;
	
	text2Rect = textRect;
	text2Rect.OffsetTo(0,0);
	
	inputRect.InsetBy(2.0, 2.0);
	inputRect.top = inputDivider.y + 5;
	inputRect.right -= B_V_SCROLL_BAR_WIDTH;

	BRect inputTextRect = inputRect;
	inputTextRect.OffsetTo(0, 0);

	fInput = new BTextView(
		inputRect, "input", inputTextRect,
		B_FOLLOW_ALL,
		B_WILL_DRAW
	);
	fInput->SetWordWrap(true);
	fInput->SetStylable(false);
	fInput->MakeSelectable(true);

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

	BRect resizeRect = Bounds();
	resizeRect.top = inputDivider.y + 1;
	resizeRect.bottom = inputDivider.y + 4;

	fResize = new ResizeView(fInputScroll,resizeRect);
	AddChild(fResize);

	fFilter = new InputFilter(fInput, new BMessage(SEND_MESSAGE));
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
	
	fText = new RunView(
		textRect, "text", fTheme,
		B_FOLLOW_ALL, B_WILL_DRAW
	);
	
//	fText->SetTimeStampFormat("[%H:%M]");
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

fText->Show();

	fInput->MakeFocus();

	// monitor node so we get updates to status etc
	BEntry entry(&ref);
	node_ref node;
	
	entry.GetNodeRef(&node);
	watch_node( &node, B_WATCH_ALL, BMessenger(this) );
	
	// get contact info
	reloadContact();
}

ChatWindow::~ChatWindow()
{
	SaveSettings();
	
	stop_watching( BMessenger(this) );

	if (fInput) {
		if (fInput->RemoveFilter(fFilter)) delete fFilter;
		fInput->RemoveSelf();
		
		delete fInput;
	};
	if (fText) {
		fText->RemoveSelf();
		delete fText;
	};

	if (fResize) {
		fResize->RemoveSelf();
		delete fResize;
	};

	fMan->Lock();
	fMan->Quit();
}

bool
ChatWindow::QuitRequested()
{
/*
	if ( ((ChatApp*)be_app)->IsQuiting() )
		return true;
	
	Minimize(true);
	
	return false;
*/
	return true;
}

status_t
ChatWindow::SaveSettings(void) {
	if (fWindowSettings.ReplaceRect("windowrect", Frame()) != B_OK) {
		fWindowSettings.AddRect("windowrect", Frame());
	};
	
	BRect resizeRect = fResize->Frame();
	BPoint resize(0, resizeRect.top);
	
	if (fWindowSettings.ReplacePoint("inputdivider", resize) != B_OK) {
		fWindowSettings.AddPoint("inputdivider", resize);
	};

	ssize_t size = fWindowSettings.FlattenedSize();
	char *buffer = (char *)calloc(size, sizeof(char));

	if (fWindowSettings.Flatten(buffer, size) != B_OK) {
		LOG("sample_client", LOW, "Could not flatten window settings");
	} else {
		LOG("sample_client", LOW, "Window settings flattened");
		BNode peopleNode(&fEntry);
		
		if (peopleNode.WriteAttr("IM:ChatSettings", B_MESSAGE_TYPE, 0, buffer, 
			(size_t)size) == size) {
			LOG("sample_client", LOW, "Window Settings saved to disk");
		} else {
			LOG("sample_client", LOW, "WIndow settings could not be written to disk");
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
				LOG("sample_client", LOW, "Could not unflatten settings messsage");
				return B_ERROR;
			};

			free(buffer);
		} else {
			LOG("sample_client", LOW, "Could not read chat attribute");
			return B_ERROR;
		};
	} else {
		LOG("sample_client", LOW, "Could not load chat settings");
		return B_ERROR;
	};
	
	return B_OK;
}

void
ChatWindow::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
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
			
			if ( msg->FindString("message") == NULL )
				return;
			
			// this message is related to our Contact
			
			int32 im_what=0;
			
			msg->FindInt32("im_what",&im_what);
				
			int32 old_sel_start, old_sel_end;
			
//			fText->GetSelection(&old_sel_start, &old_sel_end);
//			fText->Select(fText->TextLength(),fText->TextLength());

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
//					fText->ScrollToSelection();
				}	break;
				
				case IM::MESSAGE_RECEIVED:
				{
					fText->Append(timestr, C_TIMESTAMP, C_TIMESTAMP, F_TIMESTAMP);

					BString message;
					msg->FindString("message", &message);
					if (message.Compare("/me ", 4) == 0) 
					{
						fText->Append("* ", C_ACTION, C_ACTION, F_ACTION);
						fText->Append(fName, C_ACTION, C_ACTION, F_ACTION);
						fText->Append(" ", C_ACTION, C_ACTION, F_ACTION);
						message.Remove(0, 4);
						fText->Append(message.String(), C_ACTION, C_ACTION, F_ACTION);
					} else 
					{
						fText->Append(fName, C_OTHERNICK, C_OTHERNICK, F_TEXT);
						fText->Append(": ", C_OTHERNICK, C_OTHERNICK, F_TEXT);
						fText->Append(msg->FindString("message"), C_TEXT, C_TEXT, F_TEXT);
					}
					fText->Append("\n", C_TEXT, C_TEXT, F_TEXT);
//					fText->ScrollToSelection();

					if (!IsActive()) 
					{
						startNotify();
					}
				}	break;
			}
			
			if ( old_sel_start != old_sel_end )
			{ // restore selection
//				fText->Select( old_sel_start, old_sel_end );
			} else
			{
//				fText->Select( fText->TextLength(), fText->TextLength() );
			}
			
//			fText->ScrollToSelection();
			
		}	break;
		
		case SEND_MESSAGE:
		{
			if (fInput->TextLength() == 0) return;
			BMessage im_msg(IM::MESSAGE);
			im_msg.AddInt32("im_what",IM::SEND_MESSAGE);
			im_msg.AddRef("contact",&fEntry);
			im_msg.AddString("message", fInput->Text() );
			
			if ( fMan->SendMessage(&im_msg) == B_OK ) {
				fInput->SetText("");
			} else {
				LOG("sample_client", LOW, "Error sending message to im_server");
			};
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
						LOG("sample_client", LOW, "Error: New entry invalid");
					}
				}	break;
				case B_STAT_CHANGED:
				case B_ATTR_CHANGED:
					reloadContact();
					break;
			}
		}	break;
		case kResizeMessage: {
			BView *view = NULL;
			msg->FindPointer("view", reinterpret_cast<void**>(&view));
			if (dynamic_cast<BScrollView *>(view)) {
				BPoint point;
				msg->FindPoint("loc", &point);

				fResize->MoveTo(fResize->Frame().left, point.y);
				fTextScroll->ResizeTo(fTextScroll->Frame().Width(), point.y - 1);

				fInputScroll->MoveTo(fInputScroll->Frame().left, point.y + 1);
				fInputScroll->ResizeTo(fInputScroll->Frame().Width(), Bounds().bottom - point.y);

			};
		} break;
		default:
			BWindow::MessageReceived(msg);
	}
}

void
ChatWindow::FrameResized( float w, float h )
{
//	fText->SetTextRect( fText->Bounds() );
//	fText->ScrollToSelection();
	
//	fInput->SetTextRect(fInput->Bounds());
//	fInput->ScrollToSelection();
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
