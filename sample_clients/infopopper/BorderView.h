#ifndef BORDER_VIEW_H
#define BORDER_VIEW_H

#include <View.h>
#include <String.h>

class BorderView : public BView
{
	public:
		BorderView( BRect, const char * );
		virtual ~BorderView();
	
		virtual void Draw( BRect );
		
		virtual void GetPreferredSize( float *, float * );
		
	private:
		BString	fTitle;
};

#endif
