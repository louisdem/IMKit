#ifndef ZOIDBERG_RULE_FILTER_H
#define ZOIDBERG_RULE_FILTER_H
/* RuleFilter - performs action depending on matching a header value
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Message.h>
#include <List.h>
#include <MailAddon.h>
#include <String.h>

#include "StringMatcher.h"

class BMailFilter;

class InfoPopperDisplay : public BMailFilter {
	public:
							InfoPopperDisplay(BMessage* settings, BMailChainRunner* runner);
		virtual				~InfoPopperDisplay();
					
		virtual status_t	InitCheck(BString* out_message = NULL);
		
		virtual status_t ProcessMailMessage(BPositionIO** io_message,
											   BEntry* io_entry,
											   BMessage* io_headers,
											   BPath* io_folder,
											   const char* io_uid);

	protected:
		BString		fPath;
};

#endif	/* ZOIDBERG_RULE_FILTER_H */
