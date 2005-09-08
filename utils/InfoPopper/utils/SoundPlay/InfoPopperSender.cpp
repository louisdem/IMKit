#include "InfoPopperSender.h"

InfoPopperSender::InfoPopperSender(SoundPlayController *controller)
	: BLooper("SoundPlay-InfoPopper Gateway"),
	fController(controller),
	fRunner(NULL) {
	
	fMainText = "";
	fTitleText = "";
	fUpdateType = updateConstant;
	
	fController->Lock();

	while (fController->PlaylistAt(0)->IsValid() != true) fController->AddPlaylist();
	
	fTrack = fController->PlaylistAt(0);
	fController->Unlock();
	
	fAlbumPath = "/boot/home/config/settings/BeClan/InfoPopper/SoundPlay/AlbumCovers/";
};

InfoPopperSender::~InfoPopperSender(void) {
	delete fRunner;
};

//#pragma mark -

void InfoPopperSender::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case kCheckTrack: {
			if (fTrack->IsValid() == false) {
				fController->Lock();
				fTrack = fController->PlaylistAt(0);
				if (fTrack->IsValid() == false) {
					fController->AddPlaylist();
					fTrack = fController->PlaylistAt(0);
				};
				fController->Unlock();
			};
			fTrack->Lock();

			struct file_info info;
			int index = fTrack->CurrentID();
			const char *path = fTrack->PathForItem(index);
			
			if (path == NULL) {
				fTrack->Unlock();
				return;
			};
			
			entry_ref ref;
			float pitch = fTrack->Pitch();

			fTrack->GetInfo(index, &info);
			get_ref_for_path(path, &ref);
			
			if ((ref != fLastRef) || ((pitch != fLastPitch) && (pitch != 0)) ||
				(fUpdateType == updateConstant)) {
			
				float length = info.framecount / info.samplerate;
				double position = fTrack->Position();
				div_t current = div(position, 60);
				div_t total = div(length, 60);
	
				BString title = fTitleText;
				BString contents = fMainText;
				
				char currBuffer[255];
				char totalBuffer[255];
				char pitchBuffer[255];
							
				sprintf(currBuffer, "%i:%02i\0", current.quot, current.rem);
				sprintf(totalBuffer, "%i:%02i\0", total.quot, total.rem);
				sprintf(pitchBuffer, "%.2f\0", fTrack->Pitch() * 100);
				
				title.ReplaceAll("$pitch$", pitchBuffer);
				contents.ReplaceAll("$pitch$", pitchBuffer);
				title.ReplaceAll("$name$", fTrack->CurrentName());
				contents.ReplaceAll("$name$", fTrack->CurrentName());
				title.ReplaceAll("$description$", info.typedesc);
				contents.ReplaceAll("$description$", info.typedesc);
				title.ReplaceAll("$currenttime$", currBuffer);
				contents.ReplaceAll("$currenttime$", currBuffer);
				title.ReplaceAll("$totaltime$", totalBuffer);
				contents.ReplaceAll("$totaltime$", totalBuffer);

				title.ReplaceAll("\\n", "\n");
				contents.ReplaceAll("\\n", "\n");

				BMessenger msgr(InfoPopperAppSig);
				BMessage pop(InfoPopper::AddMessage);
				
				if (fUpdateType == updateConstant) {
					pop.AddInt8("type", InfoPopper::Progress);
					pop.AddFloat("progress", fTrack->Position() / length);
				} else {
					pop.AddInt8("type", InfoPopper::Information);
				};

				pop.AddString("app", "SoundPlay");
				pop.AddString("title", title);
				pop.AddString("content", contents);
				pop.AddString("messageID", "SP-IPGateway");
						
				BString searchStr;
				BString coverPath = fAlbumPath;
				int32 aLength = -1;
				BPath rPath(&ref);
				char *aBuffer = ReadAttribute(rPath.Path(), "Audio:Artist", &aLength);
				coverPath.Append(aBuffer, aLength);
				
				searchStr.Append(aBuffer, aLength);
				if (searchStr.Length() > 0) searchStr << " ";
				free(aBuffer);
	
				coverPath.Append("_");
				aBuffer = ReadAttribute(rPath.Path(), "Audio:Album", &aLength);
				coverPath.Append(aBuffer, aLength);
				
				searchStr.Append(aBuffer, aLength);
				free(aBuffer);

				BEntry entry(coverPath.String());
				if (entry.Exists() == true) {
					entry_ref cover;
					entry.GetRef(&cover);
	
					pop.AddRef("iconRef", &cover);
					pop.AddInt32("iconType", InfoPopper::Contents);
					
					pop.AddRef("overlayIconRef", &ref);
					pop.AddInt32("overlayIonType", InfoPopper::Attribute);				
				} else {
					URLEncode(&searchStr);				

					if (FetchAlbumCover(coverPath, searchStr) == 0) {
						entry_ref cover;
						get_ref_for_path(coverPath.String(), &cover);

						pop.AddRef("iconRef", &cover);
						pop.AddInt32("iconType", InfoPopper::Contents);
						
						pop.AddRef("overlayIconRef", &ref);
						pop.AddInt32("overlayIonType", InfoPopper::Attribute);
					} else {		
						pop.AddRef("iconRef", &ref);
						pop.AddInt32("iconType", InfoPopper::Attribute);
					};
				};

				msgr.SendMessage(&pop);
			};
			
			fLastRef = ref;
			fLastPitch = pitch;
			
			fTrack->Unlock();
		} break;
		
		default: {
			BLooper::MessageReceived(msg);
		};
	};
};

void InfoPopperSender::SendMessages(bool send) {
	if (send) {
		if (fRunner == NULL) {
			fRunner = new BMessageRunner(this, new BMessage(kCheckTrack),
				1000 * 1000 * 1, -1);
		};
	} else {
		delete fRunner;
	}
};

void InfoPopperSender::UpdateType(int8 type) {
	fUpdateType = type;
};

int8 InfoPopperSender::UpdateType(void) {
	return fUpdateType;
};

void InfoPopperSender::TitleText(const char *text) {
	fTitleText = text;
};

const char *InfoPopperSender::TitleText(void) {
	return fTitleText.String();
};

void InfoPopperSender::MainText(const char *text) {
	fMainText = text;
};

const char *InfoPopperSender::MainText(void) {
	return fMainText.String();
};

//#pragma mark -

int	InfoPopperSender::FetchAlbumCover(BString albumPath, BString search) {
	if ((albumPath.Length() <= 0) || (search.Length() <= 0) ) return -1;

	BString xmlURL = "http://xml.amazon.com/onca/xml3?locale=us&t=t&dev-t=";
	xmlURL << "0CRWYJS6C855FMVBEM82&";
	xmlURL << "mode=music&sort=+pmrank&offer=All&type=lite&page=1&f=xml&";
	xmlURL << "ResponseGroup=Images";
	xmlURL << "&KeywordSearch=" << search;

	int status = -1;
	char *xmlPath = tmpnam(NULL);
	if (xmlNanoHTTPFetch(xmlURL.String(), xmlPath, NULL) != 0) return -1;

	xmlParserCtxtPtr ctxt = xmlNewParserCtxt();
	xmlDocPtr doc = xmlCtxtReadFile(ctxt, xmlPath, NULL, 0);
	xmlXPathContextPtr pathCtxt = xmlXPathNewContext(doc);

	if (pathCtxt != NULL) {
		xmlXPathObjectPtr imageNode = xmlXPathEvalExpression((const xmlChar *)"/ProductInfo/Details/ImageUrlMedium", pathCtxt);
		if (imageNode == NULL) {
			xmlFreeParserCtxt(ctxt);
			xmlXPathFreeContext(pathCtxt);
			xmlXPathFreeObject(imageNode);

			return -1;
		};

		xmlNodeSetPtr items = imageNode->nodesetval;
		if (items == NULL) {
			xmlFreeParserCtxt(ctxt);
			xmlXPathFreeContext(pathCtxt);
			xmlXPathFreeObject(imageNode);

			return -1;			
		};

		for (int32 i = 0; i < items->nodeNr; i++) {
			xmlNode *node = items->nodeTab[i]->children;

			if (node != NULL) {
				BString imageURL = GetNodeContents(node);

				if (xmlNanoHTTPFetch(imageURL.String(), albumPath.String(), NULL) == 0) {
					status = 0;
					break;
				};
				node = node->next;
			};
		};
		xmlXPathFreeObject(imageNode);
	};
	
	xmlFreeParserCtxt(ctxt);
	xmlXPathFreeContext(pathCtxt);
	
	unlink(xmlPath);
	return status;
};


BString InfoPopperSender::GetNodeContents(xmlNode *node) {
	BString temp = "";
	xmlBuffer *buff = xmlBufferCreate();

	xmlNodeBufGetContent(buff, node);
	temp.SetTo((const char *)xmlBufferContent(buff), xmlBufferLength(buff));
	xmlBufferFree(buff);

	return temp;
};

void InfoPopperSender::URLEncode(BString *str) {
	str->ReplaceAll("%", "%25");
	str->ReplaceAll("\n", "%20");
	str->ReplaceAll(" ", "%20");
	str->ReplaceAll("\"", "%22");
	str->ReplaceAll("#", "%23");
	str->ReplaceAll("@", "%40");
	str->ReplaceAll("`", "%60");
	str->ReplaceAll(":", "%3A");
	str->ReplaceAll("<", "%3C");
	str->ReplaceAll(">", "%3E");
	str->ReplaceAll("[", "%5B");
	str->ReplaceAll("\\", "%5C");
	str->ReplaceAll("]", "%5D");
	str->ReplaceAll("^", "%5E");
	str->ReplaceAll("{", "%7B");
	str->ReplaceAll("|", "%7C");
	str->ReplaceAll("}", "%7D");
	str->ReplaceAll("~", "%7E"); 
};
