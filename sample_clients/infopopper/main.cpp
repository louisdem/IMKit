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
	
	BMessage tmplate(IM::SETTINGS_TEMPLATE);
	tmplate.AddMessage("setting", &autostart);
	tmplate.AddMessage("setting", &appsig);
	
	im_save_client_template("InfoPopper", &tmplate);
	
	// Make sure default settings are there
	BMessage settings;
	bool temp;
	im_load_client_settings("InfoPopper", &settings);
	if ( !settings.FindString("app_sig") )
		settings.AddString("app_sig", "application/x-vnd.beclan.IM_InfoPopper");
	if ( settings.FindBool("auto_start", &temp) != B_OK )
		settings.AddBool("auto_start", true );
	im_save_client_settings("InfoPopper", &settings);
	// done with template and settings.

	InfoWindow * win = new InfoWindow();
	
	app.Run();
}
