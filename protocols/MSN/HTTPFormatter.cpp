#include "HTTPFormatter.h"

HTTPFormatter::HTTPFormatter(void) {
	_init();
};

HTTPFormatter::HTTPFormatter(const char *host, const char *document) {
	_init();
	fHost = host;
	fDocument = document;
};

void HTTPFormatter::_init(void) {
	fHost = "";
	fDocument = "";
	fVersion = "1.1";
	fHeaders.clear();
	fContent = "";
	fDirty = true;
	fFlattened = "";
	fStatus = 0;
	fRequestType = "GET";
};

HTTPFormatter::HTTPFormatter(const char *response, int32 length) {
	BString buffer = response;
	BString line = "";
	int32 seperator = 0;
	int32 position = 0;
	vector <BString> tempVect;
	const char *kSeperator = "\r\n";
	const int32 kSeperatorLen = strlen("\r\n");
	
	while ((seperator = buffer.FindFirst(kSeperator, position)) != B_ERROR) {
		buffer.CopyInto(line, position, seperator - position);

		position = seperator + kSeperatorLen;		
		tempVect.push_back(line);
	};
	
	vector<BString>::iterator i = tempVect.begin();
	line = *i;
	seperator = line.FindFirst("/");
	position = line.FindFirst(" ", seperator);
	line.CopyInto(fVersion, seperator + 1, position - seperator - 1);
	seperator = line.FindFirst(" ", position);
	line.CopyInto(buffer, position + 1, seperator - position - 1);
	fStatus = atol(buffer.String());
	i++;
	
	for (; i != tempVect.end(); i++) {
		BString line = *i;
		if ((seperator = line.FindFirst(": ")) != B_ERROR) {
			BString name;
			BString value;
			line.CopyInto(name, 0, seperator);
			line.CopyInto(value, seperator + 2, line.Length() - (seperator + 2));

			fHeaders[name] = value;
		};
	};
};

HTTPFormatter::~HTTPFormatter(void) {
	fHeaders.clear();
};

status_t HTTPFormatter::Host(const char *host) {
	fDirty = true;
	fHost = host;
	return B_OK;
};

const char *HTTPFormatter::Host(void) {
	return fHost.String();
};

status_t HTTPFormatter::Document(const char *document) {
	fDirty = true;
	fDocument = document;
	return B_OK;
};		

const char *HTTPFormatter::Document(void) {
	return fDocument.String();
};
					
status_t HTTPFormatter::Version(const char *version) {
	fDirty = true;
	fVersion = version;
	return B_OK;
};

const char *HTTPFormatter::Version(void) {
	return fVersion.String();
};

status_t HTTPFormatter::RequestType(const char *request) {
	fDirty = true;
	fRequestType = request;
	return B_OK;
};

const char *HTTPFormatter::RequestType(void) {
	return fRequestType.String();
};

status_t HTTPFormatter::AddHeader(const char *name, const char *value) {
	fDirty = true;
	fHeaders[name] = value;
	return B_OK;
};

status_t HTTPFormatter::ClearHeaders(void) {
	fDirty = true;
	fHeaders.clear();
	return B_OK;
};

const char *HTTPFormatter::HeaderContents(const char *name) {
	HeaderMap::iterator i = fHeaders.find(name);
	
	if (i != fHeaders.end()) {
		return i->second.String();
	} else {
		return NULL;
	};
};

const char *HTTPFormatter::HeaderNameAt(int32 index) {
	HeaderMap::iterator i = fHeaders.begin();
	for (; index > 0; index--) i++;
//	i += index;
	
	if (i != fHeaders.end()) {
		return i->first.String();
	} else {
		return NULL;
	};
};

const char *HTTPFormatter::HeaderAt(int32 index) {
	HeaderMap::iterator i = fHeaders.begin();
	for (; index > 0; index--) i++;
//	i += index;
	
	if (i != fHeaders.end()) {
		return i->second.String();
	} else {
		return NULL;
	};
};

status_t HTTPFormatter::SetContent(const char *content, size_t length) {
	fDirty = true;
	fContent = content;
	return B_OK;
};

status_t HTTPFormatter::AppendContent(const char *content, size_t length) {
	fDirty = true;
	fContent << content;
	return B_OK;
};

const char *HTTPFormatter::Content(void) {
	return fContent.String();
};
status_t HTTPFormatter::ClearContent(void) {
	fContent = "";
	return B_OK;
};
		
status_t HTTPFormatter::Clear(void) {
	_init();
	return B_OK;
};

int32 HTTPFormatter::Length(void) {
	if (fDirty) {
		Flatten();
	};
	
	return fFlattened.Length();
};

const char *HTTPFormatter::Flatten(void) {
	if (fDirty) {
		fFlattened = fRequestType;
		fFlattened << " " << fDocument << " HTTP/" << fVersion << "\r\n";
		fFlattened << "Host: " << fHost << "\r\n";
		
		HeaderMap::iterator i;

		for (i = fHeaders.begin(); i != fHeaders.end(); i++) {
			fFlattened << i->first.String() << ": " << i->second.String() << "\r\n";
		};

		fFlattened << "\r\n\r\n";

		fDirty = false;
	};
	
	return fFlattened.String();
};

