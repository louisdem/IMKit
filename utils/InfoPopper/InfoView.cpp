#include "InfoView.h"

#include <StringView.h>
#include <Window.h>
#include <Messenger.h>
#include <stdio.h>

const float kEdgePadding = 2.0;

InfoView::InfoView( info_type type, const char * text, BMessage *details )
:	BView( BRect(0,0,1,1), "InfoView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW ),
	fType(type),
	fRunner(NULL),
	fDetails(details) {
	
	BMessage iconMsg;
	if (fDetails->FindMessage("icon", &iconMsg) == B_OK) {
		fBitmap = new BBitmap(&iconMsg);
	} else {
		/*fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
		int32 *bits = (int32 *)fBitmap->Bits();
		
		for (int32 i = 0; i < fBitmap->BitsLength()/4; i++) {
			bits[i] = 0xff8888ff;//B_TRANSPARENT_MAGIC_RGBA32;
		};
		*/
		fBitmap = NULL;
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
