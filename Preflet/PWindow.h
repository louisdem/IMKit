#ifndef PWINDOW_H
#define PWINDOW_H

#include "main.h"

#include "IconTextItem.h"

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Entry.h>
#include <ListView.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include "ObjectList.h"
#include <Path.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>
#include <Window.h>

#include <libim/Constants.h>
#include <libim/Manager.h>

#include "../common/IMKitUtilities.h"

#include <map>

class BWindow;
//class PButtonBox;
//class PSpeedBox;
class BButton;

class PWindow : public BWindow {
	enum {
		LISTCHANGED,
		SAVE,
		REVERT
	};

	public:
		
								PWindow(void);

		virtual bool			QuitRequested(void);
		virtual void			MessageReceived(BMessage *msg);

				
	private:
		status_t				BuildGUI(BMessage viewTemplate, BMessage settings,
									BView *view);
	
		BView					*fView;
		BButton					*fSave;
		BButton					*fRevert;
		BListView				*fListView;
		BBox					*fBox;

		BObjectList<BView>		fPrefView;		
		int32					fLastIndex;

		IM::Manager				*fManager;
		
//		Stored a pair<Settings, Template>
		map<BString, pair<BMessage, BMessage> >
								fAddOns;

		float					fFontHeight;
};

#endif
