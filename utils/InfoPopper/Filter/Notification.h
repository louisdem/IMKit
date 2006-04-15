#ifndef NOTIFICATION_H
#define NOTIFICATION_H

#include <Flattenable.h>
#include <String.h>

#include <stdlib.h>

#include <libim/InfoPopper.h>

using namespace InfoPopper;

class Notification : public BFlattenable {
	public:
							Notification(void);
							Notification(const char *title, info_type type,
								bool enabled = true);
							~Notification(void);
							
		// BFlattenable Hooks
		bool				AllowsTypeCode(type_code code) const;
		status_t			Flatten(void *buffer, ssize_t numBytes) const;
		ssize_t				FlattenedSize(void) const;
		bool				IsFixedSize(void) const;
		type_code			TypeCode(void) const;
		status_t			Unflatten(type_code code, const void *buffer,
								ssize_t numBytes);
								
		// Public
		const char 			*Title(void);
		info_type			InfoType(void);
		void				InfoType(info_type type);
		time_t				LastReceived(void);
		bool				Allowed(void);
		
		void				SetTimeStamp(time_t time);
		void				UpdateTimeStamp(void);
	
	private:
		BString				fTitle;
		info_type			fType;
		bool				fEnabled;
		time_t				fLastReceived;
};

#endif
