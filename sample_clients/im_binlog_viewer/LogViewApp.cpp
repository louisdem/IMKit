#include "LogViewApp.h"

extern "C" void process_refs(entry_ref dir_ref, BMessage *msg, void *) {
	msg->what = B_REFS_RECEIVED;
	msg->AddRef("dir_ref", &dir_ref);
	
	be_roster->Launch("application/x-vnd.BeClan.im_binlog_viewer", msg);
};

int main(void) {
	LogViewApp app;
	app.Run();
};

LogViewApp::LogViewApp(void)
	: BApplication("application/x-vnd.BeClan.im_binlog_viewer") {
};

LogViewApp::~LogViewApp(void) {
};

void LogViewApp::RefsReceived(BMessage *msg) {
	entry_ref ref;
	BNode node;
	attr_info info;
	
	for (int32 i = 0; msg->FindRef("refs", i, &ref ) == B_OK; i++ ) {
		node = BNode(&ref);
		char *type = ReadAttribute(node, "BEOS:TYPE");
		if (strcmp(type, "application/x-person") == 0) {
			if (node.GetAttrInfo("IM:connections", &info) == B_OK) {
				if (node.GetAttrInfo("IM:binarylog", &info) == B_OK) {
					new LogWin(ref, BRect(40, 40, 600, 400));
				} else {
					BAlert *alert = new BAlert("Invalid", "This People file contains no Logs",
						"Bummer!", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
						B_STOP_ALERT);
					alert->SetShortcut(0, B_ESCAPE);
					alert->Go();
				};
			} else {
				BAlert *alert = new BAlert("Invalid", "This People file contains no Logs",
					"Bummer!", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
					B_STOP_ALERT); 
				alert->SetShortcut(0, B_ESCAPE);
				alert->Go();
			};
		};
		free(type);
	};
};
						
void LogViewApp::MessageReceived(BMessage *msg) {
	BApplication::MessageReceived(msg);
};

