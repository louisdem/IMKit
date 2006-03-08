#ifndef IPMESSAGE_H
#define IPMESSAGE_H

#include <stdio.h>
#include <Entry.h>
#include <String.h>
#include <libim/InfoPopper.h>

using namespace InfoPopper;

class IPMessage {
	public:
								IPMessage(info_type type = Information);
								~IPMessage(void);

		void					Type(info_type type);
		info_type				Type(void);

		void					Application(const char *name);
		const char				*Application(void);
		
		void					Title(const char *title);
		const char				*Title(void);
		
		void					Content(const char *content);
		const char				*Content(void);
		
		void					MessageID(const char *id);
		const char				*MessageID(void);
		
		void					Progress(float progress);
		float					Progress(void);
		
		void					MainIcon(entry_ref ref);
		entry_ref				MainIcon(void);
		void					MainIconType(int32 type);
		int32					MainIconType(void);
		
		void					OverlayIcon(entry_ref ref);
		entry_ref				OverlayIcon(void);
		void					OverlayIconType(int32 type);
		int32					OverlayIconType(void);
		
		void					PrintToStream(FILE *out = stdout);

	private:
		info_type				fType;
		BString					fApp;
		BString					fTitle;
		BString					fContent;
		BString					fMessageID;
		float					fProgress;
		entry_ref				fMainIcon;
		int32					fMainIconType;
		entry_ref				fOverlayIcon;
		int32					fOverlayIconType;
};

#endif
