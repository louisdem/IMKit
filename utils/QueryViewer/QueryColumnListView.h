#ifndef QCLV_H
#define QCLV_H

#include <Roster.h>
#include <DataIO.h>
#include <Mime.h>
#include <NodeMonitor.h>
#include <PopUpMenu.h>
#include <Query.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <kernel/fs_info.h>
#include <kernel/fs_attr.h>

#include <stdio.h>
#include <sys/stat.h>

#include <map>
#include <vector>

#include "IMKitUtilities.h"
#include "CLV/ColumnListView.h"
#include "CLV/ColumnTypes.h"
#include "SVGCache.h"
#include "MenuColumns.h"

extern const char *kFolderState;
extern const int16 kAttrIndexOffset;

extern const char *kTrackerQueryVolume;
extern const char *kTrackerQueryPredicate;
extern const char *kTrackerQueryType;
extern const char *kTrackerQueryInitMime;

typedef map<BString, BString> attr_map;
typedef map<BString, uint32> type_map;
typedef map<BString, int32> ai_map;
typedef map<int32, BString> ia_map;
typedef map<entry_ref, BRow *> ref_map;
typedef vector<entry_ref> pending_stack;

typedef map<BString, BString> mime_map;

typedef vector<BVolume> vollist;
typedef vector<BQuery *> querylist;

typedef struct {
	entry_ref ref;
	node_ref nref;
} result;
typedef map<entry_ref, result> resultmap;

enum {
	qclvAddRow = 'qc01',
	qclvQueryUpdated = 'qc02',
	qclvRequestPending = 'qc03',
	qclvInvoke = 'qc04',
};

class QueryColumnListView : public BColumnListView {
	public:
							QueryColumnListView(BRect rect,
						                const char *name,
						                uint32 resizingMode,
										uint32 drawFlags,
										entry_ref ref,
										BMessage *msg = NULL,
										BMessenger *notify = NULL,
										border_style border = B_NO_BORDER,
										bool showHorizontalScrollbar = true);
							~QueryColumnListView();
							
		virtual status_t	AddRowByRef(entry_ref *ref);
		virtual status_t	RemoveRowByRef(entry_ref *ref);
				
			virtual void	MessageReceived(BMessage *msg);
			virtual void	AttachedToWindow(void);
			
			 		char	*MIMETypeFor(entry_ref *ref);
			const char		*MIMEType(void);
			const char		*Name(void);
	private:
				status_t	ExtractColumnState(BMallocIO *buffer);
				status_t	AddMIMEColumns(BMessage *msg);
			static int32	BackgroundAdder(void *arg);
			
				status_t	ExtractPredicate(entry_ref *ref, char **buffer,
								int32 *length);
				status_t	ExtractVolumes(entry_ref *ref, vollist *vols);
				entry_ref	ActionFor(entry_ref *ref);
			
		
				BMessage	*fMessage;
				BMessenger	*fNotify;
		
				entry_ref	fRef;
				BString		fMIMEString;
				attr_map	fAttributes;
				type_map	fAttrTypes;
				ai_map		fAttrIndex;
				ia_map		fIndexAttr;
				
				ref_map		fRefRows;
				
				thread_id	fThreadID;
				BMessenger	*fSelfMsgr;
			pending_stack	fPending;

//				Query stuff
				bool		fInitialFetch;
				BString		fPredicate;
				vollist		fVolumes;
				querylist	fQueries;
				resultmap	fResults;
			static mime_map	fMimeTypes;
				
};

#endif
