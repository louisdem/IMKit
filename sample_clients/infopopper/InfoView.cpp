#include "InfoView.h"

#include <StringView.h>
#include <Window.h>
#include <Messenger.h>

InfoView::InfoView( info_type type, const char * text )
:	BView( BRect(0,0,1,1), "InfoView", B_FOLLOW_LEFT_RIGHT, 0 ),
	fRunner(NULL),
	fFilter(NULL),
	fView(NULL)
{
	float w,h;
	
	fView = new BStringView( 
		BRect(0,0,1,1), "infoText", text, B_FOLLOW_ALL
	);
	
	fView->GetPreferredSize(&w,&h);
	
	fView->ResizeTo(w,h);
	
	ResizeTo(w,h);
	
	AddChild( fView );
	
	switch ( type )
	{
		case Information:
			SetViewColor(218,218,218);
			fView->SetViewColor( ViewColor() );
			break;
		case Important:
			SetViewColor(255,255,255);
			fView->SetViewColor( ViewColor() );
			break;
		case Error:
			SetViewColor(255,0,0);
			fView->SetViewColor( ViewColor() );
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
	
	fView->AddFilter(fFilter);
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
	fView->GetPreferredSize(w,h);
}
