#ifndef INFO_WINDOW_H
#define INFO_WINDOW_H

#include <Deskbar.h>
#include <Window.h>
#include <list>
#include "InfoView.h"
#include <libim/Manager.h>

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
		
		IM::Manager 		* fMan;
		list<InfoView*>		fInfoViews;
		deskbar_location 	fDeskbarLocation;
};

#endif
