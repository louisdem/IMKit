#include "InfoWindow.h"

InfoWindow::InfoWindow()
:	BWindow(BRect(10,10,20,20), "InfoWindow", B_BORDERED_WINDOW,
	B_AVOID_FRONT|B_AVOID_FOCUS) {

	SetWorkspaces( 0xffffffff );
	
	fBorder = new BorderView(Bounds(), "InfoPopper");
	
	AddChild( fBorder );
	
	Show();
	Hide();
	
	fDeskbarLocation = BDeskbar().Location();
};

InfoWindow::~InfoWindow(void) {
};

bool InfoWindow::QuitRequested(void) {
	BMessenger(be_app).SendMessage( B_QUIT_REQUESTED );
	return BWindow::QuitRequested();
}

void InfoWindow::WorkspaceActivated(int32 workspace, bool active) {
//	Ensure window is in the correct position
	if ( active ) ResizeAll();
};

void InfoWindow::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case InfoPopper::AddMessage: {
			int8 type;
			const char *message;
			const char *title;
			
			if (msg->FindInt8("type", &type) != B_OK) type = InfoPopper::Information;
			if (msg->FindString("content", &message) != B_OK) return;
			if (msg->FindString("title", &title) != B_OK) return;
			
			const char *messageID;
			if ( msg->FindString("messageID",&messageID) == B_OK )
			{ // message ID present, remove current message if present
				list<InfoView*>::iterator i;
				
				for ( i=fInfoViews.begin(); i!=fInfoViews.end(); i++ )
				{
					if ( (*i)->HasMessageID(messageID) )
					{
						(*i)->RemoveSelf();
						delete *i;
						fInfoViews.remove(*i);
						break;
					}
				}
			}
			
			BString msgText = title;
			msgText << ":\n" << message;
			msgText.ReplaceAll("\n", "\n  ");
			
			InfoView *view = new InfoView(
				(InfoPopper::info_type)type, 
				msgText.String(), 
				new BMessage(*msg)
			);
			
			fInfoViews.push_back(view);
			
			fBorder->AddChild( view );
			
			ResizeAll();
		} break;
		
		case REMOVE_VIEW: {
			void *_ptr;
			msg->FindPointer("view", &_ptr);
			
			InfoView *info = reinterpret_cast<InfoView*>(_ptr);
			
			fBorder->RemoveChild(info);
			
			fInfoViews.remove(info);
			
			ResizeAll();
		} break;
		
		default: {
			BWindow::MessageReceived(msg);
		};
	};
};

void InfoWindow::ResizeAll(void) {
	if (fInfoViews.size() == 0) {
		if (!IsHidden()) Hide();
		return;
	};
	
	float borderw, borderh;
	fBorder->GetPreferredSize(&borderw, &borderh);
	
	float curry = borderh - fBorder->BorderSize(), maxw = 150;

	for (list<InfoView*>::iterator i = fInfoViews.begin(); i != fInfoViews.end();
		i++) {
		float pw,ph;
		
		(*i)->MoveTo(fBorder->BorderSize(), curry);
		(*i)->GetPreferredSize(&pw,&ph);
		
		curry += (*i)->Bounds().Height()+1;
		
		if (pw > maxw) maxw = pw;
		
		(*i)->ResizeTo(Bounds().Width() - fBorder->BorderSize() * 2,
			(*i)->Bounds().Height());
	};
	
	ResizeTo(maxw + fBorder->BorderSize() * 2, curry - 1 + fBorder->BorderSize());
	
	PopupAnimation(Bounds().Width(), Bounds().Height());
};

void InfoWindow::PopupAnimation(float width, float height) {
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
	};
};
