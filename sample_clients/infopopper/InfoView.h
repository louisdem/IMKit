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
			Error
		};
		
		InfoView( info_type, const char * );
		~InfoView();
		
		void AttachedToWindow();
		void MessageReceived( BMessage * );
		
		void GetPreferredSize( float *, float * );
		
		void Draw( BRect );
		
		void SetText( const char * );
		
	private:
		BMessageRunner	* fRunner;
		InputFilter		* fFilter;
//		BTextView 		* fView;
		list<BString>	fLines;
};


#endif
