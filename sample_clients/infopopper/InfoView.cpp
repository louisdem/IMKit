#include "InfoView.h"

#include <StringView.h>
#include <Window.h>
#include <Messenger.h>
#include <stdio.h>

const float kEdgePadding = 2.0;

InfoView::InfoView( info_type type, const char * text, BMessage *details,
	const char * progID, float prog )
:	BView( BRect(0,0,1,1), "InfoView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW ),
	fType(type),
	fRunner(NULL),
	fProgress(prog),
	fDetails(details) {
	
	if ( progID ) fProgressID = progID;
	
	BMessage iconMsg;
	if (fDetails->FindMessage("icon", &iconMsg) == B_OK) {
		fBitmap = new BBitmap(&iconMsg);
	} else {
		fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
		int32 *bits = (int32 *)fBitmap->Bits();
		
		for (int32 i = 0; i < fBitmap->BitsLength()/4; i++) {
			bits[i] = B_TRANSPARENT_MAGIC_RGBA32;
		};
	};
	
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
}

void
InfoView::AttachedToWindow()
{
	BMessage msg(REMOVE_VIEW);
	msg.AddPointer("view", this);
	
	bigtime_t delay = 5*1000*1000;
	
	switch ( fType )
	{
		case Progress:
			delay = 15*1000*1000;
			break;
	}
	
	fRunner = new BMessageRunner( BMessenger(Window()), &msg, 5*1000*1000, 1 );
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
	GetFont(&font);
	
	font_height fh;
	font.GetHeight( &fh );
	
	float line_height = fh.ascent + fh.descent + fh.leading;
	
	*h = line_height * fLines.size() + kEdgePadding;
	*w = 0;
	
	for (list<BString>::iterator i = fLines.begin(); i !=fLines.end(); i++) {
		float width = StringWidth( (*i).String() );
		
		if ( width > *w ) *w = width + kEdgePadding;
	};
	
	*w += fBitmap->Bounds().Width() + (kEdgePadding * 2);
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
	
	DrawBitmap(fBitmap,
		BPoint(
			kEdgePadding,
			(bound.Height() / 2) - (fBitmap->Bounds().Height() / 2)
		)
	);
	
	BFont font;
	GetFont(&font);
	
	font_height fh;
	font.GetHeight( &fh );
	float line_height = fh.ascent + fh.descent + fh.leading;
	
	int y = 1;
	for (list<BString>::iterator i = fLines.begin(); i !=fLines.end(); i++) {
		DrawString(i->String(),
			BPoint(
				(kEdgePadding * 2) + fBitmap->Bounds().Width(),
				kEdgePadding + y * line_height - fh.leading - fh.descent
			)
		);
		y++;
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
			
							
			if (fDetails->FindString("onClickApp", &launchString) == B_OK) {
				be_roster->Launch(launchString.String(), &messages);
			} else {
				be_roster->Launch(&launchRef, &messages);
			};
					
		} break;
	};
};

void
InfoView::SetText(const char * _text)
{
	BString text(_text);
	
	fLines.clear();
	
	while ( text.Length() > 0 )
	{
		int32 nl;
		if ( (nl = text.FindFirst("\n")) >= 0 )
		{ // found a newline
			BString line;
			text.CopyInto(line, 0, nl);
			fLines.push_back(line);
			
			text.Remove(0,nl+1);
		} else
		{
			fLines.push_back( text );
			text = "";
		}
	}
}

bool
InfoView::HasProgressID( const char * id )
{
	return fProgressID == id;
}
