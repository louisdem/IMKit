#ifndef APPUSAGE_H
#define APPUSAGE_H

#include <Entry.h>
#include <Flattenable.h>
#include <String.h>

#include <map>

#include <libim/InfoPopper.h>

class Notification;
class BMessage;

typedef map<BString, Notification *> notify_t;

class AppUsage : public BFlattenable {
	public:
							AppUsage(void);
							AppUsage(entry_ref ref, const char *name,
								bool allow = true);
							~AppUsage(void);

		// BFlattenable Hooks
		bool				AllowsTypeCode(type_code code) const;
		status_t			Flatten(void *buffer, ssize_t numBytes) const;
		ssize_t				FlattenedSize(void) const;
		bool				IsFixedSize(void) const;
		type_code			TypeCode(void) const;
		status_t			Unflatten(type_code code, const void *buffer,
								ssize_t numBytes);
		
		// Public
		entry_ref			Ref(void);
		const char			*Name(void);
		bool				Allowed(const char *title, InfoPopper::info_type type);
		bool				Allowed(void);
		Notification		*NotificationAt(int32 index);
		int32				Notifications(void);
		void				AddNotification(Notification *notification);

	private:
		entry_ref			fRef;
		BString				fName;
		bool				fAllow;
		notify_t			fNotifications;
};

#endif
