#ifndef APPVIEW_H
#define APPVIEW_H

#include <View.h>

class AppUsage;
class BCheckBox;
class BColumnListView;
class BDateColumn;
class BStringColumn;

class AppView : public BView {
	public:
							AppView(BRect rect, AppUsage *usage);
							~AppView(void);

		// BView Hooks
		void				AttachedToWindow(void);
		void				MessageReceived(BMessage *msg);
		
		// Public
		void				Populate(void);
		AppUsage			*Value(void);
	
	private:
		AppUsage			*fUsage;
	
		BCheckBox			*fBlockAll;
		BColumnListView		*fNotifications;
		BStringColumn		*fTitleCol;
		BDateColumn			*fDateCol;
		BStringColumn		*fTypeCol;
		BStringColumn		*fAllowCol;
	
};

#endif
