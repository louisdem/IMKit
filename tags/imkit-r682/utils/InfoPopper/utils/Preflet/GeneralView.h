#ifndef GENERALVIEW_H
#define GENERALVIEW_H

#include <View.h>

#include "SettingView.h"

class BBitmap;
class BBox;
class BMenuField;
class BMenu;
class BNode;
class BTextControl;

class GeneralView : public SettingView {
	public:
							GeneralView(BRect bounds);
							~GeneralView(void);
							
	// BView Hooks
		void				AttachedToWindow(void);
		void				MessageReceived(BMessage *msg);
		void				Draw(BRect rect);
	
	// SettingView hooks
		status_t			Save(void);
		status_t			Load(void);
	
	private:
		BNode				*GetSettingsNode(void);
		
		BMenuField			*fTitlePositionField;
		BMenu				*fTitlePosition;
		BTextControl		*fIconSize;
		BTextControl		*fWindowWidth;
		BTextControl		*fDisplayTime;
		BBitmap				*fExample;
		BBox				*fExampleBox;
		
		BNode				*fSettings;
};

#endif
