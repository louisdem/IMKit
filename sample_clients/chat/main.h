#ifndef MAIN_H
#define MAIN_H

#include <libim/Manager.h>

#include <Application.h>
#include <Window.h>
#include <Messenger.h>
#include <Entry.h>
#include <TextControl.h>
#include <TextView.h>
#include <Rect.h>
#include <BeBuild.h>
#include <Window.h>
#include <Beep.h>

#include <libim/Constants.h>
#include <libim/Contact.h>
#include <libim/Helpers.h>


void
setAttributeIfNotPresent( entry_ref ref, const char * attr, const char * value);

#include "ChatApp.h"
#include "ChatWindow.h"
#include "InputFilter.h"
#include "ResizeView.h"
#include "URLTextView.h"

const uint32 kResizeMessage = 'irsz';

#endif
