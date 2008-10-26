#ifndef INFOPOPPERSENDER_H
#define INFOPOPPERSENDER_H

#include <stdlib.h>
#include <ctype.h>

#include <Entry.h>
#include <Looper.h>
#include <MessageRunner.h>

#include <libim/InfoPopper.h>
#include "../../../../common/IMKitUtilities.h"

#include "pluginproto.h"

#include <libxml/nanohttp.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/debugXML.h>

const uint32 kCheckTrack = 'ips1';

enum updateTypes {
	updateConstant = 0,
	updateChange = 1,
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
						int	FetchAlbumCover(BString albumPath, BString search);
					BString	GetNodeContents(xmlNode *node);
					void	URLEncode(BString *str);
	
		SoundPlayController	*fController;
			BMessageRunner	*fRunner;
			PlaylistPtr		fTrack;
					BString	fAlbumPath;

				entry_ref	fLastRef;
					float	fLastPitch;

					int8	fUpdateType;
					BString	fTitleText;
					BString	fMainText;
};

#endif
