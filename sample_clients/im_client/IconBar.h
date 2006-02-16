#ifndef ICONBAR_H
#define ICONBAR_H

#include <List.h>
#include <View.h>

class IconBar : public BView {
	public:
						IconBar(BRect rect);
		virtual			~IconBar();
		
		virtual void 	MessageReceived(BMessage *msg);

				int32	AddItem(BView *view);
				BView	*ViewAt(int32 index);
			
		virtual void	Draw(BRect rect);
		
				void	PositionViews();
	private:
				BList	fViews;
};

#endif
