#include "URLTextView.h"

#include <Roster.h>
#include <stdio.h>
#include <string.h>

URLTextView::URLTextView(BRect r, const char * name, BRect text_rect, uint32 follow, uint32 flags)
:	BTextView(r, name, text_rect, follow, flags)
{
}

URLTextView::~URLTextView()
{
}

bool
is_whitespace( const char c )
{
	return ( c == '\t' || c == ' ' || c == '\n' );
}

void
URLTextView::MouseUp( BPoint where )
{
	BTextView::MouseUp(where);
	return;

	int32 sel_start, sel_end;
	
	GetSelection( &sel_start, &sel_end );
	
	if ( sel_start != sel_end )
	{
		// something is selected, quit
		BTextView::MouseUp(where);
		return;
	}
	
	const char * text = Text();
	
	int32 offset = OffsetAt(where);
	
	if ( offset < 0 )
	{
		BTextView::MouseUp(where);
		return;
	}
	
	int32 start = offset, end = offset;
	
	while ( start > 0 && !is_whitespace(text[start-1]) )
		start--;
	
	while( text[end] && !is_whitespace(text[end]) )
		end++;
	
	if ( start == end )
	{
		// can't find word
		BTextView::MouseUp(where);
		return;
	}
	
	char * word = new char[end-start+5];
	
	strncpy( word, &text[start], end-start );
	
	word[end-start] = 0;
	
	if ( strncmp(word, "www.", 4) == 0 )
	{ // add "http://" so N+ opens it
		char temp[1024];
		strcpy(temp,word);
		sprintf(word, "http://%s", temp);
	}
	
	char * argv[2] = { word, NULL };
	
	if ( strncmp(word, "http://", 7) == 0 )
	{ // URL!
		be_roster->Launch( "text/html", 1, argv );
	}
	
	delete[] word;
	
	BTextView::MouseUp(where);
}

void
URLTextView::FrameResized( float w, float h )
{
	printf("Frame event.\n");
	SetTextRect( Bounds() );
}

void
URLTextView::MakeFocus( bool focus)
{
	BTextView::MakeFocus(focus);
//	BTextView::MakeFocus(false);
}

#if B_BEOS_VERSION > B_BEOS_VERSION_5
status_t
URLTextView::UISettingsChanged(const BMessage *changes, uint32 flags) {
	printf("Flags %i\n", flags);
	changes->PrintToStream();

	SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
	
	Invalidate();
	
	return B_OK;
};
#endif
