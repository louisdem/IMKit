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
#include <map>

#include "InfoView.h"
#include "BorderView.h"
#include <libim/InfoPopper.h>

#include <PropertyInfo.h>

class AppGroupView;
class AppUsage;
class SettingsFile;

typedef map<BString, AppGroupView *> appview_t;
typedef map<BString, AppUsage *> appfilter_t;

extern const float kEdgePadding;
extern const float kSmallPadding;
extern const float kCloseSize;
extern const float kExpandSize;
extern const float kPenSize;

// -------------- INFO WINDOW -----------------

class InfoWindow : public BWindow {
	public:
		enum {
			ResizeToFit = 'IWrf'
		};
		
							InfoWindow(void);
							~InfoWindow(void);

		// Hook functions		
		bool				QuitRequested(void);
		void				MessageReceived(BMessage * );
		void 				WorkspaceActivated(int32, bool);
		BHandler 			*ResolveSpecifier(BMessage *, int32, BMessage *,
								int32, const char *);
		
		// Public functions
		int16				IconSize(void);
		int32				DisplayTime(void);
		infoview_layout 	Layout(void);
		float				ViewWidth(void);
		BPath				SettingsPath(void);

		void				ResizeAll(void);
		
	private:
		void				PopupAnimation(float, float);
		void				WriteDefaultSettings(BNode *node,
								bool writeWidth = true, bool writeIcon = true,
								bool writeTimeout = true, bool writeLayout = true);
		void				LoadSettings(bool start_monitor = false);
		void				LoadAppFilters(bool startmonitor = false);
		void				SaveAppFilters(void);
		
		vector<InfoView*>	fInfoViews;
		deskbar_location	fDeskbarLocation;
		BorderView			*fBorder;
		
		appview_t			fAppViews;

		BPath				fSettingsPath;

		BString				fStatusText;
		BString				fMessageText;
		
		float				fWidth;
		int16				fIconSize;
		int32				fDisplayTime;
		infoview_layout		fLayout;
		
		appfilter_t			fAppFilters;
};

extern property_info main_prop_list[];

#endif
