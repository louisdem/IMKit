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

class InfoView : public BView
{
	public:				
		InfoView(InfoWindow *win, info_type type, const char *app, const char *title, 
			const char *text, BMessage *details);
		~InfoView();
		
		void AttachedToWindow();
		void MessageReceived( BMessage * );
		
		void GetPreferredSize( float *, float * );
		
		void Draw( BRect );
		
		/**
			Set the text to be displayed. Called by the constructor.
		*/
		void SetText(const char *app, const char *title, const char *text, float newMaxWidth=-1);
		
		bool HasMessageID( const char * );

		void MouseDown(BPoint point);
		
		void FrameResized(float,float);
		
		BHandler * ResolveSpecifier(BMessage *, int32 , BMessage *, int32, const char *);
		status_t GetSupportedSuites(BMessage*);
		
	private:
		BBitmap			*ExtractIcon(const char *prefix, BMessage *msg, int16 size);
		InfoWindow		*fParent;
	
		info_type		 fType;
		BMessageRunner	*fRunner;
		float			fProgress;
		BString			fMessageID;
		
		BMessage		*fDetails;
		BBitmap			*fBitmap;
		BBitmap			*fOverlayBitmap;
		
		vline			fLines;

		BString			fApp;
		BString			fTitle;
		BString			fText;
		
		float			fHeight;
};

#endif
