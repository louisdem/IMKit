#include "InfoWindow.h"

#include <cmath>
#include <String.h>
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
	
	fBorder = new BorderView( Bounds(), "IM info" );
	
	AddChild( fBorder );
	
	Show();
	Hide();
	
	fDeskbarLocation = BDeskbar().Location();
	
	BMessage settings;
	bool temp;
	im_load_client_settings("InfoPopper", &settings);
	if ( !settings.FindString("app_sig") )
		settings.AddString("app_sig", "application/x-vnd.beclan.IM_InfoPopper");
	if ( settings.FindBool("auto_start", &temp) != B_OK )
		settings.AddBool("auto_start", true );
	if (settings.FindString("status_text", &fStatusText) != B_OK) {
		fStatusText = "$nickname$ is now $status$";
		settings.AddString("status_text", fStatusText);
	};
	if (settings.FindString("msg_text", &fMessageText) != B_OK) {
		fMessageText = "$nickname$ says:\n$shortmsg$";
		settings.AddString("msg_text", fMessageText);
	};
	
	printf("Message: %s\nStatus: %s\n", fMessageText.String(), fStatusText.String());
	
	im_save_client_settings("InfoPopper", &settings);
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
InfoWindow::WorkspaceActivated( int32, bool active )
{
	if ( active )
	{ // move to correct position
		ResizeAll();
	}
}

void
InfoWindow::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		case IM::SETTINGS_UPDATED:
		{	
			BMessage settings;
			im_load_client_settings("InfoPopper", &settings);
			if (settings.FindString("status_text", &fStatusText) != B_OK) {
				fStatusText = "$nickname$ is now $status$";
			};
			if (settings.FindString("msg_text", &fMessageText) != B_OK) {
				fMessageText = "$nickname$ says:\n$shortmsg$";
			};			
		} break;	
	
		case IM::ERROR:
		case IM::MESSAGE:
		{
			int32 im_what=IM::ERROR;
			
			if ( msg->FindInt32( "im_what", &im_what ) != B_OK )
				im_what = IM::ERROR;
			
			BString text("");
			
			entry_ref ref;
			
			msg->FindRef("contact", &ref);
			
			char contactname[512];
			char nickname[512];
			char email[512];
			
			IM::Contact contact(&ref);
			
			if (contact.GetName(contactname, sizeof(contactname) ) != B_OK ) {
				strcpy(contactname, "<unknown contact>");
			};
			if (contact.GetEmail(email, sizeof(email)) != B_OK) {
				strcpy(email, "<unknown email>");
			};
			if (contact.GetNickname(nickname, sizeof(nickname)) != B_OK) {
				strcpy(nickname, "<unknown nick>");
			};
			
			InfoView::info_type type = InfoView::Information;
			
			switch ( im_what )
			{
				case IM::ERROR:
				{
					BMessage error;
					int32 error_what = -1;
					if ( msg->FindMessage("message", &error ) == B_OK )
					{
						error.FindInt32("im_what", &error_what);
					}
					
					if ( error_what != IM::USER_STARTED_TYPING && 
						error_what != IM::USER_STOPPED_TYPING )
					{ // we ignore errors due to typing notifications.
						text << "Error: " << msg->FindString("error");
						type = InfoView::Error;
					}
				}	break;
				
				case IM::MESSAGE_RECEIVED: {
					text = fMessageText;
					BString message = msg->FindString("message");
					BString shortMessage = message;
					
					if ( shortMessage.FindFirst("\n") >= 0 )
						shortMessage.Truncate( shortMessage.FindFirst("\n") );
									
					if ( shortMessage.Length() > 30 ) {
						shortMessage.Truncate(27);
						shortMessage.Append("...");
					}
					
					text.ReplaceAll("$shortmsg$", shortMessage.String());
					text.ReplaceAll("$message$", message.String());
				
					type = InfoView::Important;
				}	break;
				
				case IM::STATUS_CHANGED: {						
					text = fStatusText;

					text.ReplaceAll("$status$", msg->FindString("status"));
				}	break;
			}

			text.ReplaceAll("\\n", "\n");
			text.ReplaceAll("$nickname$", nickname);
			text.ReplaceAll("$contactname$", contactname);
			text.ReplaceAll("$email$", email);
			text.ReplaceAll("$id$", msg->FindString("id"));

			
			if ( text != "" )
			{ // a message to display
				//printf("Displaying message <%s>\n", text.String() );
				InfoView * view = new InfoView( type, text.String() );
				
				fInfoViews.push_back(view);
				
				fBorder->AddChild( view );
				
				ResizeAll();
			}
		}	break;
		
		case REMOVE_VIEW:
		{
			void * _ptr;
			msg->FindPointer("view", &_ptr);
			
			InfoView * info = reinterpret_cast<InfoView*>(_ptr);
			
			fBorder->RemoveChild(info);
			
			fInfoViews.remove(info);
			
			ResizeAll();
		}	break;
		
		default:
			BWindow::MessageReceived(msg);
	}
}

void
InfoWindow::ResizeAll()
{
	if ( fInfoViews.size() == 0 )
	{
		if ( !IsHidden() )
			Hide();
		return;
	}
	
	float borderw, borderh;
	fBorder->GetPreferredSize(&borderw, &borderh);
	
	float curry=borderh-borderw/2, maxw=0;
	BView * view = NULL;
	
	for ( list<InfoView*>::iterator i=fInfoViews.begin(); i != fInfoViews.end(); i++ )
	{
		float pw,ph;
		
		(*i)->MoveTo(borderw/2, curry);
		(*i)->GetPreferredSize(&pw,&ph);
		
		curry += (*i)->Bounds().Height()+1;
		
		if ( pw > maxw )
			maxw = pw;
		
		(*i)->ResizeTo( Bounds().Width() - borderw, (*i)->Bounds().Height() );
	}
	
	ResizeTo( maxw + borderw, curry-1+borderw/2);
	
	PopupAnimation( Bounds().Width(), Bounds().Height() );
}

void
InfoWindow::PopupAnimation(float width, float height) {
	float x,y,sx,sy;
	float pad = 2;
	BDeskbar deskbar;
	BRect frame = deskbar.Frame();
	
	switch ( deskbar.Location() ) {
		case B_DESKBAR_TOP:
			// put it just under, top right corner
			sx = frame.right;
			sy = frame.bottom+pad;
			y = sy;
			x = sx-width-pad;
			break;
		case B_DESKBAR_BOTTOM:
			// put it just above, lower left corner
			sx = frame.right;
			sy = frame.top-height-pad;
			y = sy;
			x = sx - width-pad;
			break;
		case B_DESKBAR_LEFT_TOP:
			// put it just to the right of the deskbar
			sx = frame.right+pad;
			sy = frame.top-height;
			x = sx;
			y = frame.top+pad;
			break;
		case B_DESKBAR_RIGHT_TOP:
			// put it just to the left of the deskbar
			sx = frame.left-width-pad;
			sy = frame.top-height;
			x = sx;
			y = frame.top+pad;
			break;
		case B_DESKBAR_LEFT_BOTTOM:
			// put it to the right of the deskbar.
			sx = frame.right+pad;
			sy = frame.bottom;
			x = sx;
			y = sy-height-pad;
			break;
		case B_DESKBAR_RIGHT_BOTTOM:
			// put it to the left of the deskbar.
			sx = frame.left-width-pad;
			sy = frame.bottom;
			y = sy-height-pad;
			x = sx;
			break;	
		default: break;
	}
	
	MoveTo(x, y);
	
	if (IsHidden() && fInfoViews.size() != 0) {
		Show();
	}
}
