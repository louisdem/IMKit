#include "InfoWindow.h"
#include <Application.h>
#include <Beep.h>

#include <libim/InfoPopper.h>

const char *kSoundNames[] = {
	"InfoPopper: Information Message",
	"InfoPopper: Important Message",
	"InfoPopper: Error Message",
	"InfoPopper: Progress Message",
	NULL
};

class InfoApp : public BApplication {
	public:
			InfoApp() : BApplication(InfoPopperAppSig) {
				fWin = new InfoWindow();
			};
			
			~InfoApp() {};
		
		void MessageReceived(BMessage *msg) {
			switch (msg->what) {
				case InfoPopper::AddMessage: {
					int8 type = 0;
					if (msg->FindInt8("type", &type) == B_OK) {
						//printf("Playing sound type %ld %s\n", (int32)(type), kSoundNames[type]);
						system_beep(kSoundNames[type]);
					};
					BMessenger(fWin).SendMessage(msg);
				} break;
				
				default:
					BApplication::MessageReceived(msg);
			};
		};
		
		status_t GetSupportedSuites(BMessage *msg) {
			msg->AddString("suites", "suite/x-vnd.beclan.InfoPopper");
			BPropertyInfo prop_info(main_prop_list);
			msg->AddFlat("messages", &prop_info);
			return BApplication::GetSupportedSuites(msg); 		
		};
		
		BHandler * ResolveSpecifier(BMessage *msg, int32 index, BMessage *spec, int32 form, const char *prop) {
			BPropertyInfo prop_info(main_prop_list);
			printf("app: looking for property %s\n",prop);
			if ( strcmp(prop,"message") == 0 ) {
				// forward to fWin
				BMessenger msgr(fWin);
				
				msgr.SendMessage(msg,fWin);
				
				return NULL;
			}
			return BApplication::ResolveSpecifier(msg, index, spec, form, prop);
		};
		
	private:
		InfoWindow *fWin;
};

//#pragma mark -

int main(int argc, char *argv[]) {
	int32 i = 0;
	while (kSoundNames[i] != NULL) add_system_beep_event(kSoundNames[i++], 0);

	InfoApp app;
	app.Run();
};
