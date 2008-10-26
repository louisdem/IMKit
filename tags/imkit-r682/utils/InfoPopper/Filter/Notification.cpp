#include "Notification.h"

#include <Message.h>

#include <stdlib.h>

#include <stdio.h>

//#pragma mark Constants

const type_code kTypeCode = 'ipnt';

//#pragma mark Constructor

Notification::Notification(void)
	: fTitle(""),
	fType(InfoPopper::Information),
	fEnabled(false),
	fLastReceived(time(NULL)) {
};

Notification::Notification(const char *title, info_type type, bool enabled = true)
	: fTitle(title),
	fType(type),
	fEnabled(enabled),
	fLastReceived(time(NULL)) {
};

Notification::~Notification(void) {
};

//#pragma mark BFlattenable Hooks

bool Notification::AllowsTypeCode(type_code code) const {
	return code == kTypeCode;
};

#include <stdio.h>

status_t Notification::Flatten(void *buffer, ssize_t numBytes) const {
	BMessage msg;
	msg.AddString("notify_title", fTitle);
	msg.AddInt32("notify_type", (int32)fType);
	msg.AddInt32("notify_lastreceived", (int32)fLastReceived);
	msg.AddBool("notify_enabled", fEnabled);

	if (numBytes < msg.FlattenedSize()) return B_ERROR;
		
	return msg.Flatten((char *)buffer, numBytes);
};

ssize_t Notification::FlattenedSize(void) const {
	BMessage msg;
	msg.AddString("notify_title", fTitle);
	msg.AddInt32("notify_type", (int32)fType);
	msg.AddInt32("notify_lastreceived", (int32)fLastReceived);
	msg.AddBool("notify_enabled", fEnabled);
	
	return msg.FlattenedSize();
};

bool Notification::IsFixedSize(void) const {
	return false;
};

type_code Notification::TypeCode(void) const {
	return kTypeCode;
};

status_t Notification::Unflatten(type_code code, const void *buffer,
	ssize_t numBytes) {
	
	if (code != kTypeCode) return B_ERROR;

	BMessage msg;
	status_t error = msg.Unflatten((const char *)buffer);
	
	if (error == B_OK) {
		msg.FindString("notify_title", &fTitle);
		msg.FindInt32("notify_type", (int32 *)&fType);
		msg.FindInt32("notify_lastreceived", (int32 *)&fLastReceived);
		msg.FindBool("notify_enabled", &fEnabled);
	};
	
	return error;
};

//#pragma mark Public								

const char *Notification::Title(void) {
	return fTitle.String();
};

info_type Notification::InfoType(void) {
	return fType;
};

void Notification::InfoType(info_type type) {
	fType = type;
};

time_t Notification::LastReceived(void) {
	return fLastReceived;
};

bool Notification::Allowed(void) {
	return fEnabled;
};

void Notification::UpdateTimeStamp(void) {
	fLastReceived = time(NULL);
};

void Notification::SetTimeStamp(time_t time) {
	fLastReceived = time;
};
