#ifndef INFO_VIEW_H
#define INFO_VIEW_H

#include <View.h>
#include <MessageRunner.h>
#include <TextView.h>
#include "InputFilter.h"
#include <String.h>
#include <list>

enum {
	REMOVE_VIEW = 'ReVi'
};

// -------------- INFO VIEW -----------------

class InfoView : public BView
{
	public:
		enum info_type {
			Information,
			Important,
			Error,
			Progress
		};
		
		InfoView( info_type, const char * text, const char * progID = NULL, float prog = 0.0 );
		~InfoView();
		
		void AttachedToWindow();
		void MessageReceived( BMessage * );
		
		void GetPreferredSize( float *, float * );
		
		void Draw( BRect );
		
		void SetText( const char * );
		
		bool HasProgressID( const char * );
		
	private:
		info_type		fType;
		BMessageRunner	* fRunner;
		InputFilter		* fFilter;
//		BTextView 		* fView;
		list<BString>	fLines;
		float			fProgress;
		BString			fProgressID;
};


#endif
