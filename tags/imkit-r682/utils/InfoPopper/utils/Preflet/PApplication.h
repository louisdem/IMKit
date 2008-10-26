#ifndef PAPPLICATION_H
#define PAPPLICATION_H

#include <Application.h>

class PWindow;
class BApplication;

class PApplication : public BApplication {
	public:
	
							PApplication(void);
							~PApplication(void);

		// Hooks
		virtual bool		QuitRequested(void);
		virtual void		MessageReceived(BMessage *msg);
		virtual void		ReadyToRun(void);
				
	private:
		PWindow				*fWindow;
};

#endif

