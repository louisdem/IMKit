#ifndef MSNHANDLER_H
#define MSNHANDLER_H

#include "MSNConstants.h"

class MSNHandler {
	public:

		virtual inline		~MSNHandler() {};

		virtual status_t	StatusChanged(const char *nick, online_types type) = 0;
		virtual status_t	MessageFromUser(const char *nick, const char *msg) = 0;
		virtual status_t	UserIsTyping(const char *nick, typing_notification type) = 0;
		virtual status_t 	SSIBuddies(list<BString> buddies) = 0;
};

#endif
