#include "Command.h"

#include <stdio.h>


Command::Command(const char *type)
	: fDirty(true),
	fFlattened(""),
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
		param.IReplaceAll("%25", "%");
	};
	
	return param.String();
};

status_t Command::AddPayload(const char *payload, uint32 length, bool encode = true) {
	fDirty = true;
	fPayloads.push_back(payload);

	return B_OK;
};

const char *Command::Payload(int32 index) {
	return fPayloads[index].String();
}

const char *Command::Flatten(int32 sequence) {
	if ((fDirty == false) && (sequence == fTrID)) {
	} else {
		fTrID = sequence;

		fFlattened = fType;
		
		if (fUseTrID) fFlattened << " " << fTrID;
		fFlattened << " ";
		
		vector<BString>::iterator i;
		
		for (i = fParams.begin(); i != fParams.end(); i++) {
			fFlattened << i->String() << " ";
		};
		
//		Ditch the trailing space.
		fFlattened.Truncate(fFlattened.Length() - 1);
		
		if (Payloads() > 0) {
			int32 size = 0;
			for (i = fPayloads.begin(); i != fPayloads.end(); i++) {
				size += i->Length();
			};
			
			fFlattened << " " << size;
		};
		
		fFlattened << "\r\n";
		
		if (Payloads() > 0) {
			for (i = fPayloads.begin(); i != fPayloads.end(); i++) {
				fFlattened += i->String();
			};
		};
		
		fDirty = false;
	};
	
	return fFlattened.String();
};

void Command::Debug(void) {
	printf("%s {TrID}", fType.String());
	vector<BString>::iterator i;
	for (i = fParams.begin(); i != fParams.end(); i++) printf(" %s", i->String());
	
	if (Payloads() > 0) {
		int32 size = 0;
		for (i = fPayloads.begin(); i != fPayloads.end(); i++) size += i->Length();
		
		printf(" %i", size);
	};
	
	printf("\r\n");
	
	if (Payloads() >? 0) {
		for (i = fPayloads.begin(); i != fPayloads.end(); i++) printf("%s\r\n", i->String());
	};
};

int32 Command::FlattenedSize(void) {
	return fFlattened.Length();
};

status_t Command::MakeObject(const char *string) {
	BString command = string;
	int32 seperator = 0;
	int32 position = 0;
	fDirty = true;
	BString temp = "";
	
	seperator = command.FindFirst(" ");
	if (seperator == B_ERROR) return B_ERROR;
	
	command.CopyInto(fType, 0, seperator);
	fType.ToUpper();
	
	position = seperator + 1;
	
	seperator = command.FindFirst(" ", position);
	if (seperator == B_ERROR) {
		fTrID = 0;
		command.CopyInto(temp, position, command.Length() - position);
		fParams.push_back(command.String());
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
};

bool Command::ExpectsPayload(int32 *payloadSize) {
	bool ret = false;
	map <BString, bool>::iterator it = gExpectsPayload.find(fType);
	*payloadSize = 0;

	if (it != gExpectsPayload.end()) ret = it->second;
	if (ret == true) *payloadSize = atol(Param(Params() - 1));
	
	return ret;
};
