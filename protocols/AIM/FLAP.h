#ifndef FLAP_H
#define FLAP_H

#include <be/support/SupportDefs.h>
#include <list>

#include "TLV.h"
#include "SNAC.h"

typedef pair <uchar *, uint32> BufferLengthPair;
typedef pair <void *, uint8> TypeDataPair;

const uchar COMMAND_START = 0x2a;

enum flap_channel {
	OPEN_CONNECTION = 0x01,
	SNAC_DATA = 0x02,
	FLAP_ERROR = 0x03,
	CLOSE_CONNECTION = 0x04,
	KEEP_ALIVE = 0x05
};

enum snac_family {
	SERVICE_CONTROL = 0x0001,
	LOCATION = 0x0002,
	BUDDY_LIST_MANAGEMENT = 0x0003,
	ICBM = 0x0004,
	ADVERTISEMENTS = 0x0005,
	INVITATION = 0x0006,
	ADMINISTRATIVE = 0x0007,
	POPUP_NOTICES = 0x0008,
	PRIVACY_MANAGEMENT = 0x0009,
	USER_LOOKUP = 0x000a,
	USAGE_STATS = 0x000b,
	TRANSLATION = 0x000c,
	CHAT_NAVIGATION = 0x000d,
	CHAT = 0x000e,
	DIRECTORY_USER_SEARCH = 0x00f,
	SERVER_STORED_BUDDY_ICONS = 0x0010,
	SERVER_SIDE_INFORMATION = 0x0013,
	ICQ_SPECIFIC_EXTENSIONS = 0x0015,
	AUTHORISATION_REGISTRATION = 0x0017
};

enum snac_subtype {
//	Family 1 - Service

	CLIENT_READY = 0x0002,
	SERVER_SUPPORTED_SNACS = 0x0003,
	REQUEST_RATE_LIMITS = 0x0006,
	RATE_LIMIT_RESPONSE = 0x0007,
	RATE_LIMIT_ACK = 0x0008,
	SET_PRIVACY_FLAGS = 0x0014,
	UPDATE_STATUS = 0x001e,
	VERIFICATION_REQUEST = 0x001f,		// Evil AOLsses, we hateses them, we do.

//	Family 2 - Location
	CLIENT_SERVER_ERROR = 0x0001,
	REQUEST_LIMITATIONS = 0x0002,
	LIMITIATIONS_RESPONSE = 0x0003,
	SET_USER_INFORMATION = 0x0004,
	REQUEST_USER_INFORMATION = 0x0005,
	USER_INFORMATION = 0x0006,
	
//	Family 3 - Buddy List
	ADD_BUDDY_TO_LIST = 0x0004,
	USER_ONLINE = 0x000b,
	USER_OFFLINE = 0x000c,
	
//	 Family 4 - ICBM
	SET_ICBM_PARAMS = 0x0002,
	SEND_MESSAGE_VIA_SERVER = 0x0006,
	MESSAGE_FROM_SERVER = 0x0007
};

enum tlv_type {
	SCREEN_NAME = 0x0001,
};

enum icbm_channel {
	PLAIN_TEXT = 0x0001,
	RTF_RENDEZVOUS = 0x0002,
	TYPED_OLD_STYLE = 0x0004
};

class Flap {
	public:

	enum {
		DATA_TYPE_RAW = 1,
		DATA_TYPE_TLV = 2,
		DATA_TYPE_SNAC = 3
	};

					Flap(uint8 channel);
					Flap(void);
					~Flap(void);
			void	Channel(uint8);
		uint8		Channel(void) const;
		status_t	AddRawData(unsigned char *data, uint16 length);
		status_t	AddRawData(uint16 raw);
		status_t	AddTLV(TLV *data);
		status_t	AddTLV(uint16 type, const char *value, uint16 length);
		status_t	AddSNAC(SNAC *snac);
		const char	*Flatten(uint16 seqNum);
		uint32		FlattenedSize(void);
		void		Clear(void);

	private:
		bool		fDirty;
		uint32		fFlattenedSize;
		char		*fFlat;
		uint8		fChannelID;
		uint16		fSequenceID;
		uint16		fLength;		
		list<TypeDataPair>
					fData;
};

#endif
