#include <Application.h>
#include <Entry.h>
#include <Roster.h>

#include "IMKitUtilities.h"

class PUPApp : public BApplication {
	public:
				PUPApp(void)
					: BApplication("application/x-vnd.BeClan.IMKit.PeopleURLOpener") {
				};
	
		void	RefsReceived(BMessage *msg) {
					entry_ref ref;
					int32 urlsAdded = 0;

					entry_ref htmlRef;
					be_roster->FindApp("text/html", &htmlRef);
					BPath htmlPath(&htmlRef);

					BMessage argv(B_ARGV_RECEIVED);
					argv.AddString("argv", htmlPath.Path());
					
					for (int32 i = 0; msg->FindRef("refs", i, &ref) == B_OK; i++) {						
						BPath path(&ref);
						int32 length = -1;
						char *url = ReadAttribute(path.Path(), "META:url", &length);
						if ((url != NULL) && (length > 1)) {
							url = (char *)realloc(url, (length + 1) * sizeof(char));
							url[length] = '\0';
							argv.AddString("argv", url);
							
							urlsAdded++;
						};
						if (url) free(url);
					};
					
					if (urlsAdded > 0) {
						argv.AddInt32("argc", urlsAdded + 1);
						be_roster->Launch(&htmlRef, &argv);
					};
					
					be_app_messenger.SendMessage(B_QUIT_REQUESTED);
				};
};

//#pragma mark -

int main(int argc, char *argv[]) {
	PUPApp app;
	app.Run();
};
