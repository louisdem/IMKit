#ifndef IPMESSAGE_H
#define IPMESSAGE_H

#include <stdio.h>
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
		
		void					PrintToStream(FILE *out = stdout);

	private:
		info_type				fType;
		BString					fApp;
		BString					fTitle;
		BString					fContent;
		BString					fMessageID;
		float					fProgress;
};

#endif
