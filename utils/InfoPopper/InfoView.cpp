#include "InfoView.h"
#include "IconUtils.h"

#include <TranslationUtils.h>
#include <StringView.h>
#include <Window.h>
#include <Messenger.h>
#include <stdio.h>
#include <PropertyInfo.h>

const float kEdgePadding = 2.0;

// 
property_info message_prop_list[] = {
	{ "content", {B_GET_PROPERTY, 0},{B_DIRECT_SPECIFIER, 0}, "get a message"},
	{ "title", {B_GET_PROPERTY, 0},{B_DIRECT_SPECIFIER, 0}, "get a message"},
//	{ "head", {B_GET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get head"},
//	{ "head", {B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "set head"},
	0 // terminate list
};

InfoView::InfoView( info_type type, const char * text, BMessage *details )
:	BView( BRect(0,0,1,1), "InfoView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW ),
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
	
	float w,h;
	
	SetText( text );
	
	GetPreferredSize(&w,&h);
	
	ResizeTo(w,h);
	
	switch ( type )
	{
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
			msg->PrintToStream();
			
			BMessage specifier;
			if ( msg->FindMessage("specifiers",0,&specifier) != B_OK )
				return;
			
			const char * property;
			if ( specifier.FindString("property",&property) != B_OK )
				return;
			
			BMessage reply(B_REPLY);
			
			if ( strcmp(property, "content") == 0 ) {
				BString content;
				
				for ( list<pair<BString,const BFont*> >::iterator i=fLines.begin(); i!=fLines.end(); i++ ) {
					if ( i != fLines.begin() ) {
						if ( content != "" )
							content.Append("\n");
						content.Append(i->first);
					}
				}
				reply.AddString("result", content.String());
			}
			
			if ( strcmp(property, "title") == 0 ) {
				BString title;
				title = fLines.begin()->first;
				title.RemoveLast(":");
				
				reply.AddString("result", title.String());
			}
			
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
	BFont font;

	*h = kEdgePadding;
	*w = 0;
	
	float first_line_height = 0.0;
	
	for (list<pair<BString,const BFont*> >::iterator i = fLines.begin(); i !=fLines.end(); i++) {
		// height
		SetFont(i->second);
		GetFont(&font);
		
		font_height fh;
		font.GetHeight( &fh );
		
		float line_height = fh.ascent + fh.descent + fh.leading;
		
		if ( i == fLines.begin() )
			first_line_height = line_height;
		
		*h += line_height;
		
		// width
		float width = StringWidth( (i->first).String() );
		
		if ( width > *w ) *w = width + kEdgePadding;
	};
	
	if ( fBitmap )
	{
		if (*h < fBitmap->Bounds().Height() + first_line_height) 
			*h = fBitmap->Bounds().Height() + (kEdgePadding * 2) + first_line_height;
		*w += fBitmap->Bounds().Width() + (kEdgePadding * 2) + 3;
	}
};

void InfoView::Draw(BRect drawBounds) {
	BRect bound = Bounds();
	
	if (fProgress > 0.0) {
		bound.right *= fProgress;
		
		SetHighColor(0x88, 0xff, 0x88);
		FillRect(bound);
		SetHighColor(0, 0, 0);
	}
	
	SetDrawingMode( B_OP_ALPHA );
	
	if ( fBitmap )
	{
		// figure out height of top line
		list<pair<BString,const BFont*> >::iterator i = fLines.begin();
		
		SetFont( i->second );
		
		BFont font;
		GetFont(&font);
		
		font_height fh;
		font.GetHeight( &fh );
		float line_height = fh.ascent + fh.leading + fh.descent;
		
		//
		DrawBitmap(fBitmap,
			BPoint(
				kEdgePadding,
				kEdgePadding + line_height + (Bounds().Height() - kEdgePadding - line_height - fBitmap->Bounds().Height()) / 2
			)
		);
	}
	
	BFont font;
	
	float y = 0.0f;
	for (list<pair<BString,const BFont*> >::iterator i = fLines.begin(); i !=fLines.end(); i++) {
		// set font
		SetFont( i->second );
		
		GetFont(&font);
		
		font_height fh;
		font.GetHeight( &fh );
		float line_height = fh.ascent;
		
		y += line_height;
		
		// figure out text position
		float tx = (kEdgePadding * 2) + (fBitmap != NULL && i != fLines.begin() ? fBitmap->Bounds().Width() + 3: 0);
		float ty = kEdgePadding + y;
		
		// draw the text
		DrawString(i->first.String(),BPoint(tx,ty));
		
		y += fh.leading + fh.descent;
	}
}

void InfoView::MouseDown(BPoint point) {
	int32 buttons;
	Window()->CurrentMessage()->FindInt32("buttons", &buttons);
	
	switch (buttons) {
		case B_PRIMARY_MOUSE_BUTTON: {
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
			
			// remove the info view after a click
			BMessage remove_msg(REMOVE_VIEW);
			remove_msg.AddPointer("view", this);
			
			BMessenger msgr(Window());
			msgr.SendMessage(&remove_msg);
		} break;
	};
};

void
InfoView::SetText(const char * _text)
{
	BString text(_text);
	
	fLines.clear();
	
	const BFont * font = be_bold_font;
	
	while ( text.Length() > 0 )
	{
		int32 nl;
		if ( (nl = text.FindFirst("\n")) >= 0 || (nl = text.FindFirst("\\n")) >= 0 )
		{ // found a newline
			BString line;
			text.CopyInto(line, 0, nl);
			fLines.push_back( pair<BString,const BFont*>(line,font) );
			
			if ( text[nl] == '\n' )
				text.Remove(0,nl+1);
			else
				text.Remove(0,nl+2);
			
			font = be_plain_font;
		} else
		{
			fLines.push_back( pair<BString,const BFont*>(text,font) );
			text = "";
		}
	}
}

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

