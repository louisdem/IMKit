#include "InfoView.h"

#include <StringView.h>
#include <Window.h>
#include <Messenger.h>

#include <libim/Constants.h>

InfoView::InfoView( info_type type, const char * text, const char * progID, float prog )
:	BView( BRect(0,0,1,1), "InfoView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW ),
	fType(type),
	fRunner(NULL),
	fFilter(NULL),
	fProgress(prog)
{
	if ( progID )
		fProgressID = progID;
	
	float w,h;
	
	SetText( text );
	
	GetPreferredSize(&w,&h);
	
	ResizeTo(w,h);
	
	switch ( type )
	{
		case Information:
			SetViewColor(218,218,218);
			break;
		case Important:
			SetViewColor(255,255,255);
			break;
		case Error:
			SetViewColor(255,0,0);
			break;
		case Progress:
			SetViewColor(218,218,218);
			break;
	}
}

InfoView::~InfoView()
{
	if ( fRunner )
		delete fRunner;
	if ( fFilter )
		delete fFilter;
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
	fFilter = new InputFilter(this);
	
	AddFilter(fFilter);
}

void
InfoView::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case IM::MESSAGE:
		{
			int32 im_what=0;
			
			if ( msg->FindInt32("im_what", &im_what) != B_OK )
				break;
			
			if ( im_what != IM::PROGRESS )
				break;
			
			float progress=0.0;
			
			if ( msg->FindFloat("progress", &progress) == B_OK && msg->FindString("message") != NULL )
			{
				fProgress = progress;
				SetText( msg->FindString("message") );
				
				if ( fRunner )
					fRunner->SetInterval( 15*1000*1000 );
				
				Invalidate();
			}
		}	break;
		
		case REMOVE_VIEW:
		{
			BMessage remove(REMOVE_VIEW);
			remove.AddPointer("view", this);
			BMessenger msgr(Window());
			msgr.SendMessage( &remove );
		}	break;
		default:
			BView::MessageReceived(msg);
	}
}

void
InfoView::GetPreferredSize( float * w, float * h )
{
	BFont font;
	GetFont( &font );
	
	font_height fh;
	
	font.GetHeight( &fh );
	
	float line_height = fh.ascent + fh.descent + fh.leading;
	
	*h = line_height * fLines.size() + 2;
	
	*w = 0;
	for ( list<BString>::iterator i = fLines.begin(); i !=fLines.end(); i++ )
	{
		float width = StringWidth( (*i).String() );
		
		if ( width > *w )
			*w = width + 2;
	}
}

void
InfoView::Draw( BRect )
{
	if ( fProgress > 0.0 )
	{
		BRect pRect = Bounds();
		
		pRect.right *= fProgress;
		
		SetHighColor( 0x88, 0xff, 0x88 );
		
		FillRect( pRect );
		
		SetHighColor( 0,0,0 );
	}
	
	
	BFont font;
	GetFont( &font );
	
	font_height fh;
	
	font.GetHeight( &fh );
	
	float line_height = fh.ascent + fh.descent + fh.leading;
	
	SetDrawingMode( B_OP_ALPHA );
	
	int y = 1;
	for ( list<BString>::iterator i = fLines.begin(); i !=fLines.end(); i++ )
	{
		DrawString(
			(*i).String(), 
			BPoint(1,1+y*line_height-fh.leading-fh.descent)
		);
		y++;
	}
}

void
InfoView::SetText( const char * _text )
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
