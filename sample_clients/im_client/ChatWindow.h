#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include "main.h"
#include "../../common/IMKitUtilities.h"

#include <FindDirectory.h>

#include <ScrollView.h>
#include <NodeMonitor.h>
#include <stdio.h>
#include <Button.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Beep.h>
#include <String.h>
#include <StringView.h>
#include <Roster.h>
#include <Bitmap.h>
#include <MessageRunner.h>

#include "IconMenuItem.h"
#include "StatusBar.h"

#include <be/kernel/fs_attr.h>

#include "ObjectList.h"

#define C_URL						0
#define C_TIMESTAMP					1
#define C_TEXT						2
#define C_OWNNICK					3
#define C_OTHERNICK					4
#define C_ACTION					5
#define C_SELECTION					6
#define C_TIMESTAMP_DUMMY			7	// Needed to fake a TS
#define MAX_COLORS					8

#define F_URL						0
#define F_TEXT						1
#define F_TIMESTAMP					2
#define F_ACTION					3
#define F_TIMESTAMP_DUMMY			4	//Needed to fake TS
#define MAX_FONTS					5

extern const char *kImNewMessageSound;

class BWindow;
class BTextView;
class BScrollView;

class InputFilter;
class ResizeView;
class RunView;
class Theme;

class StatusBar;

class ChatWindow : public BWindow
{
	public:
		ChatWindow(entry_ref &, int32 iconBarSize = kLargeIcon, bool command = true);
		~ChatWindow();

		void MessageReceived( BMessage * );
		bool QuitRequested();
		
		virtual void FrameResized( float, float );
		virtual void WindowActivated( bool );
		
		bool handlesRef( entry_ref & );
		void reloadContact();
		void startNotify();
		void stopNotify();
		
		status_t SaveSettings(void);
		status_t LoadSettings(void);
		
	private:
		void BuildProtocolMenu(void);
		void startTypingTimer();
		void stopTypingTimer();
		
		enum { 
			SEND_MESSAGE	= 1,
			
			SHOW_INFO		= 100,
			BLOCK,
			EMAIL,
			AUTH,
			
			CLEAR_TYPING	= 1000
		 };
		
		entry_ref	fEntry;
		char		fName[512];
		
		BTextView		* fInput;
		RunView			* fText;
		ResizeView		*fResize;
		
		BScrollView		*fInputScroll;
		BScrollView		*fTextScroll;
		
		InputFilter		*fFilter;
		IM::Manager		* fMan;
		bool			fChangedNotActivated;
		char			fTitleCache[512];
		
		BMessage		fWindowSettings;
		Theme			*fTheme;
		
		float			fFontHeight;
		BView			*fDock;
		
		BMenuField		*fProtocolMenu;
		StatusBar		*fStatusBar;
		BStringView		*fInfoView;
		
		BMessageRunner	*fTypingTimer;
};

#endif

