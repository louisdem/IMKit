#include "InfoWindow.h"
#include <Application.h>


// -------------- MAIN -----------------

int
main()
{
	BApplication app("application/x-vnd.beclan.IM_InfoPopper");
	
	InfoWindow * win = new InfoWindow();
	
	app.Run();
}
