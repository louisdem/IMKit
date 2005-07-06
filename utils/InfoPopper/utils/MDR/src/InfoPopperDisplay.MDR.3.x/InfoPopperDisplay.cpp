/* Match Header - performs action depending on matching a header value
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Node.h>
#include <String.h>
#include <Entry.h>
#include <Path.h>
#include <Query.h>
#include <VolumeRoster.h>
#include <Volume.h>

#include <libim/InfoPopper.h>
#include <libim/Contact.h>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <Message.h>
#include <Messenger.h>

#include <ChainRunner.h>
#include <mail_util.h>

#include "InfoPopperDisplay.h"

#include "RegEx.h"

#include "InfoPopper.h"

//using namespace Zoidberg;


//class StatusView;

extern void SubjectToThread (BString &string);
extern void extract_address_name(BString &header);
extern time_t ParseDateWithTimeZone (const char *DateString);

BString generateFileName( BMessage * headers )
{
	time_t			dateAsTime;
	const time_t   *datePntr;
	ssize_t			dateSize;
	char			numericDateString [40];
	struct tm   	timeFields;
	BString			worker;

	// Generate a file name for the incoming message.  See also
	// Message::RenderTo which does a similar thing for outgoing messages.

	BString name = headers->FindString("Subject");
	SubjectToThread (name); // Extract the core subject words.
	if (name.Length() <= 0)
		name = "No Subject";
	if (name[0] == '.')
		name.Prepend ("_"); // Avoid hidden files, starting with a dot.

	// Convert the date into a year-month-day fixed digit width format, so that
	// sorting by file name will give all the messages with the same subject in
	// order of date.
	if ( !headers->FindString("Date") )
	{
		headers->AddString("Date", "Fri, 1 Jan 1970 00:00:00 +0000 CET");
	}
	
	dateAsTime = ParseDateWithTimeZone(headers->FindString("Date"));
	if (dateAsTime == -1)
		dateAsTime = time (NULL); // Use current time if it's undecodable.
	
	localtime_r(&dateAsTime, &timeFields);
	sprintf(numericDateString, "%04d%02d%02d%02d%02d%02d",
		timeFields.tm_year + 1900,
		timeFields.tm_mon + 1,
		timeFields.tm_mday,
		timeFields.tm_hour,
		timeFields.tm_min,
		timeFields.tm_sec);
	name << " " << numericDateString;

	worker = headers->FindString("From");
	extract_address_name(worker);
	name << " " << worker;

	name.Truncate(222);	// reserve space for the uniquer

	// Get rid of annoying characters which are hard to use in the shell.
	name.ReplaceAll('/','_');
	name.ReplaceAll('\'','_');
	name.ReplaceAll('"','_');
	name.ReplaceAll('!','_');
	name.ReplaceAll('<','_');
	name.ReplaceAll('>','_');
	while (name.FindFirst("  ") >= 0) // Remove multiple spaces.
		name.Replace("  " /* Old */, " " /* New */, 1024 /* Count */);

	return name;
}


InfoPopperDisplay::InfoPopperDisplay(BMessage* settings, BMailChainRunner* runner) 
: BMailFilter(settings)
{
	fPath = runner->Chain()->MetaData()->FindString("path");
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
	
	io_headers->PrintToStream();
	printf("path: %s\n", io_folder->Path() );
	
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
	
	// figure out final name of email
	BString dest = "";
	if (io_headers->FindString("DESTINATION", &dest) != B_OK) dest = fPath;

	if ( dest.Length() > 0 && dest[dest.Length()-1] != '/' )
		dest << "/";
	dest << generateFileName(io_headers);
	entry_ref mail_ref;
	BEntry(dest.String()).GetRef(&mail_ref);
	printf("email path: %s\n", dest.String());

	bool iconAdded = false;
	BMessage msg(InfoPopper::AddMessage);
	msg.AddString("app", "Mail daemon");
	msg.AddString("title", "New e-mail");
	msg.AddString("content", text.String());
	msg.AddRef("onClickFile", &mail_ref);

	RegEx reg("<((?:.*?)@(?:.*?))>");
	if (reg.Search(from, strlen(from)) == true) {
		BString address = reg.Match(1);
		BString addUpper = address;
		BString addLower = address;
		BString predicate;
		
		addUpper.ToUpper();
		addLower.ToLower();
		int32 length = address.Length();
		
		for (int32 i = 0; i < length; i++) {
			if (isalpha(address[i])) {
				predicate << "[";
				predicate << addUpper[i];
				predicate << addLower[i];
				predicate << "]";
			} else {
				predicate << address[i];
			};
		};
		
		predicate.Prepend("((BEOS:TYPE==\"application/x-person\")&&(META:email==\"");
		predicate.Append("\"))");
		
		BVolumeRoster roster;
		BVolume vol;
		
		roster.Rewind();
		while (roster.GetNextVolume(&vol) == B_OK) {
			if ((vol.InitCheck() != B_OK) || (vol.KnowsQuery() != true)) continue;
			
			BQuery query;
			query.SetPredicate(predicate.String());
			query.SetVolume(&vol);			
			query.Fetch();
			
			entry_ref ref;
			
			if (query.GetNextRef(&ref) == B_OK) {
				IM::Contact contact(ref);

				BMessage sizeReply;
				int16 iconSize;
				
				BMessenger(InfoPopperAppSig).SendMessage(InfoPopper::GetIconSize, &sizeReply);
				if (sizeReply.FindInt16("iconSize", &iconSize) != B_OK) iconSize = 48;

				BBitmap *icon = contact.GetBuddyIcon("general", iconSize);
				if (icon) {
					iconAdded = true;
					BMessage image;
					icon->Archive(&image);
					msg.AddMessage("icon", &image);
	
					delete icon;
				};

				break;
			};
			
			query.Clear();
		};
	};
	

	if (iconAdded == false) {
		msg.AddRef("iconRef", &iconRef);
		msg.AddInt32("iconType", InfoPopper::Attribute);
	} else {
		msg.AddRef("overlayIconRef", &iconRef);
		msg.AddInt32("overlayIconType", InfoPopper::Attribute);
	};

	// send the message	
	BMessenger(InfoPopperAppSig).SendMessage(&msg);
	
	return B_OK;
}

status_t descriptive_name(BMessage *settings, char *buffer) {
	sprintf(buffer, "Send message to InfoPopper");

	return B_OK;
}

BMailFilter* instantiate_mailfilter(BMessage* settings,BMailChainRunner* runner)
{
	return new InfoPopperDisplay(settings,runner);
}
