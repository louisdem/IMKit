#include "InputFilter.h"

InputFilter::InputFilter (BTextView *owner, BMessage *msg) 
	: BMessageFilter (B_ANY_DELIVERY, B_ANY_SOURCE) {
	
	parent = owner;
	command = new BMessage(*msg);
}

filter_result InputFilter::Filter (BMessage *msg, BHandler **target) {

	filter_result result (B_DISPATCH_MESSAGE);

	switch (msg->what) {
		case B_KEY_DOWN: {
			result = HandleKeys (msg);
		} break;

		default: {
		} break;
	}
	
	return result;
};

filter_result InputFilter::HandleKeys (BMessage *msg) {
	const char *keyStroke;
	int32 keyModifiers;


	msg->FindString("bytes", &keyStroke);
	msg->FindInt32("modifiers", &keyModifiers);

	switch (keyStroke[0]) {
		case B_RETURN: {				
			if (keyModifiers & B_COMMAND_KEY) {
				parent->Window()->PostMessage(command);
				return B_SKIP_MESSAGE;
			};
		} break;
	
		case B_ESCAPE: {
		} break;
	
//		default: {
//			filter_result result (B_DISPATCH_MESSAGE);
//			return result;
//		} break;
	};

	return B_DISPATCH_MESSAGE;

};
