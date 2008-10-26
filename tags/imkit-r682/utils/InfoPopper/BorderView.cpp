#include "BorderView.h"

#include <stdio.h>

#define BORDER_W 2.0
#define TITLE_SIZE 14

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
	rgb_color col_bg = ui_color(B_PANEL_BACKGROUND_COLOR); //(rgb_color){128,128,255,255};
	rgb_color col_text = tint_color(col_bg, B_LIGHTEN_MAX_TINT); //ui_color(B_PANEL_TEXT_COLOR); //(rgb_color){255,255,255,255};
	rgb_color col_text_shadow = tint_color(col_bg, B_DARKEN_MAX_TINT); //ui_color(B_SHADOW_COLOR); //(rgb_color){255,255,255,255};
	
	// Background
	SetDrawingMode( B_OP_COPY );
	
	SetHighColor( col_bg );
	
	FillRect( rect );
	
	// Text
	SetLowColor( col_bg );
	
	SetDrawingMode( B_OP_ALPHA );
	
	SetFont( be_bold_font );
	SetFontSize( TITLE_SIZE );
	
	BFont font;
	GetFont(&font);
		
	font_height fh;
	font.GetHeight( &fh );
		
//	float line_height = fh.ascent + fh.descent + fh.leading;
	
	float text_pos = fh.ascent;
	
	SetHighColor( col_text );
	DrawString( fTitle.String(), BPoint( BORDER_W,text_pos) );

	SetHighColor( col_text_shadow );
	DrawString( fTitle.String(), BPoint( BORDER_W+1,text_pos+1) );

	// content border
	SetHighColor( tint_color(col_bg,B_DARKEN_2_TINT) /*0,0,0*/ );
	
	BRect content = Bounds();
	content.InsetBy( BORDER_W, BORDER_W );
	content.top += text_pos + fh.descent + 2;
	
	BRect content_line(content);
	content_line.InsetBy(-1,-1);
	
	StrokeRect( content_line );
	
}

void
BorderView::GetPreferredSize( float * w, float * h )
{
	SetFont( be_bold_font );
	SetFontSize( TITLE_SIZE );
	
	BFont font;
	GetFont(&font);
		
	font_height fh;
	font.GetHeight( &fh );
	
	float line_height = fh.ascent + fh.descent;

	*w = 2 * BORDER_W + StringWidth( fTitle.String() );
	
	*h = 2 * BORDER_W + line_height + 3;
	*w = *h = 0;
}

float
BorderView::BorderSize()
{
return 0;
	return BORDER_W;
}
