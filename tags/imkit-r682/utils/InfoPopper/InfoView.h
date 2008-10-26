#ifndef INFO_VIEW_H
#define INFO_VIEW_H

#include <View.h>
#include <MessageRunner.h>
#include <TextView.h>
#include <String.h>
#include <list>

#include <Bitmap.h>
#include <storage/Entry.h>
#include <Roster.h>
#include <Path.h>
#include <Message.h>

#include <libim/InfoPopper.h>

class InfoWindow;

enum {
	REMOVE_VIEW = 'ReVi'
};

enum infoview_layout {
	TitleAboveIcon = 0,
	AllTextRightOfIcon = 1
};


typedef struct line_struct {
	BFont font;
	BString text;
	BPoint location;
} lineinfo;

typedef list<lineinfo *> vline;

// -------------- INFO VIEW -----------------

using namespace InfoPopper;

class InfoView : public BView {
	public:				
							InfoView(InfoWindow *win, info_type type,
								const char *app, const char *title, 
								const char *text, BMessage *details);
							~InfoView(void);
		
		// Hooks
		void 				AttachedToWindow(void);
		void 				MessageReceived(BMessage *msg);
		void 				GetPreferredSize(float *width, float *height);
		void				Draw(BRect bounds);
		void				MouseDown(BPoint point);
		void				FrameResized(float width, float height);
		// Scripting Hooks	
		BHandler			*ResolveSpecifier(BMessage *msg, int32 index,
								BMessage *spec, int32 form, const char *prop);
		status_t			GetSupportedSuites(BMessage *msg);

		
		/**
			Set the text to be displayed. Called by the constructor.
		*/
		void 				SetText(const char *app, const char *title,
								const char *text, float newMaxWidth = -1);
		bool				HasMessageID(const char *id);
		const char			*MessageID(void);
		void				SetPosition(bool first, bool last);

		
	private:
		BBitmap			*ExtractIcon(const char *prefix, BMessage *msg, int16 size,
							icon_type &type);
		InfoWindow		*fParent;
	
		info_type		 fType;
		BMessageRunner	*fRunner;
		float			fProgress;
		BString			fMessageID;
		
		BMessage		*fDetails;
		BBitmap			*fBitmap;
		BBitmap			*fOverlayBitmap;
		icon_type		fIconType;
		icon_type		fOverlayType;
		
		vline			fLines;

		BString			fApp;
		BString			fTitle;
		BString			fText;
		
		float			fHeight;
		
		bool			fIsFirst;
		bool			fIsLast;
};

#endif
