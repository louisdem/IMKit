#ifndef IM_LOGGER_H
#define IM_LOGGER_H

#include <libim/Manager.h>

#include <Application.h>
#include <stdio.h>

class LoggerApp : public BApplication
{
	public:
		LoggerApp();
		~LoggerApp();
		
		void MessageReceived( BMessage * );
		
	private:
		IM::Manager *	fMan;
		FILE *			fFile;
};

#endif
