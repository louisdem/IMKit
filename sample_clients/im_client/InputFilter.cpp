#include "InputFilter.h"

#include <libim/Constants.h>
#include <Messenger.h>

InputFilter::InputFilter (BTextView *owner, BMessage *msg) 
	: BMessageFilter (B_ANY_DELIVERY, B_ANY_SOURCE),
	fParent(owner),
	fMessage(new BMessage(*msg)),
	fLastTyping(0) {
}

filter_result InputFilter::Filter (BMessage *msg, BHandler **target) {

	filter_result result (B_DISPATCH_MESSAGE);

	switch (msg->what) {
		case B_MOUSE_MOVED: {
		} break;
		
		case B_MOUSE_WHEEL_CHANGED: {
			BMessenger(fParent->Window()).SendMessage(msg);
			return B_SKIP_MESSAGE;
		} break;
		
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

	static bigtime_t last_typing_message_sent=0;
	
	msg->FindString("bytes", &keyStroke);
	msg->FindInt32("modifiers", &keyModifiers);

	switch (keyStroke[0]) {
		case B_RETURN: {				
			if (keyModifiers & B_COMMAND_KEY) {
				BMessage *typing = new BMessage(IM::USER_STOPPED_TYPING);
				BMessenger(fParent->Window()).SendMessage(typing);			
			
				BMessenger(fParent->Window()).SendMessage(fMessage);
				return B_SKIP_MESSAGE;
			};
		} break;
	
		case B_ESCAPE: {
		} break;
	
		default: {
			if ( system_time() - fLastTyping > 15*1000*1000 )
			{ // only send typing messages every 15 sec
				BMessenger(fParent->Window()).SendMessage(new BMessage(IM::USER_STARTED_TYPING));
				fLastTyping = system_time();
			}
//			filter_result result (B_DISPATCH_MESSAGE);
//			return result;
		} break;
	};

	return B_DISPATCH_MESSAGE;

};
