#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include <Window.h>
#include <Message.h>
#include <libim/Manager.h>

class SettingsWindow : public BWindow
{
	public:
		SettingsWindow();
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
