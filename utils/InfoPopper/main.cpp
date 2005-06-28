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

const char *kTypeNames[] = {
	"information",
	"important",
	"error",
	"progress",
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
			printf("InfoPopper [--type <type>] [--app \"App Name\"] [--title \"Message Title\"] [--messageID \"Msg ID\"] [--progress <float>] [--timeout <secs>] [--icon [overlay:][attr:]<icon file>] \"Message text\"\n");
			printf("	<type>: ");
			for (int j=0; kTypeNames[j]; j++) {
				printf(kTypeNames[j+1]?"%s|":"%s\n", kTypeNames[j]);
			};
			
			printf("	<icon file>: path to a bitmap file\n");
			printf("	attr:<icon file>: path to a file with BEOS:*:STD_ICON attributes\n");
		}
		
		void ArgvReceived(int32 argc, char **argv) {
			int i, j;
			BMessage msg(InfoPopper::AddMessage);
			InfoPopper::info_type type = InfoPopper::Information;
			const char *app = "Command Line";
			const char *title = "Something Happened";
			BString content;
			float progress = 0.0;
			const char *messageID = NULL;
			int32 timeout = 0;
			const char *iconFile = NULL;
			const char *onClickApp;
			entry_ref onClickFile;
			const char *onClickArgv;
			entry_ref onClickRef;
			for (i = 1; i < argc; i++) {
				//printf("argv[%d] = %s\n", i, argv[i]);
				if (!strcmp(argv[i], "--help")) {
					return Usage();
				} else if (!strcmp(argv[i], "--type")) {
					if (++i >= argc) return Usage();
					type = (InfoPopper::info_type)atoi(argv[i]);
					
					for (j=0; kTypeNames[j]; j++) {
						if (!strncmp(kTypeNames[j], argv[i], strlen(argv[i]))) {
							type = (InfoPopper::info_type)j;
						};
					};
				} else if (!strcmp(argv[i], "--app")) {
					if (++i >= argc)
						return Usage();
					app = argv[i];
				} else if (!strcmp(argv[i], "--title")) {
					if (++i >= argc) return Usage();
					title = argv[i];
				} else if (!strcmp(argv[i], "--messageID")) {
					if (++i >= argc) return Usage();
					messageID = argv[i];
					msg.AddString("messageID", messageID);
				} else if (!strcmp(argv[i], "--progress")) {
					if (++i >= argc) return Usage();
					progress = atof(argv[i]);
					msg.AddFloat("progress", progress);
				} else if (!strcmp(argv[i], "--timeout")) {
					if (++i >= argc) return Usage();
					timeout = atol(argv[i]);
					msg.AddInt32("timeout", timeout);
#if 0
				} else if (!strcmp(argv[i], "--empty")) { /* like alert's --empty */
					iconFile = NULL;
				} else if (!strcmp(argv[i], "--info")) {
					iconFile = "info";
				} else if (!strcmp(argv[i], "--idea")) {
				} else if (!strcmp(argv[i], "--warning")) {
				} else if (!strcmp(argv[i], "--stop")) {
#endif
				} else if (!strcmp(argv[i], "--icon")) {
					bool isOverlay = false;
					if (++i >= argc) return Usage();
					iconFile = argv[i];

					if (!strncmp(iconFile, "overlay:", 8)) {
						iconFile += 8;
						isOverlay = true;
					};
					if (!strncmp(iconFile, "attr:", 5)) {
						iconFile += 5;
						msg.AddInt32(isOverlay?"overlayIconType":"iconType", InfoPopper::Attribute);
					} else {
						msg.AddInt32(isOverlay?"overlayIconType":"iconType", InfoPopper::Contents);
					};
					
					entry_ref ref;
					if (get_ref_for_path(iconFile, &ref) < B_OK) {
						fprintf(stderr, "bad icon path\n");
						return Usage();
					};
					msg.AddRef(isOverlay?"overlayIconRef":"iconRef", &ref);
				} else if (!strcmp(argv[i], "--onClickApp")) {
					if (++i >= argc) return Usage();
					onClickApp = argv[i];
					msg.AddString("onClickApp", onClickApp);
				} else if (!strcmp(argv[i], "--onClickFile")) {
					if (++i >= argc) return Usage();
					if (get_ref_for_path(argv[i], &onClickFile) < B_OK) {
						fprintf(stderr, "bad path\n");
						return Usage();
					};
					msg.AddRef("onClickFile", &onClickFile);
				} else if (!strcmp(argv[i], "--onClickArgv")) {
					if (++i >= argc) return Usage();
					onClickArgv = argv[i];
					msg.AddString("onClickArgv", onClickArgv);
				} else if (!strcmp(argv[i], "--onClickRef")) {
					if (++i >= argc) return Usage();
					if (get_ref_for_path(argv[i], &onClickRef) < B_OK) {
						fprintf(stderr, "bad path\n");
						return Usage();
					}
					msg.AddRef("onClickRef", &onClickRef);
				} else {
					if (content.Length()) content << "\n";
					content << argv[i];
				}
			}
			msg.AddInt8("type", (int8)type);
			msg.AddString("app", app);
			msg.AddString("title", title);
			msg.AddString("content", content);
			be_app_messenger.SendMessage(&msg);
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
			if (strcmp(prop, "message") == 0) {
				BMessenger msgr(fWin);
				
				msgr.SendMessage(msg,fWin);
				
				return NULL;
			};
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
