#include "Command.h"

#include <stdio.h>

Command::Command(const char *type)
	: fDirty(true),
	fType(type),
	fTrID(-1),
	fUseTrID(true) {
		
	fType.ToUpper();

	gExpectsPayload["MSG"] = true;
};

Command::~Command(void) {
};

status_t Command::AddParam(const char *param, bool encode = false) {
	fDirty = true;

	BString added = param;
	if (encode) {
//		Yoinked from Vision's Utilities.cpp		
		added.ReplaceAll ("%",  "%25"); // do this first!
		added.ReplaceAll ("\n", "%20");
		added.ReplaceAll (" ",  "%20");
		added.ReplaceAll ("\"", "%22");
		added.ReplaceAll ("#",  "%23");
		added.ReplaceAll ("@",  "%40");
		added.ReplaceAll ("`",  "%60");
		added.ReplaceAll (":",  "%3A");
		added.ReplaceAll ("<",  "%3C");
		added.ReplaceAll (">",  "%3E");
		added.ReplaceAll ("[",  "%5B");
		added.ReplaceAll ("\\", "%5C");
		added.ReplaceAll ("]",  "%5D");
		added.ReplaceAll ("^",  "%5E");
		added.ReplaceAll ("{",  "%7B");
		added.ReplaceAll ("|",  "%7C");
		added.ReplaceAll ("}",  "%7D");
		added.ReplaceAll ("~",  "%7E");
		added.ReplaceAll ("/",  "%2F");
		added.ReplaceAll ("=",  "%3D");
		added.ReplaceAll ("+",  "%2B");
		
	};
	
	fParams.push_back(added);
	
	return B_OK;
};

const char *Command::Param(int32 index, bool decode = false) {
	BString param = fParams[index];
	if (decode == true) {
		param.IReplaceAll("%20", " ");
		param.IReplaceAll("%22", "\"");
		param.IReplaceAll("%23", "#");
		param.IReplaceAll("%40", "@");
		param.IReplaceAll("%60", "`");
		param.IReplaceAll("%3A", ":");
		param.IReplaceAll("%3C", "<");
		param.IReplaceAll("%3E", ">");
		param.IReplaceAll("%5B", "[");
		param.IReplaceAll("%5C", "\\");
		param.IReplaceAll("%5D", "]");
		param.IReplaceAll("%5E", "^");
		param.IReplaceAll("%7B", "{");
		param.IReplaceAll("%7C", "|");
		param.IReplaceAll("%7D", "}");
		param.IReplaceAll("%7E", "~");
		param.IReplaceAll("%2F", "/");
		param.IReplaceAll("%3D", "=");
		param.IReplaceAll("%2B", "+");
		param.IReplaceAll("%25", "%");
	};
	
	return param.String();
};

status_t Command::AddPayload(const char *payload, int32 length = -1, bool encode = true) {
	fDirty = true;

	if (length == -1) length = strlen(payload);

	BMallocIO content;
	content.WriteAt(0, payload, length);
	fPayloads.push_back(content);
	
	return B_OK;
};

const char *Command::Payload(int32 index) {
	return fPayloads[index].AsString();
}

const char *Command::Flatten(int32 sequence) {
	if ((fDirty == true) || (fTrID != sequence)) {
		int32 offset = 0;
		BString temp = "";
		fTrID = sequence;
		
		fFlattened.WriteAt(offset, fType.String(), fType.Length());
		offset += fType.Length();
		
		if (fUseTrID) {
			temp << " " << fTrID;
			fFlattened.WriteAt(offset, temp.String(), temp.Length());
			offset += temp.Length();
		};
		
		if (Params() > 0) {
			vector<BString>::iterator i;
			for (i = fParams.begin(); i != fParams.end(); i++) {
				fFlattened.WriteAt(offset++, " ", 1);
				fFlattened.WriteAt(offset, i->String(), i->Length());
				offset += i->Length();
			};
		};
		
		if (Payloads() > 0) {
			payloadv::iterator i;
			int32 payload = 0;
			temp = " ";
			
			for (i = fPayloads.begin(); i != fPayloads.end(); i++) {
				payload += i->BufferLength();
			};
			
			temp << payload;
			fFlattened.WriteAt(offset, temp.String(), temp.Length());
			offset += temp.Length();
		};
	
		fFlattened.WriteAt(offset, "\r\n", strlen("\r\n"));
		offset += strlen("\r\n");
		
		if (Payloads() > 0) {
			payloadv::iterator i;

			for (i = fPayloads.begin(); i != fPayloads.end(); i++) {
				fFlattened.WriteAt(offset, i->Buffer(), i->BufferLength());
				offset += i->BufferLength();
			};
		};
		
		fDirty = false;
	};
	
	return (char *)fFlattened.Buffer();
};

int32 Command::FlattenedSize(void) {
	return fFlattened.BufferLength();
//	if (fDirty) {
//		int32 payloadSize = 0;
//		int32 size = 0;
//		BString temp = "";
//		
//		size += fType.Length();
//
//		if (fUseTrID) {
//			size++; // Space
//			temp << fTrID;
//			size += temp.Length();
//		};
//		if (Params() > 0) {
//			vector<BString>::iterator i;
//			for (i = fParams.begin(); i != fParams.end(); i++) 
//				size += i->Length() + 1;
//		}
//		
//		if (Payloads() > 0 ) {
//			vector<clpair*>::iterator i;
//			for (i = fPayloads.begin(); i != fPayloads.end(); i++) 
//				payloadSize += (*i)->length;
//			
//			temp = "";
//			temp << payloadSize;
//			size += temp.Length() + 1;
//			
//			size += payloadSize;
//		};
//		
//		size += strlen("\r\n");
//		
//		fFlattenedSize = size;
//	};
//	
//	return fFlattenedSize;
};

void Command::Debug(void) {
	printf("%s {TrID}", fType.String());
	vector<BString>::iterator i;
	for (i = fParams.begin(); i != fParams.end(); i++) printf(" %s", i->String());
	
	payloadv::iterator j;

	if (Payloads() > 0) {
		int32 size = 0;
		for (j = fPayloads.begin(); j != fPayloads.end(); j++) size += j->BufferLength();
		
		printf(" %i", size);
	};
	
	printf("\r\n");
	
	if (Payloads() > 0) {
		for (j = fPayloads.begin(); j != fPayloads.end(); j++) {
			char *buffer = (char *)j->Buffer();
			for (int32 i = 0; i < j->BufferLength(); i++) {
				if ((buffer[i] = '\r') || (buffer[i] == '\n'))
					printf("%c", buffer[i]);
				else if ((buffer[i] < 0x20) || (buffer[i] > 0x7e))
					printf("0x%02x ", buffer[i]);
				else 
					printf("%c", buffer[i]);
			};
		};
	};
	
	printf("\r\n");
};

status_t Command::MakeObject(const char *string) {
	BString command = string;
	int32 seperator = 0;
	int32 position = 0;
	fDirty = true;
	BString temp = "";
	
	seperator = command.FindFirst(" ");
	if (seperator == B_ERROR) 
		return B_ERROR;
	
	command.CopyInto(fType, 0, seperator);
	fType.ToUpper();
	
	position = seperator + 1;
	
	seperator = command.FindFirst(" ", position);
	if (seperator == B_ERROR) {
		fTrID = 0;
		command.CopyInto(temp, position, command.Length() - position);
		temp.ReplaceLast("\r\n", "");
		fParams.push_back(temp.String());
		return B_OK;
	} else {
		temp = "";
		command.CopyInto(temp, position, seperator-position);
		
		fTrID = atol(temp.String());
		if ((fTrID == 0) && (temp != "0")) fParams.push_back(temp);
	};
	
	position = seperator;
	
	while ((seperator = command.FindFirst(" ", position)) != B_ERROR) {
		int32 wordBoundary = command.FindFirst(" ", seperator + 1);
		temp = "";
		command.CopyInto(temp, position + 1, wordBoundary - position - 1);

		fParams.push_back(temp.String());
		position = wordBoundary;
	};
	
	fParams.back().ReplaceLast("\r\n", "");
	
	return B_OK;
};

bool Command::ExpectsPayload(int32 *payloadSize) {
	bool ret = false;
	map <BString, bool>::iterator it = gExpectsPayload.find(fType);
	*payloadSize = 0;

	if (it != gExpectsPayload.end()) ret = it->second;
	if (ret == true) *payloadSize = atol(Param(Params() - 1));
	
	return ret;
};
