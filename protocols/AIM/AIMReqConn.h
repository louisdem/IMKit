#ifndef AIMREQCONN_H
#define AIMREQCONN_H

#include "AIMConnection.h"

class AIMReqConn : public AIMConnection {
	public:
						AIMReqConn(const char *server, uint16 port,
							AIMManager *man);
						~AIMReqConn(void);
		virtual inline
			const char	*ConnName(void) const { return "AIMReqConn"; };

				
	private:
		virtual status_t	HandleServiceControl(BMessage *msg);
		virtual status_t	HandleBuddyIcon(BMessage *msg);
				
			AIMManager	*fManager;
			BMessenger	fManMsgr;
			
		uint8			fState;
};

#endif