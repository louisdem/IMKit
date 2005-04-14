#include "InfoPopperSender.h"

InfoPopperSender::InfoPopperSender(SoundPlayController *controller)
	: BLooper("SoundPlay-InfoPopper Gateway"),
	fController(controller),
	fRunner(NULL) {
	
	fMainText = "";
	fTitleText = "";
	
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
			entry_ref ref;
			fTrack->GetInfo(index, &info);

			get_ref_for_path(path, &ref);
			
			if ((ref != fLastRef) || (fUpdateType == updateConstant)) {
			
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
						
				BString coverPath = fAlbumPath;
				int32 aLength = -1;
				BPath rPath(&ref);
				char *aBuffer = ReadAttribute(rPath.Path(), "Audio:Artist", &aLength);
				coverPath.Append(aBuffer, aLength);
				free(aBuffer);
	
				coverPath.Append("_");
				aBuffer = ReadAttribute(rPath.Path(), "Audio:Album", &aLength);
				coverPath.Append(aBuffer, aLength);
				free(aBuffer);
	
	//			coverPath.ReplaceAll("/", "_");
				
				BEntry entry(coverPath.String());
				if (entry.Exists() == true) {
					entry_ref cover;
					entry.GetRef(&cover);
	
					pop.AddRef("iconRef", &cover);
					pop.AddInt32("iconType", InfoPopper::Contents);
					
					pop.AddRef("overlayIconRef", &ref);
					pop.AddInt32("overlayIonType", InfoPopper::Attribute);				
				} else {
					pop.AddRef("iconRef", &ref);
					pop.AddInt32("iconType", InfoPopper::Attribute);
				};
				
				msgr.SendMessage(&pop);
			};
			
			fLastRef = ref;
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
