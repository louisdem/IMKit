#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include "main.h"

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
#include <Roster.h>

#include <be/kernel/fs_attr.h>

extern const char *kImNewMessageSound;

class BWindow;
class BTextView;
class BScrollView;

class InputFilter;
class ResizeView;

class ChatWindow : public BWindow
{
	public:
		ChatWindow( entry_ref & );
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
		enum { SEND_MESSAGE = 1 };
		
		entry_ref	fEntry;
		char		fName[512];
		
		BTextView		* fInput;
		BTextView		* fText;
		ResizeView		*fResize;
		
		BScrollView		*fInputScroll;
		BScrollView		*fTextScroll;
		
		InputFilter		*fFilter;
		IM::Manager		* fMan;
		bool			fChangedNotActivated;
		char			fTitleCache[512];

		BMessage		fWindowSettings;
};

#endif
