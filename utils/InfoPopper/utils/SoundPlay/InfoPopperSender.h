#ifndef INFOPOPPERSENDER_H
#define INFOPOPPERSENDER_H

#include <stdlib.h>

#include <Entry.h>
#include <Looper.h>
#include <MessageRunner.h>

#include <libim/InfoPopper.h>
#include "../../../../common/IMKitUtilities.h"

#include "pluginproto.h"

const uint32 kCheckTrack = 'ips1';

enum updateTypes {
	updateConstant = 0,
	updateFileChange = 1,
};

class InfoPopperSender : public BLooper {
	public:
							InfoPopperSender(SoundPlayController *controller);
							~InfoPopperSender(void);
							
					void	MessageReceived(BMessage *msg);	
						
					void	SendMessages(bool send);
					
					void	UpdateType(int8 type);
					int8	UpdateType(void);
					
					void	TitleText(const char *text);
				const char	*TitleText(void);
				
					void	MainText(const char *text);
				const char	*MainText(void);

	private:
		SoundPlayController	*fController;
			BMessageRunner	*fRunner;
			PlaylistPtr		fTrack;
					BString	fAlbumPath;
				entry_ref	fLastRef;

					int8	fUpdateType;
					BString	fTitleText;
					BString	fMainText;
};

#endif
