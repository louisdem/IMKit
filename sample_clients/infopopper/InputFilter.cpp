#include "InputFilter.h"

#include "InfoView.h"

InputFilter::InputFilter (BView *owner) 
	: BMessageFilter (B_ANY_DELIVERY, B_ANY_SOURCE),
	fOwner(owner)
{
}

filter_result InputFilter::Filter (BMessage *msg, BHandler **target) {

	filter_result result = B_DISPATCH_MESSAGE;

	switch (msg->what) {
		case B_MOUSE_UP: {
			BMessage remove(REMOVE_VIEW);
			remove.AddBool("clicked", true);
			BMessenger(fOwner).SendMessage(&remove);
		} break;

		default: {
		} break;
	}
	
	return result;
};
