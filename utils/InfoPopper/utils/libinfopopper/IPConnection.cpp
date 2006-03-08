#include "IPConnection.h"

#include "IPMessage.h"

#include <libim/InfoPopper.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

//#pragma mark Constants
const char *kIPWindowName = "InfoPopper";

//#pragma mark Constructors

IPConnection::IPConnection(void)
	: fMsgMsgr(NULL) {
	fIPMsgr = new BMessenger(InfoPopperAppSig);
};

IPConnection::~IPConnection(void) {
	delete fIPMsgr;
	delete fMsgMsgr;
};
	
//#pragma mark Public

int16 IPConnection::IconSize(void) {
	BMessage reply;
	int16 size = -1;
	
	if (fIPMsgr->SendMessage(GetIconSize, &reply) == B_OK) {
		reply.FindInt16("iconSize", &size);
	};
	
	return size;
};

status_t IPConnection::Send(IPMessage *message) {
	BMessage send(AddMessage);
	send.AddInt8("type", message->Type());
	send.AddString("app", message->Application());
	send.AddString("title", message->Title());
	send.AddString("content", message->Content());
	send.AddFloat("progress", message->Progress());
	
	entry_ref icon = message->MainIcon();
	BEntry entry(&icon);

	if (entry.Exists()) {
		send.AddRef("iconRef", &icon);
		send.AddInt32("iconType", message->MainIconType());
	};

	icon = message->OverlayIcon();
	entry.SetTo(&icon);
	
	if (entry.Exists()) {
		send.AddRef("overlayIconRef", &icon);
		send.AddInt32("overlayIconType", message->OverlayIconType());
	};
	
	send.PrintToStream();
	
	return fIPMsgr->SendMessage(&send);
};

int32 IPConnection::CountMessages(void) {
	int32 result = B_ERROR;
	result = FetchMessageMessenger();
	if (result == B_OK) {
		BMessage reply(B_REPLY);
		BMessage request(B_COUNT_PROPERTIES);
		request.AddSpecifier("message");

		result = fMsgMsgr->SendMessage(&request, &reply);
		if (result == B_OK) reply.FindInt32("result", &result);
	};
	
	return result;
};

IPMessage *IPConnection::MessageAt(int32 index) {
	if (FetchMessageMessenger() != B_OK) return NULL;

	int32 error = B_OK;
	BMessage reply(B_REPLY);		
	BMessage typeReq(B_GET_PROPERTY);
	typeReq.AddSpecifier("type");
	typeReq.AddSpecifier("message", index);
	
	if (fMsgMsgr->SendMessage(&typeReq, &reply) != B_OK) return NULL;
	if ((reply.FindInt32("error", &error) != B_OK) || (error != B_OK)) return NULL;

	int32 type = B_ERROR;
	if (reply.FindInt32("result", &type) != B_OK) return NULL;

	BMessage contentReq(B_GET_PROPERTY);
	contentReq.AddSpecifier("content");
	contentReq.AddSpecifier("message", index);
	
	if (fMsgMsgr->SendMessage(&contentReq, &reply) != B_OK) return NULL;
	if ((reply.FindInt32("error", &error) != B_OK) || (error != B_OK)) return NULL;

	BString content("");
	if (reply.FindString("result", &content) != B_OK) return NULL;

	BMessage titleReq(B_GET_PROPERTY);
	titleReq.AddSpecifier("title");
	titleReq.AddSpecifier("message", index);
	
	if (fMsgMsgr->SendMessage(&titleReq, &reply) != B_OK) return NULL;
	if ((reply.FindInt32("error", &error) != B_OK) || (error != B_OK)) return NULL;

	BString title("");
	if (reply.FindString("result", &title) != B_OK) return NULL;
	
	BMessage appReq(B_GET_PROPERTY);
	appReq.AddSpecifier("apptitle");
	appReq.AddSpecifier("message", index);
	
	if (fMsgMsgr->SendMessage(&appReq, &reply) != B_OK) return NULL;
	if ((reply.FindInt32("error", &error) != B_OK) || (error != B_OK)) return NULL;

	BString app("");
	if (reply.FindString("result", &app) != B_OK) return NULL;

	BMessage progressReq(B_GET_PROPERTY);
	progressReq.AddSpecifier("progress");
	progressReq.AddSpecifier("message", index);
	
	if (fMsgMsgr->SendMessage(&progressReq, &reply) != B_OK) return NULL;
	if ((reply.FindInt32("error", &error) != B_OK) || (error != B_OK)) return NULL;

	float progress = 0.0f;
	if (reply.FindFloat("result", &progress) != B_OK) return NULL;
	
	entry_ref iconRef;
	entry_ref overlayRef;
	int32 iconType;
	int32 overlayType;

	BMessage iconReq(B_GET_PROPERTY);
	iconReq.AddSpecifier("iconRef");
	iconReq.AddSpecifier("message", index);
	
	if (fMsgMsgr->SendMessage(&iconReq, &reply) != B_OK) return NULL;
	reply.FindRef("result", &iconRef);

	BMessage iconTypeReq(B_GET_PROPERTY);
	iconTypeReq.AddSpecifier("iconType");
	iconTypeReq.AddSpecifier("message", index);
	
	if (fMsgMsgr->SendMessage(&iconTypeReq, &reply) != B_OK) return NULL;
	reply.FindInt32("result", &iconType);

	BMessage overIconReq(B_GET_PROPERTY);
	overIconReq.AddSpecifier("overlayIconRef");
	overIconReq.AddSpecifier("message", index);
	
	if (fMsgMsgr->SendMessage(&overIconReq, &reply) != B_OK) return NULL;
	reply.FindRef("result", &overlayRef);

	BMessage overTypeReq(B_GET_PROPERTY);
	overTypeReq.AddSpecifier("overlayIconType");
	overTypeReq.AddSpecifier("message", index);
	
	if (fMsgMsgr->SendMessage(&overTypeReq, &reply) != B_OK) return NULL;
	reply.FindInt32("result", &overlayType);

	IPMessage *message = new IPMessage((InfoPopper::info_type)type);
	message->Application(app.String());
	message->Title(title.String());
	message->Content(content.String());
	message->Progress(progress);
	message->MainIcon(iconRef);
	message->MainIconType(iconType);
	message->OverlayIcon(overlayRef);
	message->OverlayIconType(overlayType);
	
	return message;
};

//#pragma mark Private

status_t IPConnection::FetchMessageMessenger(void) {
	status_t result = B_ERROR;
	
	if (fMsgMsgr == NULL) {
		BMessage reply(B_REPLY);
		BMessage fetch(B_GET_PROPERTY);
		fMsgMsgr = new BMessenger();
		
		fetch.AddSpecifier("Messenger");
		fetch.AddSpecifier("Window", kIPWindowName);
		
		if (fIPMsgr->SendMessage(&fetch, &reply) == B_OK) {
			result = reply.FindMessenger("result", fMsgMsgr);
		};
	} else {
		result = B_OK;
	};
	
	return result;
};

