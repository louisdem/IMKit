#include <Application.h>
#include <Roster.h>
#include <stdio.h>

#include "InfoPopperUI.h"
#include "InfoPopperSender.h"

void *getplugin(void **data,const char *name, const char *, uint32, plugin_info *, SoundPlayBufferedReader *reader);
void destroyplugin(void*,void *_loader);
status_t show(void*);
void hide(void*);
void setconfig(void*,BMessage *config);
void getconfig(void*,BMessage *config);
BView *configure(BMessage *config);

const char *PLUGINID="sp-ip";

static plugin_descriptor plugin = {
	PLUGIN_DESCRIPTOR_MAGIC,
	PLUGIN_DESCRIPTOR_VERSION,
	PLUGINID,
	1,
	PLUGIN_IS_INTERFACE,

	"InfoPopper Interface",
	"By Michael \"slaad\" Davidson.\n\n"
	"Use the following variables in the title / text;\n"
	"$pitch$: The pitch as a percentage\n"
	"$name$: Current track name\n"
	"$description$: The track description (ie. 44khz MP3)\n"
	"$currenttime$: Current track position\n"
	"$totaltime$: Length of the track" ,
	
	NULL,
	&configure,
	&getplugin,
	&destroyplugin
};

static interface_plugin_ops plugin_ops={
	PLUGIN_INTERFACE_MAGIC,
	PLUGIN_INTERFACE_VERSION,
	show,
	hide,
	setconfig,
	getconfig
};

static plugin_descriptor *plugs[] = {
	&plugin,
	NULL
};

plugin_descriptor **get_plugin_list(void) {
	return plugs;
}

BView *configure(BMessage *config) {
	return new SPIPConfigView(config);
};

void *getplugin(void **data,const char *name, const char *,
	uint32, plugin_info *info, SoundPlayBufferedReader *reader) {
	
	InfoPopperSender *sender = new InfoPopperSender(info->controller);
	sender->Run();
	*data = sender;

	return &plugin_ops;
};

void destroyplugin(void *,void *data) {
	InfoPopperSender *sender = (InfoPopperSender *)data;
	sender->Lock();
	sender->Quit();
};

status_t show(void *data) {
	InfoPopperSender *sender = (InfoPopperSender *)data;
	sender->SendMessages(true);
	
	return B_OK;
};

void hide(void *data) {
	InfoPopperSender *sender = (InfoPopperSender *)data;
	sender->SendMessages(false);
};

void setconfig(void *data, BMessage *config) {
	InfoPopperSender *sender = (InfoPopperSender *)data;
	
	int8 updateType = config->FindInt8("updateType");
	if (updateType != B_OK) {
		sender->UpdateType(updateFileChange);
	} else {
		sender->UpdateType(updateType);
	};
	
	const char *title;
	if (config->FindString("titleText", &title) != B_OK) {
		sender->TitleText("Now Playing ($pitch$% pitch)");
		config->AddString("titleText", "Now Playing ($pitch$ % pitch)");
	} else {
		sender->TitleText(title);
	};
	
	const char *main = NULL;
	if (config->FindString("mainText", &main) != B_OK) {
		sender->MainText("$name$ ($totaltime$)\n$description$");
		config->AddString("mainText", "$name$ ($totaltime$)\n$description$");
	} else {
		sender->MainText(main);
	};
};

void getconfig(void *data, BMessage *config) {
	return;

	InfoPopperSender *sender = (InfoPopperSender *)data;

	config->MakeEmpty();
	config->AddInt8("updateType", sender->UpdateType());
	config->AddString("titleText", sender->TitleText());
	config->AddString("mainText", sender->MainText());
};

enum {
	msgUpdateNotify = 'spc1',
	msgUpdateTitleText = 'spc2',
	msgUpdateMainText = 'spc3',
};

//#pragma mark -

SPIPConfigView::SPIPConfigView(BMessage *config)
	: BView(BRect(0, 0, 200, 100), "SPIPConfig", B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_ASYNCHRONOUS_CONTROLS),
	fConfig(config) {
};

SPIPConfigView::~SPIPConfigView(void) {
	fBehaviourField->RemoveSelf();
	delete fBehaviourField;
	
	fTitleCtrl->RemoveSelf();
	delete fTitleCtrl;

	fMainCtrl->RemoveSelf();
	delete fMainCtrl;
};

//#pragma mark -

void SPIPConfigView::AttachedToWindow(void) {
#if B_BEOS_VERSION > B_BEOS_VERSION_5
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(ui_color(B_PANEL_TEXT_COLOR));
#else
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetHighColor(0, 0, 0, 0);
#endif

	fBehaviour = new BMenu("Behaviour");
	fBehaviour->AddItem(new BMenuItem("Constantly", new BMessage(msgUpdateNotify)));
	fBehaviour->AddItem(new BMenuItem("On a new song", new BMessage(msgUpdateNotify)));
	
	float w = 0;
	float h = 0;
	BRect controlRect = Bounds();
	
	fBehaviourField = new BMenuField(controlRect, "Behaviour", "Update Behaviour",
		fBehaviour);
	AddChild(fBehaviourField);
	fBehaviourField->SetDivider(be_plain_font->StringWidth("Update Behaviour")
		+ kPadding);

	int8 updateType = fConfig->FindInt8("updateType");
	if (updateType < B_OK) updateType = updateFileChange;
	fBehaviour->SetTargetForItems(this);
	fBehaviour->SetLabelFromMarked(true);
	fBehaviour->ItemAt(updateType)->SetMarked(true);

	fBehaviourField->GetPreferredSize(&w, &h);
	fBehaviourField->ResizeTo(w, h);

	const char *titleText = fConfig->FindString("titleText");
	if (titleText == NULL) titleText = "Now Playing ($pitch$% p1tch)";

	controlRect.top += h + kPadding;
	fTitleCtrl = new BTextControl(controlRect, "TitleText", "Title Text",
		titleText, new BMessage(msgUpdateTitleText));
	AddChild(fTitleCtrl);
	fTitleCtrl->SetDivider(be_plain_font->StringWidth("Title Text") + kPadding);
	
	fTitleCtrl->GetPreferredSize(&w, &h);
	fTitleCtrl->ResizeTo(fTitleCtrl->Bounds().Width(), h);
	fTitleCtrl->SetTarget(this);
	
	controlRect.top += h + kPadding;

	const char *mainText = fConfig->FindString("mainText");
	if (mainText == NULL) mainText = "$name$ ($totaltime$)\n$description$";

	controlRect.top += h + kPadding;
	fMainCtrl = new BTextControl(controlRect, "MainText", "Main Text",
		mainText, new BMessage(msgUpdateMainText));
	AddChild(fMainCtrl);
	fMainCtrl->SetDivider(be_plain_font->StringWidth("Main Text") + kPadding);
	
	fMainCtrl->GetPreferredSize(&w, &h);
	fMainCtrl->ResizeTo(fMainCtrl->Bounds().Width(), h);
	fMainCtrl->SetTarget(this);
	
	controlRect.top += h + kPadding;		
};

void SPIPConfigView::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case msgUpdateNotify: {
			fConfig->ReplaceInt8("updateType", msg->FindInt32("index"));
		} break;
		
		case msgUpdateTitleText: {
			fConfig->ReplaceString("titleText", fTitleCtrl->Text());
		} break;

		case msgUpdateMainText: {
			fConfig->ReplaceString("mainText", fMainCtrl->Text());
		} break;

		
		default: {
			BView::MessageReceived(msg);
		};
	};
};

//#pragma mark -

int main(int argc, char *argv[]) {
	BApplication app("application/x-vnd.beclan-soundplay-infopopper");
	
	BMessage msg(ENABLE_INTERFACE);
	msg.AddString("interface",PLUGINID);
	be_roster->Launch("application/x-vnd.marcone-soundplay",&msg);
	return 0;
}

