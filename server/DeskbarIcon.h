#ifndef DESKBAR_ICON_H
#define DESKBAR_ICON_H

#include <View.h>
#include <Resources.h>
#include <Bitmap.h>
#include <list>
#include <Messenger.h>
#include <MessageRunner.h>

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
		
		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		
	private:
		enum {
			SET_ONLINE		= 'set1',
			SET_AWAY		= 'set2',
			SET_OFFLINE		= 'set3',
			
			OPEN_SETTINGS	= 'opse',
			RELOAD_SETTINGS = 'upse',
			
			SETTINGS_WINDOW_CLOSED = 'swcl'
		};
		
		void			_init();
		BBitmap *		GetBitmap( const char * name );
		void			reloadSettings();
		
		BResources		fResource;
		BBitmap *		fCurrIcon;
		
		BBitmap *		fStdIcon;
		BBitmap *		fFlashIcon;
		
		// for flashing
		int					fFlashCount, fBlink;
		list<BMessenger>	fMsgrs;
		BMessageRunner *	fMsgRunner;
		
		BWindow *			fSettingsWindow;
		
		// settings
		bool		fShouldBlink;
};

extern "C" _EXPORT BView * instantiate_deskbar_item();

#define DESKBAR_ICON_NAME "IM_DeskbarIcon"

#endif
