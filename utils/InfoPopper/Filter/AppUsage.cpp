#include "Filter/AppUsage.h"

#include "Filter/Notification.h"

#include <Message.h>

#include <libim/InfoPopper.h>

#include <stdlib.h>


#include <stdio.h>


//#pragma mark Constants
const type_code kTypeCode = 'ipau';

//#pragma mark Constructor

AppUsage::AppUsage(void)
	: fName(""),
	fAllow(true) {
};

AppUsage::AppUsage(entry_ref ref, const char *name, bool allow = true)
	: fRef(ref),
	fName(name),
	fAllow(allow) {
	
};

AppUsage::~AppUsage(void) {
	notify_t::iterator nIt;
	for (nIt = fNotifications.begin(); nIt != fNotifications.end(); nIt++) {
		delete nIt->second;
	};
};

//#pragma mark BFlattenable Hooks

bool AppUsage::AllowsTypeCode(type_code code) const {
	return code == kTypeCode;
};

status_t AppUsage::Flatten(void *buffer, ssize_t numBytes) const {
	BMessage msg;
	msg.AddString("app_name", fName);
	msg.AddRef("app_ref", &fRef);
	msg.AddBool("app_allow", fAllow);

	notify_t::const_iterator nIt;
	for (nIt = fNotifications.begin(); nIt != fNotifications.end(); nIt++) {
		msg.AddFlat("notify", nIt->second);
	};

	if (numBytes < msg.FlattenedSize()) return B_ERROR;
	
	return msg.Flatten((char *)buffer, numBytes);	
};

ssize_t AppUsage::FlattenedSize(void) const {
	BMessage msg;
	msg.AddString("app_name", fName);
	msg.AddRef("app_ref", &fRef);
	msg.AddBool("app_allow", fAllow);
	
	notify_t::const_iterator nIt;
	for (nIt = fNotifications.begin(); nIt != fNotifications.end(); nIt++) {
		msg.AddFlat("notify", nIt->second);
	};

	return msg.FlattenedSize();
};

bool AppUsage::IsFixedSize(void) const {
	return false;
};

type_code AppUsage::TypeCode(void) const {
	return kTypeCode;
};

status_t AppUsage::Unflatten(type_code code, const void *buffer,
	ssize_t numBytes) {
	
	if (code != kTypeCode) return B_ERROR;
	
	BMessage msg;
	status_t status = B_ERROR;
	
	status = msg.Unflatten((const char *)buffer);
	
	if (status == B_OK) {
		msg.FindString("app_name", &fName);
		msg.FindRef("app_ref", &fRef);
		msg.FindBool("app_allow", &fAllow);

		type_code type;
		int32 count = 0;
		
		status = msg.GetInfo("notify", &type, &count);
		if (status != B_OK) return status;
		
		for (int32 i = 0; i < count; i++) {
			Notification *notify = new Notification();
			msg.FindFlat("notify", i, notify);
			fNotifications[notify->Title()] = notify;
		};
		
		status = B_OK;
	};
	
	return status;
};

//#pragma mark Public
						
entry_ref AppUsage::Ref(void) {
	return fRef;
};

const char *AppUsage::Name(void) {
	return fName.String();
};

bool AppUsage::Allowed(const char *title, info_type type) {
	bool allowed = fAllow;
	
	if (allowed) {
		notify_t::iterator nIt = fNotifications.find(title);
		if (nIt == fNotifications.end()) {
			allowed = true;		
			fNotifications[title] = new Notification(title, type);
		} else {
			allowed = nIt->second->Allowed();
			nIt->second->UpdateTimeStamp();
			nIt->second->InfoType(type);
		};
	};

	return allowed;
};

bool AppUsage::Allowed(void) {
	return fAllow;
};

Notification *AppUsage::NotificationAt(int32 index) {
	notify_t::iterator nIt = fNotifications.begin();
	for (int32 i = 0; i < index; i++) nIt++;
	
	return nIt->second;
};

int32 AppUsage::Notifications(void) {
	return fNotifications.size();
};

void AppUsage::AddNotification(Notification *notification) {
	fNotifications[notification->Title()] = notification;
};
