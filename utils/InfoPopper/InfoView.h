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

#include <libim/InfoPopper.h>

enum {
	REMOVE_VIEW = 'ReVi'
};

// -------------- INFO VIEW -----------------

using namespace InfoPopper;

class InfoView : public BView
{
	public:		
		InfoView( info_type, const char * text, BMessage *details );
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
		void SetText( const char * );
		
		bool HasMessageID( const char * );

		void MouseDown(BPoint point);
		
	private:
		info_type		 fType;
		BMessageRunner	* fRunner;
		list<pair<BString,const BFont*> >	fLines;
		float			fProgress;
		BString			fMessageID;
		int32			fTimeout;
		
		BMessage		*fDetails;
		BBitmap			*fBitmap;
};


#endif
