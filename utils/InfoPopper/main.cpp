#include "InfoWindow.h"
#include <Application.h>
#include <Beep.h>
#include <stdio.h>

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
						if (type < (sizeof(kSoundNames)/sizeof(const char *)))
							system_beep(kSoundNames[type]);
					};
					BMessenger(fWin).SendMessage(msg);
				} break;
				
				case InfoPopper::GetIconSize: {
					BMessage reply(B_REPLY);
					reply.AddInt16("iconSize", fWin->IconSize());
					
					msg->SendReply(&reply);
				} break;
				
				default:
					BApplication::MessageReceived(msg);
			};
		};
		
		void Usage(void) {
			printf("InfoPopper [--type <type>] [--app \"App Name\"] [--title \"Message Title\"] [--messageID \"Msg ID\"] [--progress <float>] [--timeout <secs>] [--icon <icon name>] \"Message text\"\n");
			printf("	<type>: 0 to 3\n");
		}
		
		void ArgvReceived(int32 argc, char **argv) {
			int i;
			BMessage msg(InfoPopper::AddMessage);
			InfoPopper::info_type type = InfoPopper::Information;
			const char *app = "Command Line";
			const char *title = "Something Happened";
			BString content;
			float progress = 0.0;
			const char *messageID = NULL;
			int32 timeout = 0;
			for (i = 1; i < argc; i++) {
				//printf("argv[%d] = %s\n", i, argv[i]);
				if (!strcmp(argv[i], "--help")) {
					return Usage();
				} else if (!strcmp(argv[i], "--type")) {
					if (++i >= argc)
						return Usage();
					type = (InfoPopper::info_type)atoi(argv[i]);
				} else if (!strcmp(argv[i], "--app")) {
					if (++i >= argc)
						return Usage();
					app = argv[i];
				} else if (!strcmp(argv[i], "--title")) {
					if (++i >= argc)
						return Usage();
					title = argv[i];
				} else if (!strcmp(argv[i], "--messageID")) {
					if (++i >= argc)
						return Usage();
					messageID = argv[i];
					msg.AddString("messageID", messageID);
				} else if (!strcmp(argv[i], "--progress")) {
					if (++i >= argc)
						return Usage();
					progress = atof(argv[i]);
					msg.AddFloat("progress", progress);
				} else if (!strcmp(argv[i], "--timeout")) {
					if (++i >= argc)
						return Usage();
					timeout = atol(argv[i]);
					msg.AddInt32("timeout", timeout);
				} else if (!strcmp(argv[i], "--icon")) {
					if (++i >= argc)
						return Usage();
					fprintf(stderr, "--icon unimplemented yet\n");
				} else {
					if (content.Length())
						content << "\n";
					content << argv[i];
				}
			}
			msg.AddInt8("type", (int8)type);
			msg.AddString("app", app);
			msg.AddString("title", title);
			msg.AddString("content", content);
			//msg.PrintToStream();
			be_app_messenger.SendMessage(&msg);
			//BApplication::ArgvReceived(argc, argv);
		}
		
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
	(void)argc;
	(void)argv[0];
	int32 i = 0;
	while (kSoundNames[i] != NULL) add_system_beep_event(kSoundNames[i++], 0);

	InfoApp app;
	app.Run();
};
