#include "BorderView.h"

#include <stdio.h>

#define BORDER_W 5.0

BorderView::BorderView( BRect rect, const char * text )
:	BView(rect, "borderView", B_FOLLOW_ALL, B_WILL_DRAW|B_FRAME_EVENTS),
	fTitle(text)
{
	SetViewColor( B_TRANSPARENT_COLOR );
}

BorderView::~BorderView()
{
}

void
BorderView::FrameResized( float, float )
{
	Invalidate();
}

void
BorderView::Draw( BRect rect )
{
	rgb_color col_bg = (rgb_color){128,128,255,255};
	rgb_color col_text = (rgb_color){255,255,255,255};
	
	// Background
	SetDrawingMode( B_OP_COPY );
	
	SetHighColor( col_bg );
	
	FillRect( rect );
	
	SetHighColor( 0,0,0 );
	
	BRect content = Bounds();
	content.InsetBy( BORDER_W, BORDER_W );
	content.top += 20;
	
	BRect content_line(content);
	content_line.InsetBy(-1,-1);
	
	StrokeRect( content_line );
	
	// Text
	SetHighColor( col_text );
	SetLowColor( col_bg );
	
	SetDrawingMode( B_OP_ALPHA );
	
	SetFont( be_bold_font );
	SetFontSize( 18 );
	
	float string_width = StringWidth( fTitle.String() );
	
	DrawString( fTitle.String(), BPoint( BORDER_W,20) );
}

void
BorderView::GetPreferredSize( float * w, float * h )
{
	*w = 2 * BORDER_W;
	*h = 2 * BORDER_W + 20;
}
