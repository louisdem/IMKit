#ifndef IM_SAMPLE_CLIENT_H
#define IM_SAMPLE_CLIENT_H

#include <libim/Manager.h>

#include <TextView.h>
#include <BeBuild.h>

class URLTextView : public BTextView
{
	public:
		URLTextView( BRect r, const char * name, BRect text_rect, uint32 follow, uint32 flags );
		virtual ~URLTextView();
		
		virtual void MouseUp( BPoint );
		
		virtual void MakeFocus( bool );
#if B_BEOS_VERSION > B_BEOS_VERSION_5
		virtual status_t UISettingsChanged(const BMessage *changes, uint32 flags);
#endif

};

#endif
