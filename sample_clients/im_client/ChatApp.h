#ifndef CHATAPP_H
#define CHATAPP_H

#include "main.h"

class BApplication;
class ChatWindow;

class ChatApp : public BApplication
{
	public:
						ChatApp();
			virtual		~ChatApp();
		
		virtual bool	QuitRequested();
		
		virtual void 	MessageReceived( BMessage * );
		virtual void 	RefsReceived( BMessage * );
		
				bool	IsQuiting();
		
				void	Flash( BMessenger );
				void	NoFlash( BMessenger );
		
	private:
		ChatWindow		*findWindow( entry_ref & );
		
		IM::Manager		*fMan;
		bool			fIsQuiting;
};

#endif
