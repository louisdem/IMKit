#ifndef IM_SAMPLE_CLIENT_H
#define IM_SAMPLE_CLIENT_H

#include <libim/Manager.h>

#include <Application.h>
#include <Window.h>
#include <Messenger.h>
#include <Entry.h>
#include <TextControl.h>
#include <TextView.h>

class ChatWindow;

class MyApp : public BApplication
{
	public:
		MyApp();
		virtual ~MyApp();
		
		virtual bool QuitRequested();
		
		virtual void MessageReceived( BMessage * );
		virtual void RefsReceived( BMessage * );
		
		bool IsQuiting();
		
		void Flash( BMessenger );
		void NoFlash( BMessenger );
		
	private:
		ChatWindow * 	findWindow( entry_ref & );
		
		IM::Manager *	fMan;
		bool			fIsQuiting;
};

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
		
	private:
		enum { SEND_MESSAGE = 1 };
		
		entry_ref	fEntry;
		char		fName[512];
		
		BTextControl	* fInput;
		BTextView		* fText;
		
		IM::Manager		* fMan;
		bool			fChangedNotActivated;
		char			fTitleCache[512];
};

#endif
