#ifndef INFO_VIEW_H
#define INFO_VIEW_H

#include <View.h>
#include <MessageRunner.h>
#include <TextView.h>
#include <String.h>
#include <slist>

#include <Bitmap.h>
#include <storage/Entry.h>
#include <Roster.h>
#include <Path.h>
#include <Message.h>

#include <libim/InfoPopper.h>

enum {
	REMOVE_VIEW = 'ReVi'
};

typedef struct line_struct {
	BFont font;
	BString text;
	BPoint location;
} lineinfo;

typedef slist<lineinfo *> vline;

// -------------- INFO VIEW -----------------

using namespace InfoPopper;

class InfoView : public BView
{
	public:		
		enum infoview_layout {
			TitleAboveIcon,
			AllTextRightOfIcon
		};
		
		InfoView(info_type type, const char *app, const char *title,
			const char *text, BMessage *details);
		~InfoView();
		
		void AttachedToWindow();
		void MessageReceived( BMessage * );
		
		void GetPreferredSize( float *, float * );
		
		void Draw( BRect );
		
		/**
			Set the text to be displayed. Called by the constructor.
			
			The first line of the text is set to use be_bold_font, and the
			rest of the lines are set to use be_plain_font.
		*/
		void SetText(const char *app, const char *title, const char *text, float newMaxWidth=-1);
		
		bool HasMessageID( const char * );

		void MouseDown(BPoint point);
		
		void FrameResized(float,float);
		
		BHandler * ResolveSpecifier(BMessage *, int32 , BMessage *, int32, const char *);
		status_t GetSupportedSuites(BMessage*);
		
	private:
		info_type		 fType;
		BMessageRunner	* fRunner;
		float			fProgress;
		BString			fMessageID;
		int32			fTimeout;
		
		BMessage		*fDetails;
		BBitmap			*fBitmap;
		
		vline			fLines;

		BString			fApp;
		BString			fTitle;
		BString			fText;
		
		float			fHeight;
};

#endif
