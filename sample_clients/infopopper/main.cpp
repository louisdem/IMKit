#include "InfoWindow.h"
#include <Application.h>

#include <libim/Helpers.h>
#include <libim/Constants.h>

// -------------- MAIN -----------------

int
main()
{
	BApplication app("application/x-vnd.beclan.IM_InfoPopper");
	
	// Save settings template
	BMessage autostart;
	autostart.AddString("name", "auto_start");
	autostart.AddString("description", "Auto-start");
	autostart.AddInt32("type", B_BOOL_TYPE);
	autostart.AddBool("default", true);
	
	BMessage appsig;
	appsig.AddString("name", "app_sig");
	appsig.AddString("description", "Application signature");
	appsig.AddInt32("type", B_STRING_TYPE);
	appsig.AddBool("default", "application/x-vnd.beclan.IM_InfoPopper");
	
	BMessage status;
	status.AddString("name", "status_text");
	status.AddString("description", "Status change text");
	status.AddInt32("type", B_STRING_TYPE);
	status.AddString("default", "$nickname$ is now $status$");
	
	BMessage msg;
	msg.AddString("name", "msg_text");
	msg.AddString("description", "Message received text");
	msg.AddInt32("type", B_STRING_TYPE);
	msg.AddString("default", "$nickname$ says $shortmsg$");
	
	BMessage tmplate(IM::SETTINGS_TEMPLATE);
	tmplate.AddMessage("setting", &autostart);
	tmplate.AddMessage("setting", &appsig);
	tmplate.AddMessage("setting", &status);
	tmplate.AddMessage("setting", &msg);
	
	im_save_client_template("InfoPopper", &tmplate);
	
	InfoWindow * win = new InfoWindow();
	app.Run();
}
