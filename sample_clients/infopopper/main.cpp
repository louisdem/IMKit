#include "InfoWindow.h"
#include <Application.h>

#include <libim/Helpers.h>
#include <libim/Constants.h>

#include "InfoPopper.h"

// -------------- MAIN -----------------

class InfoApp : public BApplication {
	public:
			InfoApp() : BApplication(InfoPopperAppSig) {
				fWin = new InfoWindow();
			};
			
			~InfoApp() {};
		
		void MessageReceived(BMessage *msg) {
			switch (msg->what) {
				case InfoPopper::AddMessage: {
					BMessenger(fWin).SendMessage(msg);
				} break;
				
				default:
					BApplication::MessageReceived(msg);
			};
		};
	
	private:
		InfoWindow *fWin;
};

int
main()
{
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

	InfoApp app;
	app.Run();	
}
