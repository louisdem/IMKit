#ifndef INFOPOPPERSENDER_H
#define INFOPOPPERSENDER_H

#include <stdlib.h>

#include <Entry.h>
#include <Looper.h>
#include <MessageRunner.h>

#include <libim/InfoPopper.h>

#include "pluginproto.h"

const uint32 kSendToPopper = 'ips1';

class InfoPopperSender : public BLooper {
	public:
							InfoPopperSender(SoundPlayController *controller);
							~InfoPopperSender(void);
							
					void	MessageReceived(BMessage *msg);	
						
					void	SendMessages(bool send);

	private:
		SoundPlayController	*fController;
			BMessageRunner	*fRunner;
			PlaylistPtr		fTrack;
};

#endif
