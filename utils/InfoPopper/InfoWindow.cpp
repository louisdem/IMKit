#include "InfoWindow.h"

#include <algorithm>

// 
property_info main_prop_list[] = {
	{ "message", {B_GET_PROPERTY, B_COUNT_PROPERTIES, 0},{B_INDEX_SPECIFIER, 0}, "get a message"},
	{ "message", {B_CREATE_PROPERTY, 0},{B_DIRECT_SPECIFIER, 0}, "create a message"},
	0 // terminate list
};

InfoWindow::InfoWindow()
:	BWindow(BRect(10,10,20,20), "InfoWindow", B_BORDERED_WINDOW,
	B_AVOID_FRONT|B_AVOID_FOCUS) {

	SetWorkspaces( 0xffffffff );
	
	fBorder = new BorderView(Bounds(), "InfoPopper");
	
	AddChild( fBorder );
	
	Show();
	Hide();
	
	fDeskbarLocation = BDeskbar().Location();
	
	fWidth = 300.0f;
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
		case ResizeToFit: {
			ResizeAll();
		}	break;
		
		case B_CREATE_PROPERTY:
		case InfoPopper::AddMessage: {		
			int8 type;
			const char *message;
			const char *title;
			
			if (msg->FindInt8("type", &type) != B_OK) type = InfoPopper::Information;
			if (msg->FindString("content", &message) != B_OK)
			{
				printf("Error: missing content\n");
				msg->PrintToStream();
				return;
			}
			if (msg->FindString("title", &title) != B_OK) 
			{
				printf("Error: missing title\n");
				msg->PrintToStream();
				return;
			}
			
			const char *messageID;
			if ( msg->FindString("messageID",&messageID) == B_OK )
			{ // message ID present, remove current message if present
				vector<InfoView*>::iterator i;
				
				for ( i=fInfoViews.begin(); i!=fInfoViews.end(); i++ )
				{
					if ( (*i)->HasMessageID(messageID) )
					{
						(*i)->RemoveSelf();
						delete *i;
						fInfoViews.erase(i);
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
			
			vector<InfoView*>::iterator i = find(fInfoViews.begin(), fInfoViews.end(), info);
			if ( i != fInfoViews.end() )
				fInfoViews.erase(i);
			
			delete info;
			
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
	
	float curry = borderh - fBorder->BorderSize(), maxw = 250;

	for (vector<InfoView*>::reverse_iterator i = fInfoViews.rbegin(); i != fInfoViews.rend();
		i++) {
		float pw,ph;
		
		(*i)->MoveTo(fBorder->BorderSize(), curry);
		(*i)->GetPreferredSize(&pw,&ph);
		
		curry += (*i)->Bounds().Height()+1;
		
		if (pw > maxw) maxw = pw;
		
		(*i)->ResizeTo(Bounds().Width() - fBorder->BorderSize() * 2,
			(*i)->Bounds().Height());
	};
	
	//ResizeTo(maxw + fBorder->BorderSize() * 2, curry - 1 + fBorder->BorderSize());
	
	ResizeTo( fWidth, curry - 1 + fBorder->BorderSize());
	
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

BHandler * InfoWindow::ResolveSpecifier(BMessage *msg, int32 index, BMessage *spec, int32 form, const char *prop) {
	BPropertyInfo prop_info(main_prop_list);
	printf("Looking for property %s\n", prop);
	if ( strcmp(prop,"message") == 0 ) {
		
		printf("Matching specifier..\n");
		
		if ( msg->what == B_CREATE_PROPERTY )
		{
			printf("Create\n");
			msg->PopSpecifier();
			return this;
		} else
		{
			int32 i;
			if ( spec->FindInt32("index",&i) != B_OK ) i = -1;
		
			if ( i >= 0 && i < fInfoViews.size() ) {
				printf("Found message\n");
				msg->PopSpecifier();
				return fInfoViews[i];
			}
		
			printf("Index out of range: %ld\n",i);
			msg->PrintToStream();
			return NULL;
		}
	}
	return BWindow::ResolveSpecifier(msg, index, spec, form, prop);
};
