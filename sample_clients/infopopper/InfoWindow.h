#ifndef INFO_WINDOW_H
#define INFO_WINDOW_H

#include <Window.h>
#include <list>
#include "InfoView.h"
#include <libim/Manager.h>

// -------------- INFO WINDOW -----------------

class InfoWindow : public BWindow
{
	public:
		InfoWindow();
		~InfoWindow();
		
		bool	QuitRequested();
		void	MessageReceived( BMessage * );
	
	private:
		void	ResizeAll();
		
		IM::Manager 		* fMan;
		list<InfoView*>		fInfoViews;
};

#endif
