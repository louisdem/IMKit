#include "QueryLooper.h"

#include <stdio.h>

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

					BBitmap *icon = ReadNodeIcon(BPath(&ref).Path());
					fERefs[ref] = icon;
				} break;
				
				case B_ENTRY_REMOVED: {
					ereflist::iterator eIt;
							
					for (eIt = fERefs.begin(); eIt != fERefs.end(); eIt++) {
						BEntry entry(&eIt->first);
						if (entry.InitCheck() != B_OK) {
							printf("\t%s is invalid! Removing\n", eIt->first.name);
							delete eIt->second;
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
					BBitmap *icon = ReadNodeIcon(BPath(&ref).Path());
					fERefs[ref] = icon;
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
//	if ((index >= 0) && (index < CountEntries())) {
//		
//		ref = fERefs[index];
//	}
//	
	return ref;
};

BMenu *QueryLooper::Menu(void) {
	printf("%s have %i entry_refs\n", fName.String(), fERefs.size());
	return fMenu;
};

// #pragma mark -

void QueryLooper::CreateMenu(void) {
	ereflist::iterator eIt;
	
//	if ((fMenu) &&
//		((fMenu->Superitem() == NULL) && (fMenu->Supermenu() == NULL))) delete fMenu;
	fMenu = new BMenu(fName.String());
	
	for (eIt = fERefs.begin(); eIt != fERefs.end(); eIt++) {
		entry_ref ref = (eIt->first);
		BMessage *msg = new BMessage(fCommand);
		msg->AddRef("fileRef", &ref);
		
		IconMenuItem *item = new IconMenuItem(eIt->second , ref.name, "", msg);
		
		fMenu->AddItem(item);
	};
};
