#include "TLV.h"

#include <stdio.h>
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
	if (fTLVs.size() > 0) {
		list <TLV *>::iterator i;
		
		for (i = fTLVs.begin(); i != fTLVs.end(); i++) {
			delete (TLV *)(*i);
		};
	};
};

uint16 TLV::Type(void) {
	return fType;
};

void TLV::Type(uint16 type) {
	fDirty = true;
	fType = type;
};

uint16 TLV::Length(void) {
	if (fDirty) {
		if (fTLVs.size() > 0) {
			FlattenedSize();
		};
	};
	
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
		fFlatten = (char *)realloc(fFlatten, FlattenedSize() * sizeof(char));
		fFlatten[0] = (fType & 0xff00) >> 8;
		fFlatten[1] = (fType & 0x00ff);
		fFlatten[2] = (fLength & 0xff00) >> 8;
		fFlatten[3] = (fLength & 0x00ff);
		
		if (fTLVs.size() > 0) {
			list <TLV *>::iterator i;
			
			uint16 offset = 4;
			
			for (i = fTLVs.begin(); i != fTLVs.end(); i++) {
				TLV *tlv = (*i);
				memcpy((void *)(fFlatten + offset), tlv->Flatten(),
					tlv->FlattenedSize());
				offset += tlv->FlattenedSize();
			};
		} else {
			memcpy((void *)(fFlatten + sizeof(fType) + sizeof(fLength)), fValue,
				fLength);
		};
		fDirty = false;
	};
	
	return fFlatten;
};

uint16 TLV::FlattenedSize(void) {
	if (fDirty) {
		if (fTLVs.size() > 0) {
			list <TLV *>::iterator i;
			fFlattenedSize = 0;
			uint16 length = 0;

			for (i = fTLVs.begin(); i != fTLVs.end(); i++) {
				TLV *tlv = (*i);
				length += tlv->FlattenedSize();
			};
			
			fFlattenedSize = length + kTLVOverhead;
			fLength = length;
		} else {
			fFlattenedSize = fLength + kTLVOverhead;
		};
	};
			
	return fFlattenedSize;
};

status_t TLV::AddTLV(TLV *data) {
	fDirty = true;
	fTLVs.push_back(data);

	return B_OK;
};