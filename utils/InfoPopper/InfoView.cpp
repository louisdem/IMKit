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

property_info message_prop_list[] = {
	{ "content", {B_GET_PROPERTY, 0},{B_DIRECT_SPECIFIER, 0}, "get a message"},
	{ "title", {B_GET_PROPERTY, 0},{B_DIRECT_SPECIFIER, 0}, "get a message"},
	{ "icon", {B_GET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get icon as a flattened bitmap"},
	0 // terminate list
};

InfoView::InfoView(InfoWindow *win, info_type type,
	const char *app, const char *title, const char * text, BMessage *details)

:	BView( BRect(0,0,win->ViewWidth(),1), "InfoView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE|B_FRAME_EVENTS ),

	fType(type),
	fRunner(NULL),
	fDetails(details),
	fBitmap(NULL),
	fParent(win) {
	
	BMessage iconMsg;
	BBitmap *icon = NULL;
	
	if (fDetails->FindMessage("icon", &iconMsg) == B_OK) {
		icon = new BBitmap(&iconMsg);
	} else {		
		int32 iconType;
		if (fDetails->FindInt32("iconType", &iconType) != B_OK) {
			iconType = Attribute;
		};
		
		entry_ref ref;
		
		if ( fDetails->FindRef("iconRef", &ref) == B_OK ) {
			// It's a ref.
			BPath path(&ref);
			
			switch ( iconType )
			{
				case Attribute: {
					icon = ReadNodeIcon(path.Path(), fParent->IconSize());
				}	break;
				case Contents: {
					// ye ol' "create a bitmap from contens of file"
					icon = BTranslationUtils::GetBitmapFile(path.Path());
					if (!icon) {
						printf("Error reading bitmap\n");
					};
				} break;
				default:
					// Eek! Invalid icon type!
					break;
			}
		}
	};
	
	ScaleBitmap(icon);
	
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
InfoView::AttachedToWindow() {
	BMessage msg(REMOVE_VIEW);
	msg.AddPointer("view", this);
	int32 timeout = -1;
	if (fDetails->FindInt32("timeout", &timeout) != B_OK) {
		timeout = fParent->DisplayTime();
	};
	bigtime_t delay = timeout*1000*1000;
	
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
	*w = fParent->ViewWidth();
	*h = fHeight;
	
	if (fType == InfoPopper::Progress) {
		font_height fh;
		be_plain_font->GetHeight(&fh);
		float fontHeight = fh.ascent + fh.descent + fh.leading;
		*h += 10 + (kEdgePadding * 1) + fontHeight;
	};
};

void InfoView::Draw(BRect drawBounds) {
	BRect bound = Bounds();
	BRect progRect;
	
	// draw progress background
	
	if (fType == InfoPopper::Progress) {
		PushState();
		
		font_height fh;
		be_plain_font->GetHeight(&fh);
		float fontHeight = fh.ascent + fh.descent + fh.leading;

		progRect = Bounds();
		progRect.InsetBy(kEdgePadding, kEdgePadding);
		progRect.top = progRect.bottom - (kEdgePadding * 2) - fontHeight;
		
		StrokeRect(progRect);

		BRect barRect = progRect;		
		barRect.InsetBy(1.0, 1.0);
		barRect.right *= fProgress;
		SetHighColor(0x88, 0xff, 0x88);
		FillRect(barRect);

		SetHighColor(0, 0, 0);
		
		BString label = "";
		label << (int)(fProgress * 100) << " %";
		
		float labelWidth = be_plain_font->StringWidth(label.String());
		float labelX = progRect.left + (progRect.IntegerWidth() / 2) - (labelWidth / 2);

		SetLowColor(B_TRANSPARENT_COLOR);
		SetDrawingMode(B_OP_ALPHA);
		DrawString(label.String(), label.Length(), BPoint(labelX, progRect.top + fontHeight));

		PopState();
	};
		
	SetDrawingMode( B_OP_ALPHA );
	
	// draw icon
	if (fBitmap) {
		lineinfo * appLine = fLines.back();
		font_height fh;
		appLine->font.GetHeight( &fh );
		
		float title_bottom = fLines.back()->location.y + fh.descent;
		
		float ix = kEdgePadding;
		float iy = 0;
		if (fParent->Layout() == TitleAboveIcon) {
			iy = title_bottom + kEdgePadding + (Bounds().Height() - title_bottom - kEdgePadding*2 - fBitmap->Bounds().Height()) / 2;
		} else {
			iy = (Bounds().Height() - fBitmap->Bounds().Height()) / 2.0;
		};
		
		if (fType == InfoPopper::Progress) {
			// move icon up by half progress bar height if it's present
			iy -= (progRect.Height() + kEdgePadding) / 2.0;
		}
		
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
	closeRect.left--;

	// TODO: change this to something ui_color() dependant
	SetHighColor(218, 218, 218);
	FillRect(closeRect);
	
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
			BRect closeRect = Bounds().InsetByCopy(2,2);
			closeRect.left = closeRect.right - kCloseWidth;
			closeRect.bottom = closeRect.top + kCloseWidth;	
			
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
	if ( newMaxWidth < 0 ) newMaxWidth = Bounds().Width();
	
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
	float y = kEdgePadding;
	if (fBitmap) iconRight += fBitmap->Bounds().right;

	lineinfo *appLine = new lineinfo;
	be_bold_font->GetHeight(&fh);
	fontHeight = fh.leading + fh.descent + fh.ascent;
	y += fontHeight;
	
	//printf("Right: %.2f\n", iconRight);
	
	if (fParent->Layout() == AllTextRightOfIcon) {
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

//#pragma mark -

typedef struct {
	int32 srcColumn;
	int32 alpha0;
	int32 alpha1;
} ColumnData;

typedef int32 fixed_point;
typedef int64 long_fixed_point;

#define to_fixed_point(number) static_cast<fixed_point>((number) * kFPPrecisionFactor)
#define from_fixed_point(number) ((number) / kFPPrecisionFactor)
#define to_float(number) from_fixed_point(static_cast<float>(number))

#define int_value(number) ((number) & kFPInverseMask)
#define tail_value(number) ((number) & kFPPrecisionMask)

// Has to be called after muliplication of two fixed point values
#define mult_correction(number) ((number) / kFPPrecisionFactor)

const int32 kFPPrecision = 8; // (32-kFPPrecision).kFPPrecision
const int32 kFPPrecisionFactor = (1 << kFPPrecision);
const int32 kFPPrecisionMask = ((kFPPrecisionFactor)-1);
const int32 kFPInverseMask = (~kFPPrecisionMask);
const int32 kFPOne = to_fixed_point(1);

void InfoView::ScaleBitmap(BBitmap *source) {
	if (fBitmap) delete fBitmap;
	fBitmap = source;
/*
//	int32 iconSize = (dynamic_cast<InfoWindow *>(Window()))->IconSize();
	int32 iconSize = 16;
	BRect bounds(0, 0, iconSize - 1, iconSize - 1);
	fBitmap = new BBitmap(bounds, B_RGBA32);

	if (source == NULL) {
		memset(fBitmap->Bits(), B_TRANSPARENT_MAGIC_RGBA32, fBitmap->BitsLength());
	} else {
		int32 fromRow = 0;
		int32 toRow = iconSize - 1;
	
		const int32 kBPP = sizeof(int32);
	
		int32 srcW = source->Bounds().IntegerWidth();
		int32 srcH = source->Bounds().IntegerHeight();
		int32 destW = fBitmap->Bounds().IntegerWidth();
		int32 destH = fBitmap->Bounds().IntegerHeight();

//		Fixed point representations
		fixed_point fpSrcW = to_fixed_point(srcW);
		fixed_point fpSrcH = to_fixed_point(srcH);
		fixed_point fpDestW = to_fixed_point(destW);
		fixed_point fpDestH = to_fixed_point(destH);
		
		uchar *srcBits = (uchar *)source->Bits();
		uchar *destBits = (uchar *)fBitmap->Bits();
		
		int32 srcBPR = source->BytesPerRow();
		int32 destBPR = fBitmap->BytesPerRow();
		
		ColumnData *cols = new ColumnData[destW];
		for (int32 i = 0; i < destW; i++) {
			fixed_point column = to_fixed_point(i) * (long_fixed_point)fpSrcW / fpDestW;
			cols->srcColumn = from_fixed_point(column);
			cols->alpha1 = tail_value(column); // weight for left pixel value;
			cols->alpha0 = kFPOne - cols->alpha1; // weight for right pixel
		};
		
		uchar *destDataRow = destBits + fromRow * destBPR;
		
		for (int32 y =	0; y <= toRow; y++, destDataRow += destBPR) {
			fixed_point row = 0;
			int32 srcRow;
			fixed_point alpha0;
			fixed_point alpha1;
			
			if (fpDestH != 0) {
				row = to_fixed_point(y) * (long_fixed_point)fpSrcH / fpDestH;
			};
			
			srcRow = from_fixed_point(row);
			alpha1 = tail_value(row);	// weight for y + 1
			alpha0 = kFPOne - alpha1;	// weight for y
		
			const uchar *srcData = srcBits + srcRow * srcBPR;
			uchar *destData = destDataRow;
			
			// Need mult_correction for "outer" multiplication only
			#define I4(i) from_fixed_point(mult_correction(\
				(a[i] * a0 + b[i] * a1) * alpha0 + \
				(c[i] * a0 + d[i] * a1) * alpha1))
			#define V2(i) from_fixed_point(a[i] * alpha0 + c[i] * alpha1);
			#define H2(i) from_fixed_point(a[i] * a0 + b[i] * a1);

			if (y < destH) {
				fixed_point a0, a1;
				const uchar *a, *b, *c, *d;
	
				for (int32 x = 0; x < destW; x ++, destData += kBPP) {
					a = srcData + cols[x].srcColumn * kBPP;
					b = a + kBPP;
					c = a + srcBPR;
					d = c + kBPP;
					
					a0 = cols[x].alpha0;
					a1 = cols[x].alpha1;
					
					printf("%i / %i\n", x, destW);
					
					destData[0] = I4(0);
					destData[1] = I4(1);
					destData[2] = I4(2);
				}
				
				// right column
				a = srcData + srcW * kBPP;
				c = a + srcBPR;
				
				destData[0] = V2(0);
				destData[1] = V2(1);
				destData[2] = V2(2);
			} else {
				fixed_point a0, a1;
				const uchar *a, *b;
				for (int32 x = 0; x < destW; x ++, destData += kBPP) {
					a = srcData + cols[x].srcColumn * kBPP;
					b = a + kBPP;
					
					a0 = cols[x].alpha0;
					a1 = cols[x].alpha1;
					
					destData[0] = H2(0);
					destData[1] = H2(1);
					destData[2] = H2(2);
				}
				
				// bottom, right pixel
				a = srcData + srcW * kBPP;
	
				destData[0] = a[0];
				destData[1] = a[1];
				destData[2] = a[2];
			};
		};
		
		delete[] cols;
	};
*/
};
