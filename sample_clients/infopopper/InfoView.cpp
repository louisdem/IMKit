#include "InfoView.h"

#include <StringView.h>

InfoView::InfoView( info_type type, const char * text )
:	BView( BRect(0,0,1,1), "InfoView", B_FOLLOW_LEFT_RIGHT, 0 )
{
	float w,h;
	
	BStringView * view = new BStringView( 
		BRect(0,0,1,1), "infoText", text, B_FOLLOW_ALL
	);
	
	view->GetPreferredSize(&w,&h);
	
	view->ResizeTo(w,h);
	
	ResizeTo(w,h);
	
	AddChild( view );
	
	switch ( type )
	{
		case Information:
			SetViewColor(218,218,218);
			view->SetViewColor( ViewColor() );
			break;
		case Important:
			SetViewColor(255,255,255);
			view->SetViewColor( ViewColor() );
			break;
		case Error:
			SetViewColor(255,0,0);
			view->SetViewColor( ViewColor() );
			break;
	}
}

InfoView::~InfoView()
{
}

void
InfoView::AttachedToWindow()
{
	BMessage msg(REMOVE_VIEW);
	msg.AddPointer("view", this);
	
	fRunner = new BMessageRunner( BMessenger(this), &msg, 5*1000*1000, 1 );
}

