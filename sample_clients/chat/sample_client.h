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
		~MyApp();
		
		bool QuitRequested();
		
		void MessageReceived( BMessage * );
		
		bool IsQuiting();
		
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
		
		bool handlesRef( entry_ref & );
		void reloadContact();
		
	private:
		enum { SEND_MESSAGE = 1 };
		
		entry_ref	fEntry;
		char		fName[512];
		
		BTextControl	* fInput;
		BTextView		* fText;
		
		IM::Manager		* fMan;
};

class SettingsWindow : public BWindow
{
	public:
		SettingsWindow( const char * protocol );
		~SettingsWindow();
		
		void MessageReceived( BMessage * );
		bool QuitRequested();
		
	private:
		void rebuildUI();
		
		enum {
			APPLY_SETTINGS	= 1234,
			ALTER_STATUS,
			SELECT_PROTOCOL
		};
		BMessage	fTemplate;
		char		fProtocol[128];
		
		IM::Manager	* fMan;
};

#endif
