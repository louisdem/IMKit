#include "pluginproto.h"

#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <TextControl.h>
#include <Window.h>
#include <View.h>

const int kPadding = 5;

class SPIPConfigView : public BView {
	public:
						SPIPConfigView(BMessage *config);
						~SPIPConfigView(void);
						
				void	AttachedToWindow(void);
				void	MessageReceived(BMessage *msg);

	private:
			BMessage	*fConfig;
			
		BTextControl	*fTitleCtrl;
		BTextControl	*fMainCtrl;
			
			BMenuField	*fBehaviourField;
				BMenu	*fBehaviour;
	
};
