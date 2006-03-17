#ifndef INFOPOPPER_H
#define INFOPOPPER_H

namespace InfoPopper {
	
	// Message types you can send
	enum info_type {
		Information = 0,	// Normal message
		Important = 1,		// Important message - more visible
		Error = 2,			// Error message (displays in red)
		Progress = 3,		// Progress bar - allows updating
	};
	
	enum info_messages {
		AddMessage = 'ipAM',	// Adds a message to the InfoPopper
		GetIconSize = 'ipGS',	// Fetch the current user preferred icon size
								// Sends back int16 iconSize
	};
	
	enum icon_type {
		Attribute,				// Loads the icon from the entry_ref's icon
		Contents,				// Loads the icon from the content of the entry_ref
		Picture,				// Creates the icon from a BPicture object
		Bitmap					// The icon is a premade BBitmap
	};
	
// AddMessage
///// Message data - required!
//  int8 type: One of the info_type constants - changes appearance
//  String app: The main title of the message (Eg. "The IM Kit")
//  String title: The name of the origin of the message, for example "ICQ"
//  String content: The message to be displayed
//// Optional message specifiers
//  String messageID: optional. When you add a new message with the
//					  same messageID as an existing message, that
//					  message is replaced with the new one
//	Float progress: optional. value between 0.0 and 1.0 describing progress.
// 					Requires that type is Progress to be displayed.
//	Int32 timeout: optional. Number of seconds the message should be displayed,
//					If 0, the message will stay until clicked. Default 5.
////// Icon
//  BMessage icon: An archived BBitmap to display
//    - or -
//	entry_ref	iconRef: entry_ref to icon file
//  int32		iconType: where to look for icon, either Attribute or Contents.
////// Overlay Icon
//	BMessage overlayIcon: An archived BBitmap to display
//	  - or -
//	entry_ref	overlayIconRef: entry_ref to icon file
//	int32		overlayIconType: where to look for icon, Attribute or Contents
////// Action on click
//  String onClickApp: MIME string of Application to launch when clicked
//    - or -
//  entry_ref onClickFile: File to launch when clicked
//  String onClickArgv: A string to be passed to the application as an argv (may be multiples)
//  entry_ref onClickRef: Entry ref to be passed to launched (may be multiples)
//  BMessage onClickMsg: Message to be sent on click (may be multiples)

#define InfoPopperAppSig "application/x-vnd.beclan.InfoPopper"

}

#endif
