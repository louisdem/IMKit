/* Match Header - performs action depending on matching a header value
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Node.h>
#include <String.h>
#include <Entry.h>

#include <stdlib.h>
#include <stdio.h>

#include <Message.h>
#include <Messenger.h>

#include "InfoPopperDisplay.h"

#include "InfoPopper.h"

using namespace Zoidberg;


//class StatusView;

InfoPopperDisplay::InfoPopperDisplay(BMessage *settings) : Mail::Filter(settings) {
}

InfoPopperDisplay::~InfoPopperDisplay() {
}

status_t InfoPopperDisplay::InitCheck(BString* out_message) {
	return B_OK;
}

status_t InfoPopperDisplay::ProcessMailMessage
(
	BPositionIO** , BEntry* entry,
	BMessage* io_headers, BPath* io_folder, const char* 
) {
	BString text;
	const char *from;
	const char *subject;
	
	BNode node(entry);
	
	if ( io_headers->FindString("From", &from) != B_OK )
		from = "<unknown sender>";
	
	if ( io_headers->FindString("Subject", &subject) != B_OK )
		subject = "   <no subject>";
	
	if ( strncmp(subject, "[Spam", 5) == 0 )
		// skip spam
		return B_OK;
	
	text = from;
	text.Append(":\n   ");
	text.Append(subject);
	
	entry_ref iconRef;
	get_ref_for_path("/boot/beos/apps/BeMail",&iconRef);
	
	// send the message
	BMessage msg(InfoPopper::AddMessage);
	msg.AddString("app", "Mail daemon");
	msg.AddString("title", "New e-mail");
	msg.AddString("content", text.String());
	msg.AddRef("iconRef", &iconRef);
	msg.AddInt32("iconType", InfoPopper::Attribute);
	
	BMessenger(InfoPopperAppSig).SendMessage(&msg);
	
	return B_OK;
}

status_t descriptive_name(BMessage *settings, char *buffer) {
	sprintf(buffer, "Send message to InfoPopper");

	return B_OK;
}

Mail::Filter* instantiate_mailfilter(BMessage* settings,Mail::ChainRunner *)
{
	return new InfoPopperDisplay(settings);
}
