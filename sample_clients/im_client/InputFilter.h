#ifndef INPUTFILTER_H
#define INPUTFILTER_H

#include <MessageFilter.h>
#include <Message.h>
#include <TextView.h>
#include <Window.h>

class BMessageFilter;

class InputFilter : public BMessageFilter {
	public:
								InputFilter(BTextView *owner, BMessage *msg);
	    virtual filter_result	Filter (BMessage *, BHandler **);
    	filter_result			HandleKeys (BMessage *);
    
	private:
		BTextView	 			*fParent;
		BMessage				*fMessage;
};

#endif
