#ifndef IMUTILITIES_H
#define IMUTILITIES_H

#include <Bitmap.h>
#include <Node.h>
#include <be/kernel/fs_attr.h>
#include <Path.h>
#include <Entry.h>

#include <stdlib.h>

#define BEOS_SMALL_ICON_ATTRIBUTE 	"BEOS:M:STD_ICON"
#define BEOS_LARGE_ICON_ATTRIBUTE	"BEOS:L:STD_ICON"

extern const int32 kSmallIcon;
extern const int32 kLargeIcon;

BBitmap *GetTrackerIcon(BNode &, unsigned long, long *);

BBitmap *GetBitmapFromAttribute(const char *name, const char *attribute,
	type_code type = 'BBMP', bool followSymlink = true);

char *ReadAttribute(BNode node, const char *attribute, int32 *length = NULL);
status_t WriteAttribute(BNode node, const char *attribute, const char *value,
	size_t length, type_code type);


// This will return the standard icon for R5 and the SVG icon for Zeta
BBitmap *ReadNodeIcon(const char *name, int32 size = kSmallIcon,
	bool followSymlink = true);

#endif
