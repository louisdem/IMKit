#ifndef MSNCONSTANTS_H
#define MSNCONSTANTS_H

extern const char *kProtocolName;
extern const char *kThreadName;

extern const char *kClientVer;
extern const char *kProtocolsVers;

extern const int16 kDefaultPort;

extern const uint32 kOurCaps;
extern const char *kClientIDString;
extern const char *kClientIDCode;

enum online_types {
	otOnline,
	otAway,
	otBusy,
	otIdle,
	otBRB,
	otPhone,
	otLunch,
	otInvisible,
	otBlocked,
	otConnecting,
	otOffline
};

enum list_types {
	ltForwardList,
	ltReverseList,
	ltAllowList,
	ltBlockList	
};

enum typing_notification {
	tnStartedTyping,
	tnStillTyping,
	tnStoppedTyping
};

enum client_caps {
	ccMobileDev = 0x00000001,
	ccUnknown1 = 0x00000002,
	ccViewInk = 0x00000004,
	ccMakeInk = 0x00000008,
	ccVideoChat = 0x00000010,
	ccUnknown2 = 0x00000020,
	ccHasMobileDev = 0x00000040,
	ccHasDirectDev = 0x00000080,
	ccMSNC1 = 0x10000000,
	ccMSNC2 = 0x20000000
};

#endif
