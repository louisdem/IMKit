#include "InfoView.h"

#include <StringView.h>
#include <Window.h>
#include <Messenger.h>

InfoView::InfoView( info_type type, const char * text )
:	BView( BRect(0,0,1,1), "InfoView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW ),
	fRunner(NULL),
	fFilter(NULL)
{
	float w,h;
	
/*	fView = new BTextView( 
		Bounds(), "infoText", Bounds(), B_FOLLOW_ALL
	);
	fView->Insert( text );
	
	fView->GetPreferredSize(&w,&h);
	
	fView->ResizeTo(w,h);
*/	
	SetText( text );
	
	GetPreferredSize(&w,&h);
	
	ResizeTo(w,h);
	
//	AddChild( fView );
	
	switch ( type )
	{
		case Information:
			SetViewColor(218,218,218);
//			fView->SetViewColor( ViewColor() );
			break;
		case Important:
			SetViewColor(255,255,255);
//			fView->SetViewColor( ViewColor() );
			break;
		case Error:
			SetViewColor(255,0,0);
//			fView->SetViewColor( ViewColor() );
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
	
	fRunner = new BMessageRunner( BMessenger(Window()), &msg, 5*1000*1000, 1 );
	fFilter = new InputFilter(this);
	
	AddFilter(fFilter);
}

void
InfoView::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case REMOVE_VIEW:
		{
			BMessage remove(REMOVE_VIEW);
			remove.AddPointer("view", this);
			BMessenger msgr(Window());
			msgr.SendMessage( &remove );
//			BMessenger(Window()).SendMessage(&remove);
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
	BFont font;
	GetFont( &font );
	
	font_height fh;
	
	font.GetHeight( &fh );
	
	float line_height = fh.ascent + fh.descent + fh.leading;
	
//	SetHighColor(
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
