#ifndef IMUTILITIES_H
#define IMUTILITIES_H

#include <Bitmap.h>
#include <Node.h>
#include <be/kernel/fs_attr.h>

#include <stdlib.h>

#define BEOS_SMALL_ICON_ATTRIBUTE 	"BEOS:M:STD_ICON"
#define BEOS_LARGE_ICON_ATTRIBUTE	"BEOS:L:STD_ICON"

BBitmap *GetBitmapFromAttribute(const char *name, const char *attribute,
	type_code type = 'BBMP');

char *ReadAttribute(BNode node, const char *attribute);

#endif
