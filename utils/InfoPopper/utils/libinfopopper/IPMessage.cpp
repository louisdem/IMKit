#include "IPMessage.h"

#include <stdio.h>

//#pragma mark Constructor

IPMessage::IPMessage(info_type type = Information)
	: fType(type),
	fApp(""),
	fTitle(""),
	fContent(""),
	fMessageID(""),
	fProgress(0.0) {
};

IPMessage::~IPMessage(void) {
};

//#pragma mark Public

void IPMessage::Type(info_type type) {
	fType = type;
};

info_type IPMessage::Type(void) {
	return fType;
};

void IPMessage::Application(const char *name) {
	fApp = name;
};

const char *IPMessage::Application(void) {
	return fApp.String();
};
		
void IPMessage::Title(const char *title) {
	fTitle = title;
};

const char *IPMessage::Title(void) {
	return fTitle.String();
};
		
void IPMessage::Content(const char *content) {
	fContent = content;
};

const char *IPMessage::Content(void) {
	return fContent.String();
};
		
void IPMessage::MessageID(const char *id) {
	fMessageID = id;
};

const char *IPMessage::MessageID(void) {
	return fMessageID.String();
};
		
void IPMessage::Progress(float progress) {
	fProgress = progress;
};

float IPMessage::Progress(void) {
	return fProgress;
};

void IPMessage::PrintToStream(FILE *out = stdout) {
	const char *id = "(Message ID)";
	const char *type = NULL;
	if (fMessageID.Length() > 0) id = fMessageID.String();
	switch (fType) {
		case InfoPopper::Information: type = "Information"; break;
		case InfoPopper::Important: type = "Important"; break;
		case InfoPopper::Error: type = "Error"; break;
		case InfoPopper::Progress: type = "Progress"; break;
		default: type = "Unknown";
	};
	
	fprintf(out, "%s (%s)\n", id, type);
	fprintf(out, "\tApplication: %s\n", fApp.String());
	fprintf(out, "\tTitle: %s\n", fTitle.String());
	fprintf(out, "\tContent: %s\n", fContent.String());
};
