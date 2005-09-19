#ifndef IPCONNECTION_H
#define IPCONNECTION_H

#include <Messenger.h>

class IPMessage;

class IPConnection {
	public:
								IPConnection(void);
								~IPConnection(void);
	
		int16					IconSize(void);
		
		status_t				Send(IPMessage *message);
		
		int32					CountMessages(void);
		IPMessage				*MessageAt(int32 index);
	
	private:
		status_t				FetchMessageMessenger(void);
	
		BMessenger				*fIPMsgr;
		BMessenger				*fMsgMsgr;
};

#endif
