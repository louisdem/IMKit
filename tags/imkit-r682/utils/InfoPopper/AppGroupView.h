#ifndef APPGROUPVIEW_H
#define APPGROUPVIEW_H

#include <String.h>
#include <View.h>

#include <vector>

class InfoWindow;
class InfoView;

typedef vector<InfoView *> infoview_t;

class AppGroupView : public BView {
	public:
							AppGroupView(InfoWindow *win, const char *label);
							~AppGroupView(void);
	
		// Hooks
		void				AttachedToWindow(void);
		void				Draw(BRect bounds);
		void				MouseDown(BPoint point);
		void				GetPreferredSize(float *width, float *height);
		void				MessageReceived(BMessage *msg);
 	
		// Public
		void				AddInfo(InfoView *view);
		void				ResizeViews(void);
		bool				HasChildren(void);
	
	private:
		BString				fLabel;
		InfoWindow			*fParent;
		infoview_t			fInfo;
		bool				fCollapsed;
		BRect				fCloseRect;
		BRect				fCollapseRect;
};

#endif
