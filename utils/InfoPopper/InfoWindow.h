#ifndef INFO_WINDOW_H
#define INFO_WINDOW_H

#include <Deskbar.h>
#include <Window.h>
#include <String.h>
#include <Entry.h>
#include <Application.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <Alert.h>

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
		
		int16	IconSize(void);
		int32	DisplayTime(void);
		infoview_layout Layout(void);
		float	ViewWidth(void);
		
	private:
		void	ResizeAll();
		void	PopupAnimation(float, float);
		void	WriteDefaultSettings(BNode *node, bool writeWidth = true,
					bool writeIcon = true, bool writeTimeout = true,
					bool writeLayout = true);
		void	LoadSettings( bool start_monitor = false );
		
		vector<InfoView*>	fInfoViews;
		deskbar_location	fDeskbarLocation;
		BorderView			* fBorder;

		BString				fStatusText;
		BString				fMessageText;
		
		float				fWidth;
		int16				fIconSize;
		int32				fDisplayTime;
		infoview_layout		fLayout;
};

extern property_info main_prop_list[];

#endif
