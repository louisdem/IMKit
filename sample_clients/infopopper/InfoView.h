#ifndef INFO_VIEW_H
#define INFO_VIEW_H

#include <View.h>
#include <MessageRunner.h>

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
		
	private:
		BMessageRunner * fRunner;
};


#endif
