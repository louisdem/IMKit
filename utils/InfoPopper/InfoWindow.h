#ifndef INFO_WINDOW_H
#define INFO_WINDOW_H

#include <Deskbar.h>
#include <Window.h>
#include <String.h>
#include <Entry.h>
#include <Application.h>

#include <cmath>
#include <list>

#include "InfoView.h"
#include "BorderView.h"
#include <libim/InfoPopper.h>

// -------------- INFO WINDOW -----------------

class InfoWindow : public BWindow
{
	public:
		InfoWindow();
		~InfoWindow();
		
		bool	QuitRequested();
		void	MessageReceived( BMessage * );
		void 	WorkspaceActivated( int32, bool );
		
	private:
		void	ResizeAll();
		void	PopupAnimation(float, float);
		
		list<InfoView*>		fInfoViews;
		deskbar_location 	fDeskbarLocation;
		BorderView			* fBorder;

		BString				fStatusText;
		BString				fMessageText;
};

#endif
