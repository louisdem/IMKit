#include "QueryLooper.h"

QueryLooper::QueryLooper(const char *predicate, vollist vols, const char *name = NULL,
	BHandler *notify = NULL, uint32 command = 0)
	: BLooper(name) {
	
	fMenu = NULL;
	fCommand = command;
	fName = name;
	fNotify = notify;
	fPredicate = predicate;
	fVolumes = vols;
	
	Run();

	BMessenger(this).SendMessage(msgInitialFetch);
};

QueryLooper::~QueryLooper(void) {
	querylist::iterator vIt;
	for (vIt = fQueries.begin(); vIt != fQueries.end(); vIt++) {
		(*vIt)->Clear();
		delete (*vIt);
	};
	delete fMenu;
};

void QueryLooper::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case B_QUERY_UPDATE: {
			int32 opcode = 0;
			if (msg->FindInt32("opcode", &opcode) != B_OK) return;
					
			switch (opcode) {
				case B_ENTRY_CREATED: {
					entry_ref ref;
					const char *name;
				
					msg->FindInt32("device", &ref.device); 
					msg->FindInt64("directory", &ref.directory); 
					msg->FindString("name", &name); 
					ref.set_name(name);
									
					fERefs.push_back(ref);				
				} break;
				
				case B_ENTRY_REMOVED: {
					ereflist::iterator eIt;
					for (eIt = fERefs.begin(); eIt != fERefs.end(); eIt++) {
						BEntry entry(eIt);
						if (entry.InitCheck() != B_OK) {
							fERefs.erase(eIt);
							break;
						};
					};
				} break;
			};
		
			CreateMenu();
		
			if (fNotify) {
				BMessage notify(QUERY_UPDATED);
				notify.AddString("qlName", fName);
				
				BMessenger(fNotify).SendMessage(notify);
			};
		} break;
		
		case msgInitialFetch: {
			vollist::iterator vIt;
			for (vIt = fVolumes.begin(); vIt != fVolumes.end(); vIt++) {
				BVolume vol = (*vIt);
				BQuery *query = new BQuery();
		
				query->SetPredicate(fPredicate.String());
				query->SetTarget(this);
				query->SetVolume(&vol);
		
				query->Fetch();
		
				entry_ref ref;
				while (query->GetNextRef(&ref) == B_OK) {
					fERefs.push_back(ref);
				};
				
				fQueries.push_back(query);
			};
			
			CreateMenu();
		} break;
		
		default: {
			BLooper::MessageReceived(msg);
		};
	};
};

int32 QueryLooper::CountEntries(void) {
	return fERefs.size();
};

entry_ref QueryLooper::EntryAt(int32 index) {
	entry_ref ref;
	if ((index >= 0) && (index < CountEntries())) {
		ref = fERefs[index];
	}
	
	return ref;
};

BMenu *QueryLooper::Menu(void) {
	return fMenu;
};

// #pragma mark -

void QueryLooper::CreateMenu(void) {
	ereflist::iterator eIt;
	
	if ((fMenu) &&
		((fMenu->Superitem() == NULL) && (fMenu->Supermenu() == NULL))) delete fMenu;
	fMenu = new BMenu(fName.String());
	
	for (eIt = fERefs.begin(); eIt != fERefs.end(); eIt++) {
		entry_ref ref = (*eIt);
		BMessage *msg = new BMessage(fCommand);
		msg->AddRef("fileRef", &ref);
		
		BBitmap *icon = ReadNodeIcon(BPath(&ref).Path());
		IconMenuItem *item = new IconMenuItem(icon , ref.name, "", msg);
		
		fMenu->AddItem(item);
	};
};
