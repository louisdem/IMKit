
/*
	"Mixer" SoundPlay interface plugin with skip/hold buttons.
	Copyright 2000 Marco Nelissen (marcone@xs4all.nl)

	Permission is hereby granted to use this code for creating other
	SoundPlay-plugins.
*/

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

const char *PLUGINID="sp-ip";

static plugin_descriptor plugin = {
	PLUGIN_DESCRIPTOR_MAGIC,
	PLUGIN_DESCRIPTOR_VERSION,
	PLUGINID,
	1,
	PLUGIN_IS_INTERFACE,

	"InfoPopper gateway",
	"By Michael \"Slaad\" Davidson.\n",
	NULL,
	NULL, // configure
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

void setconfig(void *data,BMessage *config)
{
//	MixerWindow *win=(MixerWindow*)data;
//	BRect rect;
//	if(B_OK==config->FindRect("frame",&rect))
//	{
//		win->MoveTo(rect.LeftTop());
//		win->ResizeTo(rect.Width(),rect.Height());
//	}
}
void getconfig(void *data,BMessage *config)
{
//	MixerWindow *win=(MixerWindow*)data;
//	config->MakeEmpty();
//	config->AddRect("frame",win->Frame());
}


enum {
	PLAY0='aaaa',
	STOP0,
	HOLD0,
	SKIP0,
	PITCH0,

	PLAY1,
	STOP1,
	HOLD1,
	SKIP1,
	PITCH1,
};

//
//MixerWindow::MixerWindow(SoundPlayController *controller)
//	: MWindow(BRect(100,100,300,250),"Mixer",B_TITLED_WINDOW,B_ASYNCHRONOUS_CONTROLS)
//{
//	ctrl=controller;
//	MSlider *mixslider;
//	MSlider *pitchslider0;
//	MSlider *pitchslider1;
//	AddChild
//	(
//		new MBorder
//		(
//			M_RAISED_BORDER,8,"",
//			new VGroup
//			(
//				new HGroup
//				(
//					new MBorder
//					(
//						M_LABELED_BORDER,8,"Track 1",
//						new VGroup
//						(
//							new HGroup
//							(
//								new MStop(this,new BMessage(STOP0)),
//								new MPlayFW(this,new BMessage(PLAY0)),
//								0
//							),
//							new HGroup
//							(
//								new MButton("<",new BMessage(HOLD0),this,minimax(25,25,25,25)),
//								new MButton(">",new BMessage(SKIP0),this,minimax(25,25,25,25)),
//								0
//							),
//							pitchslider0=new MSlider("pitch",950,1050,1,new BMessage(PITCH0),this),
//							0
//						)
//					),
//					new MBorder
//					(
//						M_LABELED_BORDER,8,"Track 2",
//						new VGroup
//						(
//							new HGroup
//							(
//								new MStop(this,new BMessage(STOP1)),
//								new MPlayFW(this,new BMessage(PLAY1)),
//								0
//							),
//							new HGroup
//							(
//								new MButton("<",new BMessage(HOLD1),this,minimax(25,25,25,25)),
//								new MButton(">",new BMessage(SKIP1),this,minimax(25,25,25,25)),
//								0
//							),
//							pitchslider1=new MSlider("pitch",950,1050,1,new BMessage(PITCH1),this),
//							0
//						)
//					),
//					0
//				),
//				mixslider=new MSlider("mix",0,1000,1,new BMessage('vol '),this),
//				0
//			)
//		)
//	);
//	pitchslider0->SetValue(1000);
//	pitchslider0->Invoke();
//	pitchslider1->SetValue(1000);
//	pitchslider1->Invoke();
//	mixslider->SetValue(500);
//	mixslider->Invoke();
//}
//
//void MixerWindow::Show()
//{
//	// This UI will only use 2 tracks. Delete the rest.
//	ctrl->Lock();
//	while(ctrl->PlaylistAt(2)->IsValid())
//		ctrl->RemovePlaylist(ctrl->PlaylistAt(2));
//	// Make sure we have at least 2 track
//	while(!ctrl->PlaylistAt(1)->IsValid())
//		ctrl->AddPlaylist();
//
//	// Cache the two playlists/tracks
//	// It is important to realize that once you have a valid playlist,
//	// it will never become invalid. Its controls might go away, but
//	// the playlist itself will remain valid.
//	track0=ctrl->PlaylistAt(0);
//	track1=ctrl->PlaylistAt(1);
//	ctrl->Unlock();
//	MWindow::Show();
//}
//
//MixerWindow::~MixerWindow()
//{
//	ctrl->Lock();
//	if(track0->CountItems()==0)
//		ctrl->RemovePlaylist(track0);
//	if(track1->CountItems()==0)
//		ctrl->RemovePlaylist(track1);
//	ctrl->Unlock();
//}
//
//void MixerWindow::MessageReceived(BMessage *mes)
//{
//
//	// re-add the playlists to the window in case they got orphaned
//	ctrl->Lock();
//	if(!track0->HasControls())
//		track0->AddControls();
//	if(!track1->HasControls())
//		track1->AddControls();
//
//	// For simplicity, we lock the playlists here and unlock at the end
//	// of the function, instead of around function that requires them to
//	// be locked.
//	// Since some of the calls we are making also require (or cause)
//	// spcontroller to be locked, and you should always lock spcontroller
//	// first, we also keep spcontroller locked for the duration of this function.
//	track0->Lock();
//	track1->Lock();
//	
//	switch(mes->what)
//	{
//		case 'vol ':
//				{
//					long slidervalue;
//					if(mes->FindInt32("be:value",&slidervalue)==B_OK)
//					{
//						float vol=float(slidervalue)/500.0; // 0-2
//						track0->SetVolume(min_c(1.0,2.0-vol));
//						track1->SetVolume(min_c(1.0,vol));
//					}
//				}
//				break;
//			
//		case STOP0:
//				track0->Pause();
//				break;
//
//		case PLAY0:
//				track0->Play();
//				track0->SetPitch(pitch0);
//				break;
//
//		case SKIP0:
//				// Skip forward a short period of time
//				// NOTE: Don't do this:
//				// double position=ctrl->PlaylistAt(0)->Position();
//				// ctrl->PlaylistAt(0)->SetPosition(position+0.1);
//				// because setting the position will potentially
//				// take a long time for certain types of files.
//				// Instead, do this:
//				track0->SetPitch(2.0*pitch0);
//				snooze(25000);
//				track0->SetPitch(1.0*pitch0);
//				break;
//			
//		case HOLD0:
//				// Skip backwards a short period of time
//				track0->SetPitch(0.0);
//				snooze(25000);
//				track0->SetPitch(1.0*pitch0);
//				break;
//
//		case PITCH0:
//				{
//					long slidervalue;
//					if(mes->FindInt32("be:value",&slidervalue)==B_OK)
//					{
//						pitch0=float(slidervalue)/1000.0;
//						track0->SetPitch(pitch0);
//					}
//				}
//				break;
//		// =====================================================
//		case STOP1:
//				track1->Pause();
//				break;
//
//		case PLAY1:
//				track1->Play();
//				track1->SetPitch(pitch1);
//				break;
//
//		case SKIP1:
//				track1->SetPitch(2.0*pitch1);
//				snooze(25000);
//				track1->SetPitch(1.0*pitch1);
//				break;
//
//		case HOLD1:
//				// Skip backwards a short period of time
//				track1->SetPitch(0.0);
//				snooze(25000);
//				track1->SetPitch(1.0*pitch1);
//				break;
//
//		case PITCH1:
//				{
//					long slidervalue;
//					if(mes->FindInt32("be:value",&slidervalue)==B_OK)
//					{
//						pitch1=float(slidervalue)/1000.0;
//						track1->SetPitch(pitch1);
//					}
//				}
//				break;
//
//		case B_SIMPLE_DATA:
//			// file dropped
//			// to keep things simple, we simply determine in which half
//			// of the window it was dropped (*if* it was dropped)
//			if(mes->WasDropped())
//			{
//				entry_ref ref;
//				if(B_OK==mes->FindRef("refs",&ref))
//				{
//					BPoint point=mes->DropPoint();
//					point=ConvertFromScreen(point);
//					PlaylistPtr track;
//					if(point.x<(Bounds().Width()/2))
//						track=track0;
//					else
//						track=track1;
//					track->MakeEmpty();
//					track->Add(ref);
//					track->PlayFile(0);
//				}
//			}
//			break;
//			
//		default:
//			MWindow::MessageReceived(mes);
//			break;
//	}
//	track1->Unlock();
//	track0->Unlock();
//	ctrl->Unlock();
//}
//
//bool MixerWindow::QuitRequested()
//{
//	ctrl->DisableInterface(PLUGINID);
//	return false;
//}

int main()
{
	BApplication app("application/x-vnd.beclan-soundplay-infopopper");
	
	BMessage msg(ENABLE_INTERFACE);
	msg.AddString("interface",PLUGINID);
	be_roster->Launch("application/x-vnd.marcone-soundplay",&msg);
	return 0;
}

