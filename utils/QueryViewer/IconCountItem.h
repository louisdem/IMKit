#ifndef ICONCOUNTITEM_H
#define ICONCOUNTITEM_H

#include <Bitmap.h>
#include <ListItem.h>
#include <String.h>
#include <View.h>

class IconCountItem : public BListItem {
	public:
	
						IconCountItem(const char *text, BBitmap *icon = NULL);
						~IconCountItem(void);
						
		virtual void	DrawItem(BView *owner, BRect frame, bool complete);
		virtual void	Update(BView *owner, const BFont *font);
		const char 		*Text(void) const;
		const BBitmap	*Icon(void) const;
		
				int32	GetCount(void);
				void	SetCount(int32 count);
		

	private:
		BBitmap			*fIcon;
		BString			fText;
		float			fFontHeight;
		float			fIconHeight;
		float			fIconWidth;
		float			fFontOffset;
		
		int32			fCount;
};

#endif
