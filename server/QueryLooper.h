#ifndef QUERYLOOPER_H
#define QUERYLOOPER_H

#include <Entry.h>
#include <Looper.h>
#include <MessageFilter.h>
#include <NodeMonitor.h>
#include <Menu.h>
#include <Query.h>
#include <String.h>
#include <Volume.h>

#include "../common/IconMenuItem.h"
#include "../common/IMKitUtilities.h"

#include <map>
#include <vector>

typedef map<entry_ref, BBitmap *> ereflist;
typedef vector<BVolume> vollist;
typedef vector<BQuery *> querylist;

const uint32 QUERY_UPDATED = 'qlup';

class QueryLooper : public BLooper {
	public:
						QueryLooper(const char *predicate, vollist vols,
							const char *name = NULL, BHandler *notify = NULL,
							uint32 command = 0);
		virtual			~QueryLooper(void);

		virtual	void	MessageReceived(BMessage *msg);

				int32	CountEntries(void);
			entry_ref	EntryAt(int32 index);
			ereflist	*List(void) { return &fERefs; };
			
			BMenu		*Menu(void);
		
	private:
			enum {
				msgInitialFetch = 'ql01'
			};
	
			void		CreateMenu(void);
			BMenu		*fMenu;
			uint32		fCommand;
	
			BHandler	*fNotify;	

			ereflist	fERefs;
			
			BString		fName;

			vollist		fVolumes;
			querylist	fQueries;
			BString		fPredicate;
};

#endif
