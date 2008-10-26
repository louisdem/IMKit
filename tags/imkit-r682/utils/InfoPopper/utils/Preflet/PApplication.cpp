#include "PApplication.h"

#include "PWindow.h"

//#pragma mark Constructor

PApplication::PApplication(void)
	: BApplication("application/x-vnd.beclan.InfoPopper.Preflet") {
};

PApplication::~PApplication(void) {
};

//#pragma mark Hooks

void PApplication::ReadyToRun(void) {
	fWindow = new PWindow();
};

bool PApplication::QuitRequested(void) {
	return true;
};

void PApplication::MessageReceived(BMessage *msg) {
	BApplication::MessageReceived(msg);
};

