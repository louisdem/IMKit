#include "TLV.h"

#include <stdlib.h>
#include <string.h>

const uint8 kTLVOverhead = 4;

TLV::TLV(void) {
	fValue = NULL;
	fFlatten = NULL;
};

TLV::TLV(uint16 type) {
	fDirty = true;
	fType = type;
	fValue = NULL;
	fFlatten = NULL;
};

TLV::TLV(uint16 type, const char *value, uint16 length) {
	fFlatten = NULL;
	fDirty = true;
	fType = type;
	fFlatten = NULL;
	fLength = length;
	fValue = NULL;
	fValue = (char *)realloc(fValue, length * sizeof(char));
	memcpy(fValue, value, length);
};

TLV::~TLV(void) {
	if (fValue) free(fValue);
	if (fFlatten) free(fFlatten);
};

uint16 TLV::Type(void) {
	return fType;
};

void TLV::Type(uint16 type) {
	fDirty = true;
	fType = type;
};

uint16 TLV::Length(void) {
	return fLength;
};

status_t TLV::Value(const char *value, uint16 length) {
	fDirty = true;
	
	status_t r = B_ERROR;
	fValue = (char *)realloc(fValue, length * sizeof(char));
	fLength = length;
	if (fValue) {
		memcpy(fValue, value, length);
		r = B_OK;
	};
	
	return r;
};

const char *TLV::Value(void) {
	return fValue;
};

const char *TLV::Flatten(void) {
	if (fDirty) {
		fFlatten = (char *)realloc(fFlatten,
			(fLength + kTLVOverhead) * sizeof(char));
		fFlatten[0] = (fType & 0xff00) >> 8;
		fFlatten[1] = (fType & 0x00ff);
		fFlatten[2] = (fLength & 0xff00) >> 8;
		fFlatten[3] = (fLength & 0x00ff);
		memcpy((void *)(fFlatten + sizeof(fType) + sizeof(fLength)), fValue,
			fLength);
		fDirty = false;
	};
	
	return fFlatten;
};

uint16 TLV::FlattenedSize(void) const {
	return fLength + kTLVOverhead;
};
