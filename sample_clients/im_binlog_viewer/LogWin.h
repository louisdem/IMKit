#ifndef LOGWIN_H
#define LOGWIN_H

#include <Window.h>
#include <View.h>
#include <Button.h>
#include <Entry.h>
#include <File.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <String.h>
#include <Roster.h>

#include <stdlib.h>

#include "CLV/ColumnListView.h"
#include "CLV/ColumnTypes.h"
#include "../../../common/IMKitUtilities.h"

#include <libim/Constants.h>
#include <libim/Contact.h>

#include <map>

typedef map<BString, BBitmap *> iconmap;

class LogWin : public BWindow {
	public:
		enum {
			coDate = 0,
			coProtocol = 1,
			coSender = 2,
			coContents = 3,
			coType = 4
		};
	
						LogWin(entry_ref contact, BRect size);
						~LogWin(void);

			void 		MessageReceived(BMessage *msg);
			bool 		QuitRequested(void);

	private:
		static int32	GenerateContents(void *arg);

			entry_ref	fEntryRef;

		BView			*fView;
		BColumnListView	*fCLV;
		BDateColumn		*fDate;
		BBitmapColumn	*fProtocol;
		BStringColumn	*fSender;
		BStringColumn	*fContents;
		BStringColumn	*fType;
		
		iconmap			fIcons;
		
		thread_id		fThreadID;

		IM::Contact		fContact;
		BString			fNick;
		BString			fName;
};

#endif
