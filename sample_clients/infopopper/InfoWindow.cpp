#include "InfoWindow.h"

#include <Entry.h>
#include <Application.h>
#include <libim/Constants.h>
#include <libim/Contact.h>

InfoWindow::InfoWindow()
:	BWindow( 
		BRect(10,10,20,20), 
		"InfoWindow", 
		B_BORDERED_WINDOW, 
		B_AVOID_FRONT|B_AVOID_FOCUS
	)
{
	fMan = new IM::Manager( BMessenger(this) );
	fMan->StartListening();
	
	SetWorkspaces( 0xffffffff );
	
	Show();
	Hide();
}

InfoWindow::~InfoWindow()
{
	fMan->StopListening();
	
	BMessenger(fMan).SendMessage( B_QUIT_REQUESTED );
}

bool
InfoWindow::QuitRequested()
{
	BMessenger(be_app).SendMessage( B_QUIT_REQUESTED );
	return true;
}

void
InfoWindow::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case IM::MESSAGE:
		{
			int32 im_what=0;
			
			msg->FindInt32( "im_what", &im_what );
			
			BString text("");
			
			entry_ref ref;
			
			msg->FindRef( "contact", &ref );
			
			char contact_name[512];
			
			IM::Contact contact(&ref);
			
			if ( contact.GetName( contact_name, sizeof(contact_name) ) != B_OK )
			{
				strcpy(contact_name, "<unknown contact>");
			}
			
			InfoView::info_type type = InfoView::Information;
			
			switch ( im_what )
			{
				case IM::MESSAGE_RECEIVED:
				{
					text << "Message received from " << contact_name;
					type = InfoView::Important;
				}	break;
				
				case IM::STATUS_CHANGED:
				{
					text << contact_name;
					text << " is now " << msg->FindString("status");
				}	break;
			}
			
			if ( text != "" )
			{ // a message to display
				//printf("Displaying message <%s>\n", text.String() );
				InfoView * view = new InfoView( type, text.String() );
				
				if ( fInfoViews.size() == 0 )
				{
					Show();
				}
				
				fInfoViews.push_back(view);
				
				float w = Bounds().Width();
				
				if ( view->Bounds().Width() > Bounds().Width() )
					w = view->Bounds().Width();
				
				if ( fInfoViews.size() == 1 )
				{
					ResizeTo( w, view->Bounds().Height() );
				} else
				{
					ResizeTo( w, Bounds().Height()+view->Bounds().Height() );
				}
				
				AddChild( view );
				
				view->MoveTo( 0, Bounds().bottom - view->Bounds().Height() );
			}
		}	break;
		
		case REMOVE_VIEW:
		{
			void * _ptr;
			msg->FindPointer("view", &_ptr);
			
			InfoView * info = reinterpret_cast<InfoView*>(_ptr);
			
			RemoveChild(info);
			
			fInfoViews.remove(info);
			
			if ( fInfoViews.size() == 0 )
			{
				Hide();
			} else
			{
				float curry=0, maxw=0;
				BView * view = NULL;
			
				for ( list<InfoView*>::iterator i=fInfoViews.begin(); i != fInfoViews.end(); i++ )
				{
					(*i)->MoveTo(0, curry);
					curry += (*i)->Bounds().Height()+1;
					if ( (*i)->Bounds().Width() > maxw )
						maxw = (*i)->Bounds().Width();
				}
			
				ResizeTo( maxw, curry );
			}
		}	break;
		
		default:
			BWindow::MessageReceived(msg);
	}
}

