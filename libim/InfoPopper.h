#ifndef INFOPOPPER_H
#define INFOPOPPER_H

namespace InfoPopper {
	
	// Message types you can send
	enum info_type {
		Information,	// Normal message
		Important,		// Important message - more visible
		Error,			// Error message (displays in red)
		Progress		// Progress bar - allows updating
	};
	
	enum info_messages {
		AddMessage = 'ipAM',	// Adds a message to the InfoPopper
	};

// AddMessage
//  int8 type: One of the info_type constants - changes appearance
//  String title: The name of the origin of the message, for example
//  String content: The message to be displayed
//  BMessage icon: An archived BBitmap to display
//  String onClickApp: MIME string of Application to launch when clicked
//    - or -
//  entry_ref onClickFile: File to launch when clicked
//  String onClickArgv: A string to be passed to the application as an argv (may be multiples)
//  entry_ref onClickRef: Entry ref to be passed to launched (may be multiples)


//	const char *kInfoPopperSig = "application/x-vnd.beclan.IM_InfoPopper";
#define InfoPopperAppSig "application/x-vnd.beclan.InfoPopper"

}

#endif
