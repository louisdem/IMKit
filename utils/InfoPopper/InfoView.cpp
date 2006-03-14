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
#include <Region.h>

//#pragma mark Constants

property_info message_prop_list[] = {
	{ "content", {B_GET_PROPERTY, B_SET_PROPERTY, 0},{B_DIRECT_SPECIFIER, 0}, "get message contents"},
	{ "title", {B_GET_PROPERTY, B_SET_PROPERTY, 0},{B_DIRECT_SPECIFIER, 0}, "get message title"},
	{ "apptitle", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get message's app"},
	{ "icon", {B_GET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get icon as an archived bitmap"},
	{ "iconRef", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get / set icon entry_ref"},
	{ "iconType", {B_GET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get  icon type (Attribute / Contents)"},
	{ "overlayIconRef", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get / set overlay icon entry_ref"},
	{ "overlayIconType", {B_GET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get overlay icon type (Attribute / Contents)" },
	{ "type", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get the message type"},
	{ "progress", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get the progress (0.0-1.0)"},
	NULL // terminate list
};

//#pragma mark Constructor

InfoView::InfoView(InfoWindow *win, info_type type,
	const char *app, const char *title, const char * text, BMessage *details)

:	BView( BRect(0,0,win->ViewWidth(),1), "InfoView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_FRAME_EVENTS ),

	fParent(win),
	fType(type),
	fRunner(NULL),
	fDetails(details),
	fBitmap(NULL),
	fIsFirst(false),
	fIsLast(false) {
	
	int16 iconSize = fParent->IconSize();
	
	fBitmap = ExtractIcon("icon", fDetails, iconSize);
	fOverlayBitmap = ExtractIcon("overlayIcon", fDetails, iconSize / 4);
	
	if (fBitmap == NULL) {
		app_info info;
		BPath path;
		BMessenger msgr = details->ReturnAddress();

		if (msgr.IsValid() == true) {
			be_roster->GetRunningAppInfo(msgr.Team(), &info);
		} else {
			be_roster->GetAppInfo("application/x-vnd.Be-SHEL", &info);
		};

		path.SetTo(&info.ref);
		
		fDetails->AddRef("iconRef", &info.ref);
		fDetails->AddInt32("iconType", Attribute);
		
		fBitmap = ReadNodeIcon(path.Path(), iconSize);
	};
	
	if (fOverlayBitmap == NULL) {
		BPath basepath = BPath(fParent->SettingsPath());
		basepath.Append("icons");

		switch (type) {
			case Information: basepath.Append("Information"); break;
			case Important: basepath.Append("Important"); break;
			case Error: basepath.Append("Error"); break;
			case Progress: basepath.Append("Progress"); break;
			default: basepath.Append("Information"); 
		};
				
		entry_ref overlayRef;
		get_ref_for_path(basepath.Path(), &overlayRef);
		fDetails->AddRef("overlayIconRef", &overlayRef);
		fDetails->AddInt32("overlayIconType", Attribute);

		fOverlayBitmap = ReadNodeIcon(basepath.Path(), iconSize / 4);
	};
	
	const char * messageID = NULL;
	if (fDetails->FindString("messageID", &messageID) != B_OK) {
		fMessageID = ""; 
	} else {
		fMessageID = messageID;
	}
	if (fDetails->FindFloat("progress", &fProgress) != B_OK) fProgress = 0.0;
	
	SetText(app, title, text);
	ResizeToPreferred();
	
	switch (type) {
		case InfoPopper::Information: {
			SetViewColor(218, 218, 218);
			SetLowColor(218, 218, 218);
		} break;
		case InfoPopper::Important: {
			SetViewColor(255, 255, 255);
			SetLowColor(255, 255, 255);
		} break;
		case InfoPopper::Error: {
			SetViewColor(255, 0, 0);
			SetLowColor(255, 0, 0);
		} break;
		case InfoPopper::Progress: {
			SetViewColor(218, 218, 218);
			SetLowColor(218, 218, 218);
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

//#pragma mark Hooks

void InfoView::AttachedToWindow(void) {
	BMessage msg(REMOVE_VIEW);
	msg.AddPointer("view", this);
	int32 timeout = -1;
	if (fDetails->FindInt32("timeout", &timeout) != B_OK) {
		timeout = fParent->DisplayTime();
	};
	bigtime_t delay = timeout*1000*1000;
	
	if ( delay > 0 )
		fRunner = new BMessageRunner( BMessenger(Parent()), &msg, delay, 1 );		
};

void InfoView::MessageReceived(BMessage * msg) {
	switch (msg->what) {
		case B_GET_PROPERTY: {
			BMessage specifier;
			const char * property;
			BMessage reply(B_REPLY);
			bool msgOkay = true;
			
			if (msg->FindMessage("specifiers", 0, &specifier) != B_OK) msgOkay = false;
			if (specifier.FindString("property", &property) != B_OK) msgOkay = false;

			if (msgOkay) {
				printf("Looking for %s\n", property);
				if (strcmp(property, "content") == 0) {
					reply.AddString("result", fText);
				};
				
				if (strcmp(property, "title") == 0)  {
					reply.AddString("result", fTitle);
				};
				
				if (strcmp(property, "apptitle") == 0) {
					reply.AddString("result", fApp);
				};
				
				if (strcmp(property, "type") == 0) {
					reply.AddInt32("result", fType);
				};
				
				if (strcmp(property, "progress") == 0) {
					reply.AddFloat("result", fProgress);
				};
				
				if (strcmp(property, "iconRef") == 0) {
					entry_ref ref;
					if (fDetails->FindRef("iconRef", &ref) == B_OK) {
						reply.AddRef("result", &ref);
					} else {
						reply.AddInt32("error", B_ERROR);
					};
				};
				
				if (strcmp(property, "iconType") == 0) {
					int32 type = B_ERROR;
					if (fDetails->FindInt32("iconType", &type) == B_OK) {
						reply.AddInt32("result", type);
					} else {
						reply.AddInt32("error", B_ERROR);
					};
				};
				
				if (strcmp(property, "overlayIconRef") == 0) {
					entry_ref ref;
					if (fDetails->FindRef("overlayIconRef", &ref) == B_OK) {
						reply.AddRef("result", &ref);
					} else {
						reply.AddInt32("error", B_ERROR);
					};
				};
	
				if (strcmp(property, "overlayIconType") == 0) {
					int32 type = B_ERROR;
					if (fDetails->FindInt32("overlayIconType", &type) == B_OK) {
						reply.AddInt32("result", type);
					} else {
						reply.AddInt32("error", B_ERROR);
					};
				};
							
				if (strcmp(property, "icon") == 0) {
					if (fBitmap) {
						BMessage archive;
//						int32 bitmap_size = fBitmap->FlattenedSize();
//						char * bitmap_data = malloc( bitmap_size );
//						if (fBitmap->Flatten(bitmap_data, bitmap_size) == B_OK) {
						if ( fBitmap->Archive(&archive) == B_OK ) {
//							reply.AddData("result", B_RAW_TYPE, bitmap_data, bitmap_size);
							reply.AddMessage("result", &archive);
						};
					}
				};
				reply.AddInt32("error", B_OK);
			} else {
				reply.what = B_MESSAGE_NOT_UNDERSTOOD;
				reply.AddInt32("error", B_ERROR);
			};

			msg->SendReply(&reply);
		} break;
		
		case B_SET_PROPERTY: {
			BMessage specifier;
			const char * property;
			BMessage reply(B_REPLY);
			bool msgOkay = true;
			
			if (msg->FindMessage("specifiers", 0, &specifier) != B_OK) msgOkay = false;
			if (specifier.FindString("property", &property) != B_OK) msgOkay = false;
		
			if (msgOkay) {
				BString app(fApp), title(fTitle), text(fText);
				
				if (strcmp(property, "content") == 0) {
					msg->FindString("data", &text);
				};
				
				if (strcmp(property, "apptitle") == 0) {
					msg->FindString("data", &app);
				};
				
				if (strcmp(property, "title") == 0) {
					msg->FindString("data", &title);
				};
				
				if (strcmp(property, "icon") == 0) {
					
				};
				
				if (strcmp(property, "iconRef") == 0) {
					entry_ref ref;
					if (msg->FindRef("data", &ref) == B_OK) {
						BPath path(&ref);
						BBitmap *temp = ReadNodeIcon(path.Path(), fParent->IconSize());
						delete fBitmap;
						fBitmap = rescale_bitmap(temp, fParent->IconSize());
						delete temp;
					};
				};
				
				if (strcmp(property, "overlayIconRef") == 0) {
					entry_ref ref;
					if (msg->FindRef("data", &ref) == B_OK) {
						BPath path(&ref);
						BBitmap *temp = ReadNodeIcon(path.Path(), fParent->IconSize() / 4);
						delete fBitmap;
						fBitmap = rescale_bitmap(temp, fParent->IconSize() / 4);
						delete temp;
					};
				};
	
				SetText( 
					app.Length() > 0 ? app.String() : NULL, 
					title.Length() > 0 ? title.String() : NULL, 
					text.Length() > 0 ? text.String() : NULL
				);
				Invalidate();
				
				reply.AddInt32("error", B_OK);
			} else {
				reply.what = B_MESSAGE_NOT_UNDERSTOOD;
				reply.AddInt32("error", B_ERROR);				
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
	// Parent width, minus the edge padding, minus the pensize
	*w = fParent->ViewWidth() - (kEdgePadding * 2) - (kPenSize * 2);
	*h = fHeight;
	
	if (fType == InfoPopper::Progress) {
		font_height fh;
		be_plain_font->GetHeight(&fh);
		float fontHeight = fh.ascent + fh.descent + fh.leading;
		*h += (kSmallPadding * 2) + (kEdgePadding * 1) + fontHeight;
	};
};

void InfoView::Draw(BRect /*drawBounds*/) {
	BRect bound = Bounds();
	BRect progRect;
	
	// draw progress background
	
	if (fType == InfoPopper::Progress) {
		PushState();
		
		font_height fh;
		be_plain_font->GetHeight(&fh);
		float fontHeight = fh.ascent + fh.descent + fh.leading;

		progRect = bound;
		progRect.InsetBy(kEdgePadding, kEdgePadding);
		progRect.top = progRect.bottom - (kSmallPadding * 2) - fontHeight;
		
		StrokeRect(progRect);

		BRect barRect = progRect;		
		barRect.InsetBy(1.0, 1.0);
		barRect.right *= fProgress;
#ifdef B_BEOS_VERSION_DANO
		SetHighColor(ui_color(B_UI_CONTROL_HIGHLIGHT_COLOR));
#else
		SetHighColor(0x88, 0xff, 0x88);
#endif
		FillRect(barRect);

		SetHighColor(0, 0, 0);
		
		BString label = "";
		label << (int)(fProgress * 100) << " %";
		
		float labelWidth = be_plain_font->StringWidth(label.String());
		float labelX = progRect.left + (progRect.IntegerWidth() / 2) - (labelWidth / 2);

		SetLowColor(B_TRANSPARENT_COLOR);
		SetDrawingMode(B_OP_ALPHA);
		DrawString(label.String(), label.Length(),
			BPoint(labelX, progRect.top + fh.ascent + fh.leading + kSmallPadding));

		PopState();
	};
		
	SetDrawingMode(B_OP_ALPHA);
	
	// draw icon
	BPoint iconPoint(0, 0);
	if (fBitmap) {
		lineinfo *appLine = fLines.back();
		font_height fh;
		appLine->font.GetHeight(&fh);
		
		float title_bottom = appLine->location.y + fh.descent;
		
		float ix = kEdgePadding;
		float iy = 0;
		if (fParent->Layout() == TitleAboveIcon) {
			iy = title_bottom + kEdgePadding + (bound.Height() - title_bottom - kEdgePadding*2 - fBitmap->Bounds().Height()) / 2;
		} else {
			iy = (bound.Height() - fBitmap->Bounds().Height()) / 2.0;
		};
		
		if (fType == InfoPopper::Progress) {
			// move icon up by half progress bar height if it's present
			iy -= (progRect.Height() + kEdgePadding) / 2.0;
		}
		
		iconPoint.x = ix;
		iconPoint.y = iy;
		DrawBitmapAsync(fBitmap, iconPoint);
	};
	
	if (fOverlayBitmap) {
		BRect overRect = fOverlayBitmap->Bounds();
		if (fBitmap) {
			BRect rect = fBitmap->Bounds();
			iconPoint.x += rect.Width() - overRect.Width();
			iconPoint.y += rect.Height() - overRect.Height();
		} else {
			iconPoint.x += overRect.Width();
			iconPoint.y += overRect.Height();
		};
		
		DrawBitmapAsync(fOverlayBitmap, iconPoint);
	};
	
//	Draw content
	vline::iterator lIt;
	for (lIt = fLines.begin(); lIt != fLines.end(); lIt++) {
		lineinfo *l = (*lIt);
		
		SetFont(&l->font);
		DrawString(l->text.String(), l->text.Length(), l->location);
	};
	
	rgb_color detailCol = ui_color(B_CONTROL_BORDER_COLOR);
	detailCol = tint_color(detailCol, B_LIGHTEN_2_TINT);

	// Draw the close widget
	BRect closeRect = bound;
	closeRect.InsetBy(kEdgePadding, kEdgePadding);
	closeRect.left = closeRect.right - kCloseSize;
	closeRect.bottom = closeRect.top + kCloseSize;
	
	PushState();
		SetHighColor(detailCol);

		StrokeRoundRect(closeRect, kSmallPadding, kSmallPadding);
		
		BRect closeCross = closeRect.InsetByCopy(kSmallPadding, kSmallPadding);
		StrokeLine(closeCross.LeftTop(), closeCross.RightBottom());
		StrokeLine(closeCross.LeftBottom(), closeCross.RightTop());
	PopState();
		
	Sync();
}

void InfoView::MouseDown(BPoint point) {
	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	
	switch (buttons) {
		case B_PRIMARY_MOUSE_BUTTON: {
			BRect closeRect = Bounds().InsetByCopy(2,2);
			closeRect.left = closeRect.right - kCloseSize;
			closeRect.bottom = closeRect.top + kCloseSize;	
			
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
					if (be_roster->FindApp(&launchRef, &appRef) == B_OK) {
						printf("File to launch for %s is %s\n", launchRef.name, appRef.name);
						 useArgv = true;
					};
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
				
				BMessage tmp;
				for (int32 i = 0; fDetails->FindMessage("onClickMsg", i, &tmp) == B_OK; i++) {
					messages.AddItem((void *)&tmp);
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
			
			BMessenger msgr(Parent());
			msgr.SendMessage(&remove_msg);
		} break;
	};
};

void InfoView::FrameResized( float w, float /*h*/ ) {
	// SetText again to re-wrap lines to new view width
	BString app(fApp), title(fTitle), text(fText);
	
	SetText( 
		app.Length() > 0 ? app.String() : NULL, 
		title.Length() > 0 ? title.String() : NULL, 
		text.Length() > 0 ? text.String() : NULL,
		w
	);
}

//#pragma mark Scripting Hooks

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

//#pragma mark Public

void InfoView::SetText(const char *app, const char *title, const char *text, float newMaxWidth) {
 	if ( newMaxWidth < 0 ) newMaxWidth = Bounds().Width() - (kEdgePadding * 2);
	
	// delete old lines
	vline::iterator lIt;
	for (lIt = fLines.begin(); lIt != fLines.end(); lIt++) delete (*lIt);
	fLines.clear();
	
	// do the text thing
	
	// TODO: comment this mess!
	
	fApp = app;
	fTitle = title;
	fText = text;

	font_height fh;
	float fontHeight = 0;
	float iconRight = kEdgePadding + kEdgePadding;
	float y = 0;
	if (fBitmap) iconRight += fBitmap->Bounds().right;

	be_bold_font->GetHeight(&fh);
	fontHeight = fh.leading + fh.descent + fh.ascent;
	y += fontHeight;

	lineinfo *titleLine = new lineinfo;
	titleLine->text = title;
	titleLine->font = be_bold_font;

	if (fParent->Layout() == AllTextRightOfIcon) {
		titleLine->location = BPoint(iconRight, y);
	} else {
		titleLine->location = BPoint(kEdgePadding, y);
	};

	fLines.push_front(titleLine);
	y += fontHeight;
	
	const char spacers[] = " \n-\\/";
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
	float maxWidth = newMaxWidth - kEdgePadding - iconRight;
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
			
			fLines.push_front(tempLine);
		} else if ( 
			((i+1 < count) && (StringWidth(text + offset, spaces[i+1] - offset) > maxWidth)) ||
			((i+1 == count) && (StringWidth(text + offset) > maxWidth))
		) {
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
			if ((tempLine->text.String())[0] == ' ') {
				tempLine->text.RemoveFirst(" ");
			};
			
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
	if ( (tempLine->text.String())[0] == ' ' ) tempLine->text.RemoveFirst(" ");

	free(spaces);		

	fLines.push_front(tempLine);
	
	fHeight = y + (kEdgePadding * 2);
	
	// make sure icon fits
	if ( fBitmap )
	{
		lineinfo * appLine = fLines.back();
		font_height fh;
		appLine->font.GetHeight( &fh );
		
		float title_bottom = fLines.back()->location.y + fh.descent;
		
		if ( fParent->Layout() == TitleAboveIcon )
		{
			if ( fHeight < title_bottom + fBitmap->Bounds().Height() + 2*kEdgePadding )
				fHeight = title_bottom + fBitmap->Bounds().Height() + 2*kEdgePadding;
		} else
		{
			if ( fHeight < fBitmap->Bounds().Height() + kEdgePadding*2 )
				fHeight = fBitmap->Bounds().Height() + kEdgePadding*2;
		}
	}
	
	BMessenger msgr(Parent());
	msgr.SendMessage(InfoWindow::ResizeToFit);
};

bool InfoView::HasMessageID(const char * id) {
	return fMessageID == id;
};

const char *InfoView::MessageID(void) {
	return fMessageID.String();
};

void InfoView::SetPosition(bool first, bool last) {
	fIsFirst = first;
	fIsLast = last;
};

//#pragma mark Private

BBitmap *InfoView::ExtractIcon(const char *prefix, BMessage *msg, int16 size) {
	BBitmap *icon = NULL;
	BMessage iconMsg;
	BString refName = prefix;
	BString refType = prefix;
	refName << "Ref";
	refType << "Type";
	
	if (msg->FindMessage(prefix, &iconMsg) == B_OK) {
		BBitmap temp(&iconMsg);
		icon = rescale_bitmap(&temp, size);
	} else {		
		int32 iconType;
		if (msg->FindInt32(refType.String(), &iconType) != B_OK) iconType = Attribute;
		
		entry_ref ref;
		
		if (fDetails->FindRef(refName.String(), &ref) == B_OK) {
			// It's a ref.
			BPath path(&ref);
			
			switch (iconType) {
				case Attribute: {
					BBitmap *temp = ReadNodeIcon(path.Path(), size);
					icon = rescale_bitmap(temp, size);
					delete temp;
				} break;
				case Contents: {
					// ye ol' "create a bitmap from contens of file"
					BBitmap *temp = BTranslationUtils::GetBitmapFile(path.Path());
					icon = rescale_bitmap(temp, size);
					delete temp;
					if (!icon) printf("Error reading bitmap\n");
				} break;
				default: {
					// Eek! Invalid icon type!
				}; break;
			};
		};
	};

	return icon;
};
