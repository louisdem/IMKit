#ifndef INPUTFILTER_H
#define INPUTFILTER_H

#include <MessageFilter.h>
#include <Message.h>
#include <View.h>

class BMessageFilter;

class InputFilter : public BMessageFilter {
	public:
								InputFilter(BView *owner);
	    virtual filter_result	Filter (BMessage *, BHandler **);
    
	private:
		BView		 			*fOwner;
};

#endif
