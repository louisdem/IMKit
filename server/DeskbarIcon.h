#ifndef DESKBAR_ICON_H
#define DESKBAR_ICON_H

#include <View.h>
#include <Resources.h>
#include <Bitmap.h>
#include <list>
#include <map>
#include <Messenger.h>
#include <MessageRunner.h>
#include <TextView.h>
#include <string>
#include <PopUpMenu.h>
#include <String.h>

#include "../common/IMKitUtilities.h"
#include "../common/BubbleHelper.h"
#include "AwayMessageWindow.h"

#include <libim/Manager.h>
#include <be/kernel/fs_attr.h>

class _EXPORT IM_DeskbarIcon : public BView
{
	public:
		IM_DeskbarIcon();
		IM_DeskbarIcon( BMessage * );
		virtual ~IM_DeskbarIcon();
		
		virtual status_t Archive( BMessage * archive, bool deep = true );
		static BArchivable * Instantiate( BMessage * archive );
		
		virtual void Draw( BRect );
		
		virtual void MessageReceived( BMessage * );
		
		virtual void MouseDown( BPoint );
		virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *msg);
		
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		
	private:
		enum {
			SET_STATUS		= 'set1',
			SET_ONLINE		= 'set2',
			SET_AWAY		= 'set3',
			SET_OFFLINE		= 'set4',

			
			OPEN_SETTINGS	= 'opse',
			RELOAD_SETTINGS = 'upse',
			
			CLOSE_IM_SERVER = 'imqu',
			
			SETTINGS_WINDOW_CLOSED = 'swcl'
		};
			
		void				_init();
//		BBitmap 			*GetBitmap( const char * name );
		void				reloadSettings();
		
		BResources			fResource;

		BBitmap				*fCurrIcon;
		BBitmap				*fModeIcon;
		BBitmap				*fOnlineIcon;
		BBitmap				*fOfflineIcon;
		BBitmap				*fAwayIcon;
		BBitmap 			*fFlashIcon;
		
		int					fStatus;
		
		// for flashing
		int					fFlashCount, fBlink;
		list<BMessenger>	fMsgrs;
		BMessageRunner *	fMsgRunner;
		
		BWindow *			fSettingsWindow;
		
		// settings
		bool				fShouldBlink;

		bool				fDirtyStatus;	// Need to re-fetch the Statuses
		bool				fDirtyMenu;		// Need to re-make the right click Menu
		BubbleHelper *		fTip;
		BString				fTipText;
		map<string, string>	fStatuses;
		BPopUpMenu			*fMenu;
		
};

extern "C" _EXPORT BView * instantiate_deskbar_item();

#define DESKBAR_ICON_NAME "IM_DeskbarIcon"

#endif
