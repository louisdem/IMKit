#ifndef INFO_WINDOW_H
#define INFO_WINDOW_H

#include <Deskbar.h>
#include <Window.h>
#include <String.h>
#include <Entry.h>
#include <Application.h>

#include <cmath>
#include <vector>

#include "InfoView.h"
#include "BorderView.h"
#include <libim/InfoPopper.h>

#include <PropertyInfo.h>


// -------------- INFO WINDOW -----------------

class InfoWindow : public BWindow
{
	public:
		enum {
			ResizeToFit = 'IWrf'
		};
		
		InfoWindow();
		~InfoWindow();
		
		bool	QuitRequested();
		void	MessageReceived( BMessage * );
		void 	WorkspaceActivated( int32, bool );
		
		BHandler * ResolveSpecifier(BMessage *, int32 , BMessage *, int32, const char *);
		
	private:
		void	ResizeAll();
		void	PopupAnimation(float, float);
		
		vector<InfoView*>		fInfoViews;
		deskbar_location 	fDeskbarLocation;
		BorderView			* fBorder;

		BString				fStatusText;
		BString				fMessageText;
};

extern property_info main_prop_list[];

#endif
