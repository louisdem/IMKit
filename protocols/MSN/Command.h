#ifndef COMMAND_H
#define COMMAND_H

#include <vector>
#include <String.h>
#include <map.h>

class Command {
	public:
					Command(const char *type);
					~Command(void);
				
		const char *TypeStr(void) { return fType.String(); };
		BString		Type(void) { return fType; };
		status_t	AddParam(const char *param, bool encode = false);
		const char	*Param(int32 index);
		int32		Params(void) { return fParams.size(); };
		
		const char	*Flatten(int32 sequence);
		int32		FlattenedSize(void);
		
		int32		TransactionID(void) { return fTrID; };

		status_t	AddPayload(const char *payload, uint32 length, bool encode = true);
		const char	*Payload(int32 index);
		int32		Payloads(void) { return fPayloads.size(); };

		status_t	MakeObject(const char *string);

		bool		UseTrID(void) { return fUseTrID; };
		void		UseTrID(bool use) { fUseTrID = use; };

		bool		ExpectsPayload(int32 *payloadSize);
	private:
		int32		fTrID;
		bool		fUseTrID;
		bool		fDirty;
		BString		fFlattened;

		BString		fType;
		vector<BString>
					fParams;
		vector<BString>
					fPayloads;
		

		map<BString, bool>
					gExpectsPayload;
};

#endif
