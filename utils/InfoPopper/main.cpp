#include "InfoWindow.h"
#include <Application.h>

#include <libim/InfoPopper.h>

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

//#pragma mark -

int main(int argc, char *argv[]) {
	InfoApp app;
	app.Run();
};
