#ifndef PWINDOW_H
#define PWINDOW_H

#include "main.h"

#include "IconTextItem.h"

#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <Entry.h>
#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include "ObjectList.h"
#include <OutlineListView.h>
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
#include "../common/BubbleHelper.h"

#include <map>

class BWindow;
class BButton;
class SettingView;

typedef map<BString, SettingView *> view_map;

class PWindow : public BWindow {
	enum {
		LISTCHANGED,
		SAVE,
		REVERT
	};

	public:
		
								PWindow(void);

		// Hooks
		virtual bool			QuitRequested(void);
		virtual void			DispatchMessage(BMessage *msg, BHandler *target);
		virtual void			MessageReceived(BMessage *msg);

				
	private:
		BView					*fView;
		BButton					*fSave;
		BButton					*fRevert;
		BOutlineListView		*fListView;
		BBox					*fBox;

		view_map				fPrefViews;		
		SettingView				*fCurrentView;
		int32					fCurrentIndex;

		float					fFontHeight;
};

#endif
