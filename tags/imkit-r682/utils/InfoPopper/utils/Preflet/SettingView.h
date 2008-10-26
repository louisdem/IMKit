#ifndef SETTINGVIEW_H
#define SETTINGVIEW_H

#include <View.h>

class SettingView : public BView {
	public:
							SettingView(BRect frame, const char *name,
								uint32 resizing, uint32 flags)
								: BView(frame, name, resizing, flags) { };
	
		virtual status_t	Save(void) = 0;
		virtual status_t	Load(void) = 0;
};

#endif
