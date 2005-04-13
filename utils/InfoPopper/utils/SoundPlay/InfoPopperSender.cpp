#include "InfoPopperSender.h"

InfoPopperSender::InfoPopperSender(SoundPlayController *controller)
	: BLooper("SoundPlay-InfoPopper Gateway"),
	fController(controller),
	fRunner(NULL) {
	
	fController->Lock();
//	while (fController->PlaylistAt(1)->IsValid()) {
//		fController->RemovePlaylist(fController->PlaylistAt(1));
//	};
	
	while (fController->PlaylistAt(0)->IsValid() != true) fController->AddPlaylist();
	
	fTrack = fController->PlaylistAt(0);
	fController->Unlock();
};

InfoPopperSender::~InfoPopperSender(void) {
	delete fRunner;
};

//#pragma mark -

void InfoPopperSender::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case kSendToPopper: {
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
		
			float length = info.framecount / info.samplerate;
			double position = fTrack->Position();
			div_t current = div(position, 60);
			div_t total = div(length, 60);

			BString title = "Now Playing (";
			title << (fTrack->Pitch() * 100) << "% pitch)";

			BString contents = fTrack->CurrentName();
			char buffer[255];
			sprintf(buffer, "%i:%02i / %i:%02i", current.quot, current.rem,
				total.quot, total.rem);
			contents << "\n(" << buffer << ")";
			contents << "\n" << info.typedesc;
		
			BMessenger msgr(InfoPopperAppSig);
			BMessage pop(InfoPopper::AddMessage);
			pop.AddInt8("type", InfoPopper::Progress);
			pop.AddString("app", "SoundPlay");
			pop.AddString("title", title);
			pop.AddString("messageID", "SP-IPGateway");
			pop.AddFloat("progress", fTrack->Position() / length); //info.framecount);
			pop.AddString("content", contents);
			
			pop.AddRef("iconRef", &ref);
			pop.AddInt32("iconType", InfoPopper::Attribute);
			
			msgr.SendMessage(pop);

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
			fRunner = new BMessageRunner(this, new BMessage(kSendToPopper),
				1000 * 1000 * 2, -1);
		};
	} else {
		delete fRunner;
	}
};
