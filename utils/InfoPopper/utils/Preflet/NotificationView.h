#ifndef NOTIFICATIONVIEW_H
#define NOTIFICATIONVIEW_H

// Be API
#include <String.h>

// STL
#include <map>

// Local
#include "SettingView.h"

class AppView;
class BBox;
class BCheckBox;
class BColumnListView;
class BDateColumn;
class BMenu;
class BMenuField;
class BMenuItem;
class BStringColumn;
class BTextControl;

class AppUsage;

typedef map<BString, AppUsage *> appusage_t;
typedef map<BString, AppView *> appview_t;

class NotificationView : public SettingView {
	public:
							NotificationView(BRect bounds);
							~NotificationView(void);

	// BView Hooks
		void				AttachedToWindow(void);
		void				MessageReceived(BMessage *msg);
		void				Draw(BRect rect);
	
	// SettingView hooks
		status_t			Save(void);
		status_t			Load(void);

	private:
		status_t			LoadAppUsage(void);
		void				SetupView(BMenuItem *item);

		BBox				*fAppBox;
		BMenuField			*fAppField;
		BMenu				*fAppMenu;
		
		appusage_t			fAppUsage;
		BMenuItem			*fCurrentAppMenu;
		AppView				*fCurrentAppView;
		appview_t			fAppView;
		BRect				fChildRect;
		
};

#endif
