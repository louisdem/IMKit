#include "InfoView.h"
#include "InfoWindow.h"
#include "../../common/IMKitUtilities.h"

#include <TranslationUtils.h>
#include <StringView.h>
#include <Window.h>
#include <Messenger.h>
#include <stdio.h>
#include <PropertyInfo.h>
#include <Font.h>

const float kEdgePadding = 2.0;
const float kCloseWidth = 10.0;
const float kWidth = 300.0f;

//InfoView::infoview_layout gLayout = InfoView::AllTextRightOfIcon;
InfoView::infoview_layout gLayout = InfoView::TitleAboveIcon;

// 
property_info message_prop_list[] = {
	{ "content", {B_GET_PROPERTY, 0},{B_DIRECT_SPECIFIER, 0}, "get a message"},
	{ "title", {B_GET_PROPERTY, 0},{B_DIRECT_SPECIFIER, 0}, "get a message"},
	{ "icon", {B_GET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get icon as a flattened bitmap"},
	0 // terminate list
};

InfoView::InfoView( info_type type, const char *app, const char *title,
	const char * text, BMessage *details)
:	BView( BRect(0,0,kWidth,1), "InfoView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_FRAME_EVENTS ),
	fType(type),
	fRunner(NULL),
	fDetails(details) {
	
	BMessage iconMsg;
	if (fDetails->FindMessage("icon", &iconMsg) == B_OK) {
		fBitmap = new BBitmap(&iconMsg);
	} else {
		fBitmap = NULL;
		
		int32 iconType;
		if ( fDetails->FindInt32("iconType", &iconType) != B_OK )
			iconType = Attribute;
		
		entry_ref ref;
		
		if ( fDetails->FindRef("iconRef",&ref) == B_OK )
		{ // It's a ref.
			BPath path(&ref);
			
			switch ( iconType )
			{
				case Attribute: {
					fBitmap = ReadNodeIcon(path.Path(),16);
				}	break;
				case Contents: {
					// ye ol' "create a bitmap from contens of file"
					fBitmap = BTranslationUtils::GetBitmapFile(path.Path());
					if ( !fBitmap ) {
						printf("Error reading bitmap\n");
					} else {
						// we should re-scale the bitmap to our preferred size here, I suppose
					}
				}	break;
				default:
					// Eek! Invalid icon type!
					break;
			}
		}
	};
	
	const char * messageID = NULL;
	if (fDetails->FindString("messageID", &messageID) != B_OK) {
		fMessageID = ""; 
	} else {
		fMessageID = messageID;
	}
	if (fDetails->FindFloat("progress", &fProgress) != B_OK) fProgress = 0.0;
	if (fDetails->FindInt32("timeout", &fTimeout) != B_OK) fTimeout = 5;
	
	SetText(app, title, text);
	ResizeToPreferred();
	
	switch (type) {
		case InfoPopper::Information: {
			SetViewColor(218,218,218);
		} break;
		case InfoPopper::Important: {
			SetViewColor(255,255,255);
		} break;
		case InfoPopper::Error: {
			SetViewColor(255,0,0);
		} break;
		case InfoPopper::Progress: {
			SetViewColor(218,218,218);
		} break;
	}
}

InfoView::~InfoView(void) {
	if (fRunner) delete fRunner;
	if (fDetails) delete fDetails;
	if (fBitmap) delete fBitmap;

	vline::iterator lIt;
	for (lIt = fLines.begin(); lIt != fLines.end(); lIt++) delete (*lIt);
}

void
InfoView::AttachedToWindow()
{
	BMessage msg(REMOVE_VIEW);
	msg.AddPointer("view", this);
	
	bigtime_t delay = fTimeout*1000*1000;
	
	if ( delay > 0 )
		fRunner = new BMessageRunner( BMessenger(Window()), &msg, delay, 1 );
}

void InfoView::MessageReceived(BMessage * msg) {
	switch (msg->what) {
		case B_GET_PROPERTY: {
			BMessage specifier;
			const char * property;
			
			if (msg->FindMessage("specifiers", 0, &specifier) != B_OK) return;
			if (specifier.FindString("property", &property) != B_OK) return;
			
			BMessage reply(B_REPLY);
			
			if (strcmp(property, "content") == 0) {
				reply.AddString("result", fText);
			};
			
			if (strcmp(property, "title") == 0)  {
				reply.AddString("result", fTitle);
			};
			
			if (strcmp(property, "icon") == 0) {
/*				if ( fBitmap )
				{
					int32 bitmap_size = fBitmap->FlattenedSize();
					char * bitmap_data = malloc( bitmap_size );
					if ( fBitmap->Flatten(bitmap_data, bitmap_size) == B_OK )
						reply.AddData("result", bitmap_data, bitmap_size);
				}
*/			};

			msg->SendReply(&reply);
		} break;
		
		case B_SET_PROPERTY: {
			BMessage specifier;
			const char * property;
			
			if (msg->FindMessage("specifiers", 0, &specifier) != B_OK) return;
			if (specifier.FindString("property", &property) != B_OK) return;
			
			BMessage reply(B_REPLY);
			
			if (strcmp(property, "content") == 0) {
			};
			
			if (strcmp(property, "title") == 0) {
			};
			
			if (strcmp(property, "icon") == 0) {
			};

			msg->SendReply(&reply);
		}	break;
		
		case REMOVE_VIEW: {
			BMessage remove(REMOVE_VIEW);
			remove.AddPointer("view", this);
			BMessenger msgr(Window());
			msgr.SendMessage( &remove );
		} break;
		default: {
			BView::MessageReceived(msg);
		};
	};
};

void InfoView::GetPreferredSize(float *w, float *h) {
	*w = kWidth;
	*h = fHeight;
};

void InfoView::Draw(BRect drawBounds) {
	BRect bound = Bounds();
	
//	printf("Drawing. Width is %.0f pixels\n", Bounds().Width());

	// draw progress background
	if (fProgress > 0.0) {
		bound.right *= fProgress;
		
		SetHighColor(0x88, 0xff, 0x88);
		FillRect(bound);
		SetHighColor(0, 0, 0);
	}
	
	SetDrawingMode( B_OP_ALPHA );
	
	// draw icon
	if (fBitmap) {
		font_height fh;
		be_plain_font->GetHeight( &fh );
		
		float title_bottom = fh.ascent + fh.leading + fh.descent;
		float icon_right = kEdgePadding + fBitmap->Bounds().right + kEdgePadding;
		
		float ix = kEdgePadding;
		float iy = 0;
		if (gLayout == TitleAboveIcon) {
			iy = kEdgePadding + title_bottom + (Bounds().Height() - title_bottom - fBitmap->Bounds().Height()) / 2;
		} else {
			iy = (Bounds().Height() - fBitmap->Bounds().Height()) / 2.0;
		};
		
		DrawBitmap(fBitmap,	BPoint(ix,iy));
	}
	
//	Draw content
	vline::iterator lIt;
	for (lIt = fLines.begin(); lIt != fLines.end(); lIt++) {
		lineinfo *l = (*lIt);
		
		SetFont(&l->font);
		DrawString(l->text.String(), l->text.Length(), l->location);
	};
	
	// draw 'close rect'
	BRect closeRect = Bounds().InsetByCopy(2,2);
	closeRect.left = closeRect.right - kCloseWidth;
	closeRect.bottom = closeRect.top + kCloseWidth;
	SetHighColor(218, 218, 218);
	FillRect(closeRect);
	
	closeRect.right++;
	SetHighColor(0, 0, 0);
	StrokeRect(closeRect);

	BRect crossRect;
	float midY = (closeRect.bottom - closeRect.top) / 2;
	float midX = (closeRect.right - closeRect.left) / 2;
	crossRect.left = closeRect.left + midX - kEdgePadding;
	crossRect.right = closeRect.left + midX + kEdgePadding;
	crossRect.top = closeRect.top + midY - kEdgePadding;
	crossRect.bottom = closeRect.top + midY + kEdgePadding;

	StrokeLine(crossRect.LeftTop(), crossRect.RightBottom());
	StrokeLine(crossRect.LeftBottom(), crossRect.RightTop());
}

void InfoView::MouseDown(BPoint point) {
	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	
	switch (buttons) {
		case B_PRIMARY_MOUSE_BUTTON: {
			BRect closeRect;
			closeRect = Bounds();
			closeRect.left = closeRect.right - kCloseWidth;
			
			if (closeRect.Contains(point) == false) {		
				entry_ref launchRef;
				BString launchString;
				BMessage argMsg(B_ARGV_RECEIVED);
				BMessage refMsg(B_REFS_RECEIVED);
				entry_ref appRef;
				bool useArgv = false;
				BList messages;
				entry_ref ref;
				
				if (fDetails->FindString("onClickApp", &launchString) == B_OK) {
					if (be_roster->FindApp(launchString.String(), &appRef) == B_OK) useArgv = true;
				};
				if (fDetails->FindRef("onClickFile", &launchRef) == B_OK) {
					if (be_roster->FindApp(&launchRef, &appRef) == B_OK) useArgv = true;
				};
				
				if (fDetails->FindRef("onClickRef", &ref) == B_OK) {			
					for (int32 i = 0; fDetails->FindRef("onClickRef", i, &ref) == B_OK; i++) {
						refMsg.AddRef("refs", &ref);
					};
					
					messages.AddItem((void *)&refMsg);
				};
				
				if (useArgv == true) {
					type_code type;
					int32 argc = 0;
					BString arg;
					
					BPath p(&appRef);
					argMsg.AddString("argv", p.Path());
					
					fDetails->GetInfo("onClickArgv", &type, &argc);
					argMsg.AddInt32("argc", argc + 1);
					
					for (int32 i = 0; fDetails->FindString("onClickArgv", i, &arg) == B_OK;
						i++) {
		
						argMsg.AddString("argv", arg);
					};
					
					messages.AddItem((void *)&argMsg);
				};
				
				BMessage *tmp;
				for (int32 i = 0; fDetails->FindMessage("onClickMsg", i, tmp) == B_OK; i++) {
					messages.AddItem((void *)tmp);
				};
				
				if (fDetails->FindString("onClickApp", &launchString) == B_OK) {
					be_roster->Launch(launchString.String(), &messages);
				} else {
					be_roster->Launch(&launchRef, &messages);
				};
			};
			
			// remove the info view after a click
			BMessage remove_msg(REMOVE_VIEW);
			remove_msg.AddPointer("view", this);
			
			BMessenger msgr(Window());
			msgr.SendMessage(&remove_msg);
		} break;
	};
};

void InfoView::SetText(const char *app, const char *title, const char *text, float newMaxWidth) {
	if ( newMaxWidth < 0 )
		newMaxWidth = Bounds().Width();
	
//	printf("Setting new text, wrapping to %.0f pixels\n", newMaxWidth);
	
	// delete old lines
	vline::iterator lIt;
	for (lIt = fLines.begin(); lIt != fLines.end(); lIt++) delete (*lIt);
	fLines.clear();
	
	// do the text thing
	fApp = app;
	fTitle = title;
	fText = text;

	font_height fh;
	float fontHeight = 0;
	float iconRight = kEdgePadding + (fBitmap != NULL ? fBitmap->Bounds().right : 0) + kEdgePadding;
	float y = kEdgePadding;

	lineinfo *appLine = new lineinfo;
	be_bold_font->GetHeight(&fh);
	fontHeight = fh.leading + fh.descent + fh.ascent;
	y += fontHeight;
	if (gLayout == AllTextRightOfIcon) {
		appLine->location = BPoint(iconRight, y);
	} else {
		appLine->location = BPoint(kEdgePadding, y);
	};
	appLine->text = app;
	appLine->font = be_bold_font;
	fLines.push_front(appLine);
	y += fontHeight;
	
	be_plain_font->GetHeight(&fh);
	fontHeight = fh.leading + fh.descent + fh.ascent;
	
	lineinfo *titleLine = new lineinfo;
	titleLine->location = BPoint(iconRight + kEdgePadding, y);
	titleLine->font = be_plain_font;
	titleLine->text = title;
	fLines.push_front(titleLine);
	y += fontHeight;
	
	const char spacers[] = " \t\n-\\/";
	BString textBuffer = text;
	textBuffer.ReplaceAll("\t", "    ");
	text = textBuffer.String();
	
	size_t offset = 0;
	size_t n = 0;
	int16 count = 0;
	int16 length = strlen(text);
	int16 *spaces = NULL;;
	int16 index = 0;
	
	while ((n = strcspn(text + offset, spacers)) < (length - offset)) {
		++count;
		offset += n + 1;
	};
	
	spaces = (int16 *)calloc(count, sizeof(int16));
	offset = 0;
	
	while ((n = strcspn(text + offset, spacers)) < (length - offset)) {
		spaces[index++] = n + offset;
		offset += n + 1;
	};
	
	offset = 0;
	float maxWidth = newMaxWidth - kEdgePadding; //kWidth;
	bool wasNewline = false;
	
	for (int32 i = 0; i < count; i++) {
		if (text[spaces[i]] == '\n') {
			lineinfo *tempLine = new lineinfo;
			tempLine->text = "";
			tempLine->text.Append(text + offset, spaces[i] - offset);
			wasNewline = true;
			tempLine->font = be_plain_font;
			tempLine->location = BPoint(iconRight + kEdgePadding, y);
			y += fontHeight;
			
			offset = spaces[i] + 1;
			
/*			// debug
			printf("Line (newline) '%s' is %.0f pixels\n", 
				tempLine->text.String(), 
				tempLine->font.StringWidth(tempLine->text.String())
			);
*/			
			fLines.push_front(tempLine);
		} else if (i+1 < count && StringWidth(text + offset, spaces[i+1] - offset) > maxWidth) {
			lineinfo *tempLine = new lineinfo;
			tempLine->font = be_plain_font;
			if (wasNewline == false) {
				tempLine->location = BPoint(iconRight + (kEdgePadding * 2), y);
			} else {
				tempLine->location = BPoint(iconRight, y);
				wasNewline = false;
			};
			
			y += fontHeight;
			tempLine->text = "";
			tempLine->text.Append(text + offset, spaces[i] - offset);
			
			// strip first space
			if ( (tempLine->text.String())[0] == ' ' )
				tempLine->text.RemoveFirst(" ");
			
/*			// debug
			printf("Line (too long) '%s' is %.0f pixels\n", 
				tempLine->text.String(), 
				tempLine->font.StringWidth(tempLine->text.String())
			);
*/			
			fLines.push_front(tempLine);
			
			offset = spaces[i];
		};
	};
	
	lineinfo *tempLine = new lineinfo;
	tempLine->text = "";
	tempLine->text.Append(text + offset, strlen(text) - offset);
	tempLine->font = be_plain_font;
	if (wasNewline) {
		tempLine->location = BPoint(iconRight, y);
	} else {
		tempLine->location = BPoint(iconRight + (kEdgePadding * 2), y);
	};
	
	// strip first space
	if ( (tempLine->text.String())[0] == ' ' )
		tempLine->text.RemoveFirst(" ");
			
/*	// debug
	printf("Line (no more text) '%s' is %.0f pixels\n", 
		tempLine->text.String(), 
		tempLine->font.StringWidth(tempLine->text.String())
	);
*/
	fLines.push_front(tempLine);
	
	free(spaces);
	
	fHeight = y + kEdgePadding;
	
	BMessenger msgr(Parent());
	msgr.SendMessage( InfoWindow::ResizeToFit );
};

bool
InfoView::HasMessageID( const char * id )
{
	return fMessageID == id;
}

BHandler * InfoView::ResolveSpecifier(BMessage *msg, int32 index, BMessage *spec, int32 form, const char *prop) {
	BPropertyInfo prop_info(message_prop_list);
	if (prop_info.FindMatch(msg, index, spec, form, prop) >= 0) {
		msg->PopSpecifier();
		return this;
	}
	return BView::ResolveSpecifier(msg, index, spec, form, prop);
};

status_t InfoView::GetSupportedSuites(BMessage *msg) {
	msg->AddString("suites", "suite/x-vnd.beclan.InfoPopper-message");
	BPropertyInfo prop_info(message_prop_list);
	msg->AddFlat("messages", &prop_info);
	return BView::GetSupportedSuites(msg); 		
};

void InfoView::FrameResized( float w, float h ) {
	// SetText again to re-wrap lines to new view width
	BString app(fApp), title(fTitle), text(fText);
	
	SetText( 
		app.Length() > 0 ? app.String() : NULL, 
		title.Length() > 0 ? title.String() : NULL, 
		text.Length() > 0 ? text.String() : NULL,
		w
	);
}
