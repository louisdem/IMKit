#include "QueryColumnListView.h"

const char *kTrackerQueryVolume = "_trk/qryvol1";
const char *kTrackerQueryPredicate = "_trk/qrystr";
const char *kTrackerQueryInitMime = "_trk/qryinitmime";
const char *kTrackerQueryType = "application/x-vnd.Be-query";

const char *kFolderState = "_trk/columns_le";
const int16 kAttrIndexOffset = 5;
const int32 kSnoozePeriod = 1000 * 1000;
const int32 kIconSize = 16;

mime_map QueryColumnListView::fMimeTypes;

int64 IntValue(char *buffer, int16 size) {
	int64 value = 0;
	for (int i = 0; i < size; i++) value += (uint8)buffer[i] << (i * 8);
	
	return value;
};

QueryColumnListView::QueryColumnListView(BRect rect, const char *name,
	uint32 resizingMode, uint32 drawFlags, entry_ref ref, BMessage *msg = NULL,
	BMessenger *notify = NULL, border_style border = B_NO_BORDER,
	bool showHorizontalScrollbar = true)
	: BColumnListView(rect, name, resizingMode, drawFlags, border,
		showHorizontalScrollbar) {
		
	fMessage = msg;
	fNotify = notify;

	fRef = ref;
	
	char *buffer = NULL;
	int32 length;
	ExtractVolumes(&fRef, &fVolumes);
	ExtractPredicate(&ref, &buffer, &length);
	fPredicate = buffer;
	free(buffer);
	fInitialFetch = true;
};

QueryColumnListView::~QueryColumnListView(void) {
	if (fNotify) delete fNotify;
	if (fMessage) delete fMessage;
};

//#pragma mark -

void QueryColumnListView::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case qclvAddRow: {
			entry_ref ref;
			if (msg->FindRef("ref", &ref) == B_OK) {
				AddRowByRef(&ref);
			};
		} break;
		
		case B_QUERY_UPDATE: {
			int32 opcode = 0;
			if (msg->FindInt32("opcode", &opcode) != B_OK) return;
			result r;
					
			switch (opcode) {
				case B_ENTRY_CREATED: {
					const char *name;
				
					msg->FindInt32("device", &r.ref.device); 
					msg->FindInt64("directory", &r.ref.directory); 
					msg->FindString("name", &name); 
					r.ref.set_name(name);

					msg->FindInt32("device", &r.nref.device);
					msg->FindInt64("node", &r.nref.node);
										
					fResults[r.ref] = r;
					fPending.push_back(r.ref);

				} break;
				
				case B_ENTRY_REMOVED: {
					node_ref nref;
					resultmap::iterator rIt;

					msg->FindInt32("device", &nref.device);
					msg->FindInt64("node", &nref.node);

					for (rIt = fResults.begin(); rIt != fResults.end(); rIt++) {
						result r = rIt->second;
						
						if (nref == r.nref) {
							fResults.erase(r.ref);
							RemoveRowByRef(&r.ref);
							break;
						};
					};
				} break;
			};
		
			if ((fNotify) && (fNotify->IsValid()) && (fMessage)) {
				BMessage send(*fMessage);
				send.AddInt32("qclvCount", fResults.size());
				
				fNotify->SendMessage(send);
			};
		} break;
				
		case qclvRequestPending: {
			pending_stack::iterator pIt;
			BMessage reply(B_REPLY);
			for (pIt = fPending.begin(); pIt != fPending.end(); pIt++) {
				reply.AddRef("ref", pIt);
			};
			fPending.clear();
			
			if (fInitialFetch) {
				reply.AddBool("initial", fInitialFetch);
				fInitialFetch = false;
			};
			
			msg->SendReply(&reply);
		} break;
		
		case qclvInvoke: {
			msg->PrintToStream();
			BRow *row = FocusRow();
			if (row) {
				BStringField *field = reinterpret_cast<BStringField *>(row->GetField(0));
				if (field) {
					entry_ref ref;
					if (get_ref_for_path(field->String(), &ref) == B_OK) {
					
						entry_ref actionRef = ActionFor(&ref);
						BPath t(&actionRef);
				
						BMessage open(B_REFS_RECEIVED);
						open.AddRef("refs", &ref);
			
						be_roster->Launch("application/x-vnd.Be-TRAK", &open);	
					};
				};
			};
			
		} break;
		
		default: {
			BColumnListView::MessageReceived(msg);
		};
	};
};

void QueryColumnListView::AttachedToWindow(void) {
	fMIMEString = MIMEType();

	vollist::iterator vIt;
	for (vIt = fVolumes.begin(); vIt != fVolumes.end(); vIt++) {
		BQuery *query = new BQuery();
		
		query->SetPredicate(fPredicate.String());
		query->SetTarget(this);
		query->SetVolume(vIt);

		query->Fetch();
		fQueries.push_back(query);
	};

	BMessage msg;
	BMimeType mime(fMIMEString.String());
	mime.GetAttrInfo(&msg);
	AddMIMEColumns(&msg);
	
	BString queryPath = fMIMEString;
	queryPath.ReplaceAll("/", "_");
	queryPath.Prepend("/boot/home/config/settings/Tracker/DefaultQueryTemplates/");
	
	int32 length = -1;
	char *value = ReadAttribute(queryPath.String(), kFolderState, &length);
	
	BMallocIO buffer;
	buffer.WriteAt(0, value, length);
	buffer.Seek(0, SEEK_SET);
	ExtractColumnState(&buffer);

	free(value);

	fSelfMsgr = new BMessenger(this);
	
	BString name = fRef.name;
	name << " Background Adder";
	fThreadID = spawn_thread(BackgroundAdder, name.String(), B_LOW_PRIORITY, this);
	if (fThreadID > B_ERROR) resume_thread(fThreadID);

	SetTarget(this);
	SetInvocationMessage(new BMessage(qclvInvoke));
};

status_t QueryColumnListView::AddRowByRef(entry_ref *ref) {
	ref_map::iterator rIt = fRefRows.find(*ref);
	if (rIt != fRefRows.end()) {
		return B_ERROR;
	};
	
	index_map::iterator iIt;
	BPath path(ref);
	BNode node(ref);
	BFile file(ref, B_READ_ONLY);
	off_t size;
	struct stat s;

	node.GetStat(&s);

	BRow *row = new BRow(20);

	file.GetSize(&size);
	row->SetField(new BStringField(path.Path()), 0);
	BBitmap *icon = getBitmap(path.Path(), kIconSize);
	row->SetField(new BBitmapField(icon), 1);
	row->SetField(new BStringField(ref->name), 2);
	row->SetField(new BSizeField(size), 3);
	row->SetField(new BDateField(&s.st_mtime), 4);
	
	for (iIt = fAttrIndex.begin(); iIt != fAttrIndex.end(); iIt++) {
		attr_info info;
		node.GetAttrInfo(iIt->first.String(), &info);
		char *value = (char *)calloc(info.size, sizeof(char));
		node.ReadAttr(iIt->first.String(), info.type, 0, value, info.size);
		
		switch (info.type) {
			case B_CHAR_TYPE: {
				row->SetField(new BStringField(value), iIt->second);
			} break;
			case B_STRING_TYPE: {
				row->SetField(new BStringField((char *)value), iIt->second);
			} break;

			case B_INT8_TYPE: {
				int8 *intValue = (int8 *)&value;
				row->SetField(new BIntegerField(*intValue), iIt->second);
			} break;
			case B_INT16_TYPE: {
				int16 *intValue = (int16 *)&value;
				row->SetField(new BIntegerField(*intValue), iIt->second);
			} break;
			case B_INT32_TYPE: {
				int32 *intValue = (int32 *)&value;
				row->SetField(new BIntegerField(*intValue), iIt->second);
			} break;
			case B_INT64_TYPE: {
				int64 *intValue = (int64 *)&value;
				row->SetField(new BIntegerField(*intValue), iIt->second);
			} break;
			
			case B_SIZE_T_TYPE: {
				size_t *sizeValue = (size_t *)&value;
				row->SetField(new BSizeField(*sizeValue), iIt->second);
			} break;
			
			case B_TIME_TYPE: {
				time_t *timeValue = (time_t *)&value;
				row->SetField(new BDateField(timeValue), iIt->second);
			} break;
			
			default: {
				printf("%s (%4.4s) was unhandled!\n", iIt->first.String(), &info.type);
			};

		};

		free(value);		
	};

	AddRow(row);
	fRefRows[*ref] = row;

	return B_OK;
};

status_t QueryColumnListView::RemoveRowByRef(entry_ref *ref) {
	ref_map::iterator rIt = fRefRows.find(*ref);
	status_t ret = B_ERROR;
	if (rIt != fRefRows.end()) {
		RemoveRow(rIt->second);
		fRefRows.erase(rIt);
		ret = B_OK;
	};
	
	return B_OK;
};

const char *QueryColumnListView::MIMEType(void) {
	if (fMimeTypes.size() == 0) {
		BMimeType mimeDB;
		BMessage types;
		mimeDB.GetInstalledTypes(&types);
			
		const char *type;
		for (int32 i = 0; types.FindString("types", i, &type) == B_OK; i++) {
			char name[B_MIME_TYPE_LENGTH];
			memset(name, '\0', B_MIME_TYPE_LENGTH);
			BMimeType mime(type);
			if (mime.GetShortDescription(name) == B_OK) fMimeTypes[name] = type;
		};
	};

	const char *ret = NULL;
	char *initial = ReadAttribute(BNode(&fRef), kTrackerQueryInitMime);
	mime_map::iterator qIt = fMimeTypes.find(initial);
	if (qIt != fMimeTypes.end()) ret = qIt->second.String();
	
	return ret;
};

char *QueryColumnListView::MIMETypeFor(entry_ref *ref) {
	int32 length = -1;
	char *type = ReadAttribute(BNode(ref), "BEOS:TYPE", &length);
	if ((type) && (length > 0)) {
		type = (char *)realloc(type, (length + 1) * sizeof(char));
		type[length] = '\0';
	};
	
	return type;
};

const char *QueryColumnListView::Name(void) {
	return fRef.name;
};

//#pragma mark -

status_t QueryColumnListView::ExtractColumnState(BMallocIO *source) {
	status_t ret = B_ERROR;

//	Data is;
//	int32, int32; Key / Hash
//	int32 nameLen, char [nameLen + 1]
//	float offset, float width
//	align alignment
//	int32 internalLen, char [internalLen + 1]
//	uint32 hash
//	uint32 type
//	bool statField
//	bool editable
		
	for (int32 i = kAttrIndexOffset; i < CountColumns(); i++) SetColumnVisible(i, false);
	
	while (source->Position() < source->BufferLength()) {
		char *publicName = NULL;
		char *internalName = NULL;
		float width = -1;
		alignment align;
		uint32 type;	
		int32 length = -1;
		
		source->Seek(sizeof(int32) * 2, SEEK_CUR);	// Key / Hash
		source->Read(&length, sizeof(int32));
		publicName = (char *)calloc(length + 1, sizeof(char));
		source->Read(publicName, length + 1);
		
		source->Seek(sizeof(float), SEEK_CUR); // Offset
		source->Read(&width, sizeof(float));
		source->Read(&align, sizeof(alignment));
		
		source->Read(&length, sizeof(int32));
		internalName = (char *)calloc(length + 1, sizeof(char));
		source->Read(internalName, length + 1);
		
		source->Seek(sizeof(uint32), SEEK_CUR); // Hash
		source->Read(&type, sizeof(int32));

		source->Seek(sizeof(bool), SEEK_CUR); // statField
		source->Seek(sizeof(bool), SEEK_CUR); // editable
	
		index_map::iterator iIt = fAttrIndex.find(internalName);
		if (iIt != fAttrIndex.end()) SetColumnVisible(iIt->second, true);
	};

	return ret;
};

status_t QueryColumnListView::AddMIMEColumns(BMessage *msg) {
	status_t ret = B_OK;
	int32 index = kAttrIndexOffset;

	float maxWidthMulti = be_plain_font->StringWidth("M");
	
	BStringColumn *path = new MenuStringColumn("Path", 0, 0, 0, B_ALIGN_CENTER);
	path->SetShowHeading(false);
	AddColumn(path, 0);
	
	BBitmapColumn *icon = new BBitmapColumn("", 20, 20, 20, B_ALIGN_CENTER);
	icon->SetShowHeading(false);
	AddColumn(icon, 1);
	
	BStringColumn *name = new MenuStringColumn("Name", 100, 200, 300,
		B_TRUNCATE_END, B_ALIGN_LEFT);
	AddColumn(name, 2);
	AddColumn(new MenuSizeColumn("Size", 20, 50, 100, B_ALIGN_RIGHT), 3);
	AddColumn(new MenuDateColumn("Modified", 50, 100, 200, B_ALIGN_LEFT), 4);

	SetSortingEnabled(true);
	SetSortColumn(name, false, true);

	for (int32 i = 0; msg->FindString("attr:name", i) != NULL; i++) {
		bool viewable = false;
		if (msg->FindBool("attr:viewable", i, &viewable) != B_OK) continue;
		if (viewable == false) continue;
		
		const char *publicName;
		const char *internalName;
		alignment align = B_ALIGN_LEFT;
		int32 type;
		int32 widthTemp;
		float width = 0;
		float maxWidth = 0;
		float minWidth = 0;

		if (msg->FindString("attr:name", i, &internalName) != B_OK) continue;
		if (msg->FindString("attr:public_name", i, &publicName) != B_OK) continue;
		if (msg->FindInt32("attr:type", i, &type) != B_OK) continue;
		msg->FindInt32("attr:alignment", i, (int32 *)&align);
		msg->FindInt32("attr:width", i, &widthTemp);
		width = widthTemp;
		maxWidth = width * maxWidthMulti;
		minWidth = be_plain_font->StringWidth(publicName);
				
		fAttributes[internalName] = publicName;
		fAttrTypes[internalName] = type;
		fAttrIndex[internalName] = index;
		
		switch (type) {
			case B_CHAR_TYPE:
			case B_STRING_TYPE: {
				AddColumn(new MenuStringColumn(publicName, width, minWidth,
					maxWidth, align), index++);
			} break;
			
			case B_UINT8_TYPE:
			case B_INT8_TYPE:
			case B_UINT16_TYPE:
			case B_INT16_TYPE:
			case B_UINT32_TYPE:
			case B_INT32_TYPE:
			case B_UINT64_TYPE:
			case B_INT64_TYPE: {
				AddColumn(new MenuIntegerColumn(publicName, width, minWidth,
					maxWidth, align), index++);
			} break;
			
			case B_SIZE_T_TYPE: {
				AddColumn(new MenuSizeColumn(publicName, width, minWidth,
					maxWidth, align), index++);
			} break;
			
			case B_TIME_TYPE: {
				AddColumn(new MenuDateColumn(publicName, width, minWidth,
					maxWidth, align), index++);
			} break;
			
			default: {
				printf("%s (%s) is of an unhandled type: %4.4s\n", publicName,
					internalName, &type);
			};
		};
	};
	
	return ret;
};

int32 QueryColumnListView::BackgroundAdder(void *arg) {
	QueryColumnListView *self = reinterpret_cast<QueryColumnListView *>(arg);
	
	while (self->fSelfMsgr->IsValid()) {
		BMessage getPending(qclvRequestPending);
		BMessage pending;
		if (self->fSelfMsgr->SendMessage(getPending, &pending) == B_OK) {
			entry_ref ref;
			bool updatedAny = false;
			
			if (pending.CountNames(B_ANY_TYPE) > 0) { 
				self->LockLooper();
				
				if (pending.FindBool("initial") == true) {
					entry_ref ref;
					querylist::iterator qIt;
					for (qIt = self->fQueries.begin(); qIt != self->fQueries.end(); qIt++) {
						BQuery *query = (*qIt);
						while (query->GetNextRef(&ref) == B_OK) {
							BNode node(&ref);
							result r;
				
							r.ref = ref;
							node.GetNodeRef(&r.nref);
				
							self->fResults[ref] = r;
							pending.AddRef("ref", &ref);
						};
					};
					
					updatedAny = true;
				};			
				
				for (int32 i = 0; pending.FindRef("ref", i, &ref) == B_OK; i++) {
					updatedAny = true;
					self->AddRowByRef(&ref);
				};
	
				if (updatedAny) {
					self->Hide();
					self->Sync();
					self->Show();
					self->Sync();
					
					if ((self->fNotify) && (self->fNotify->IsValid()) && (self->fMessage)) {
						BMessage send(*self->fMessage);
						send.AddInt32("qclvCount", self->fResults.size());
						
						self->fNotify->SendMessage(send);
					};
				};
				self->UnlockLooper();
			};
		};
		
		snooze(kSnoozePeriod);
	};

	return B_OK;
};

status_t QueryColumnListView::ExtractPredicate(entry_ref *ref, char **buffer,
	int32 *length) {
	
	status_t ret = B_ERROR;
	if (*buffer) free(*buffer);
	*buffer = ReadAttribute(BNode(ref), kTrackerQueryPredicate, length);
	
	if ((*buffer != NULL) && (length > 0)) ret = B_OK;
	
	return ret;
};

status_t QueryColumnListView::ExtractVolumes(entry_ref *ref, vollist *vols) {
	BNode node(ref);
	int32 length = 0;
	char *attr = ReadAttribute(node, kTrackerQueryVolume, &length);
	BVolumeRoster roster;

	if (attr == NULL) {
		roster.Rewind();
		BVolume vol;
		
		while (roster.GetNextVolume(&vol) == B_NO_ERROR) {
			if (vol.KnowsQuery() == true) vols->push_back(vol);
		};
	} else {
		BMessage msg;
		msg.Unflatten(attr);

//		!*YOINK*!d from that project... with the funny little doggie as a logo...
//		OpenTracker, that's it!
			
		time_t created;
		off_t capacity;
		
		for (int32 index = 0; msg.FindInt32("creationDate", index, &created) == B_OK;
			index++) {
			
			if ((msg.FindInt32("creationDate", index, &created) != B_OK)
				|| (msg.FindInt64("capacity", index, &capacity) != B_OK))
				return B_ERROR;
		
			BVolume volume;
			BString deviceName = "";
			BString volumeName = "";
			BString fshName = "";
		
			if (msg.FindString("deviceName", &deviceName) == B_OK
				&& msg.FindString("volumeName", &volumeName) == B_OK
				&& msg.FindString("fshName", &fshName) == B_OK) {
				// New style volume identifiers: We have a couple of characteristics,
				// and compute a score from them. The volume with the greatest score
				// (if over a certain threshold) is the one we're looking for. We
				// pick the first volume, in case there is more than one with the
				// same score.
				int foundScore = -1;
				roster.Rewind();
				while (roster.GetNextVolume(&volume) == B_OK) {
					if (volume.IsPersistent() && volume.KnowsQuery()) {
						// get creation time and fs_info
						BDirectory root;
						volume.GetRootDirectory(&root);
						time_t cmpCreated;
						fs_info info;
						if (root.GetCreationTime(&cmpCreated) == B_OK
							&& fs_stat_dev(volume.Device(), &info) == 0) {
							// compute the score
							int score = 0;
		
							// creation time
							if (created == cmpCreated)
								score += 5;
							// capacity
							if (capacity == volume.Capacity())
								score += 4;
							// device name
							if (deviceName == info.device_name)
								score += 3;
							// volume name
							if (volumeName == info.volume_name)
								score += 2;
							// fsh name
							if (fshName == info.fsh_name)
								score += 1;
		
							// check score
							if (score >= 9 && score > foundScore) {
								vols->push_back(volume);
							}
						}
					}
				}
			} else {
				// Old style volume identifiers: We have only creation time and
				// capacity. Both must match.
				roster.Rewind();
				while (roster.GetNextVolume(&volume) == B_OK)
					if (volume.IsPersistent() && volume.KnowsQuery()) {
						BDirectory root;
						volume.GetRootDirectory(&root);
						time_t cmpCreated;
						root.GetCreationTime(&cmpCreated);
						if (created == cmpCreated && capacity == volume.Capacity()) {
							vols->push_back(volume);
						}
					}
			}
		};
	};

	return B_OK;	
};

entry_ref QueryColumnListView::ActionFor(entry_ref *ref) {
	entry_ref action;
	BPath queryPath(&fRef);
	BPath actionPath("/boot/home/config/settings/BeClan/QueryViewer/Actions");
	BPath tempPath = actionPath;
	
	tempPath.Append(queryPath.Leaf());
	tempPath.Append("_DEFAULT_");
	
	BEntry actionEntry(tempPath.Path(), false);
	if (actionEntry.Exists() == false) {
	
		tempPath = actionPath;
		char *mimeType = MIMETypeFor(ref);
		BString mime = mimeType;
		free(mimeType);
		mime.ReplaceAll("/", "_");
		tempPath.Append(mime.String());
		tempPath.Append("_DEFAULT_");

		actionEntry = BEntry(tempPath.Path(), false);

		if (actionEntry.Exists() == false) {
			tempPath = actionPath;
			tempPath.Append("Generic/_DEFAULT");

			actionEntry = BEntry(tempPath.Path(), false);
			if (actionEntry.Exists() == false) {
				actionEntry = BEntry("/boot/beos/system/Tracker", false);
			};
		};
	};
	
	actionEntry.GetRef(&action);
		
	return action;
};