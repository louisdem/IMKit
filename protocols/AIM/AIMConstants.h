#ifndef AIMCONSTANTS_H
#define AIMCONSTANTS_H

extern const char *kProtocolName;
extern const char *kThreadName;

extern const uint16 AIM_ERROR_COUNT;
extern const char *kErrors[];

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
	ERROR = 0x0001,						// This is the same for all SNAC families

//	Family 1 - Service

	CLIENT_READY = 0x0002,
	SERVER_SUPPORTED_SNACS = 0x0003,
	REQUEST_NEW_SERVICE = 0x0004,
	SERVICE_REDIRECT = 0x0005,
	REQUEST_RATE_LIMITS = 0x0006,
	RATE_LIMIT_RESPONSE = 0x0007,
	RATE_LIMIT_ACK = 0x0008,
	OWN_ONLINE_INFO = 0x000f,
	SET_PRIVACY_FLAGS = 0x0014,
	SERVER_FAMILY_VERSIONS = 0x0018,
	UPDATE_STATUS = 0x001e,
	VERIFICATION_REQUEST = 0x001f,		// Evil AOLsses, we hateses them, we do.

//	Family 2 - Location
	REQUEST_LIMITATIONS = 0x0002,
	LIMITIATIONS_RESPONSE = 0x0003,
	SET_USER_INFORMATION = 0x0004,
	REQUEST_USER_INFORMATION = 0x0005,
	USER_INFORMATION = 0x0006,
	
//	Family 3 - Buddy List
	ADD_BUDDY_TO_LIST = 0x0004,
	REMOVE_BUDDY_FROM_LIST = 0x0005,
	USER_ONLINE = 0x000b,
	USER_OFFLINE = 0x000c,
	
//	Family 4 - ICBM
	SET_ICBM_PARAMS = 0x0002,
	SEND_MESSAGE_VIA_SERVER = 0x0006,
	MESSAGE_FROM_SERVER = 0x0007,
	TYPING_NOTIFICATION = 0x0014,

//	Family 13 - SSI
	REQUEST_LIST = 0x0004,
	ROSTER_CHECKOUT = 0x0006,
	ACTIVATE_SSI_LIST = 0x0007,
	SSI_DELETE_ITEM = 0x000a,
	SSI_EDIT_BEGIN = 0x0011,
	SSI_EDIT_END = 0x0012
};

enum typing_notification {
	FINISHED_TYPING = 0x0000,
	STILL_TYPING = 0x0001,
	STARTED_TYPING = 0x0002
};

enum tlv_type {
	SCREEN_NAME = 0x0001,
};

enum icbm_channel {
	PLAIN_TEXT = 0x0001,
	RTF_RENDEZVOUS = 0x0002,
	TYPED_OLD_STYLE = 0x0004
};

enum user_class {
	CLASS_UNCONFIRMED = 0x0001,
	CLASS_ADMINISTRATOR = 0x0002,
	CLASS_AOL = 0x0004,
	CLASS_COMMERCIAL = 0x0008,
	CLASS_FREE = 0x0010,
	CLASS_AWAY = 0x0020,
	CLASS_ICQ = 0x0040,
	CLASS_WIRELESS = 0x0080,
};

enum ssi_item_types {
	BUDDY_RECORD = 0x0000,
	GROUP_RECORD = 0x0001,
	PERMIT_RECORD = 0x0002,
	DENY_RECORD = 0x0003,
	PERMIT_DENY_SETTINGS = 0x0004,
	PRESENCE_INFO = 0x0005,
	UNKOWN = 0x0009,
	IGNORE_LIST = 0x000e,
	NON_ICQ_CONTACT = 0x0010,
	IMPORT_TIME = 0x0013,
	BUDDY_ICON_INFO = 0x0014
};

//	Internal status types
enum online_types {
	AMAN_OFFLINE = 0,
	AMAN_CONNECTING = 1,
	AMAN_AWAY = 2,
	AMAN_ONLINE = 3
};

#endif
