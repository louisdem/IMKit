#ifndef IM_CONSTANTS_H
#define IM_CONSTANTS_H

namespace IM {

/**
	Defines different IM messages
*/
enum im_what_code {
	GET_CONTACT_LIST	= 1,
	SEND_MESSAGE		= 2,
	MESSAGE_SENT		= 3,
	MESSAGE_RECEIVED	= 4,
	STATUS_CHANGED		= 5,
	CONTACT_LIST		= 6,
	SET_STATUS			= 7, // set own status
	GET_CONTACT_INFO	= 8,
	CONTACT_INFO		= 9,
	REGISTER_CONTACTS	= 10, // provide a list of contacts we're interested in to the protocol
	CONTACT_STARTED_TYPING	= 11,
	CONTACT_STOPPED_TYPING	= 12,
	USER_STARTED_TYPING		= 13,
	USER_STOPPED_TYPING		= 14,
	STATUS_SET			= 15  // own status was altered
};

/**
	what-codes for messages sent to and from the im_server
*/
enum message_what_codes {
	/*
		USed for all error messages
	*/
	ERROR				= 'IMer',
	/*
		Returned after a request has succeded
	*/
	ACTION_PERFORMED	= 'IMok',
	/*
		All client <> protocol communication uses the MESSAGE what-code.
	*/
	MESSAGE				= 'IMme', 
	/*
		Endpoint management
	*/
	ADD_ENDPOINT		= 'IMae',
	REMOVE_ENDPOINT		= 'IMre',
	/*
		Settings management
	*/
	GET_SETTINGS		= 'IMgs',
	SETTINGS			= 'IMse',
	SET_SETTINGS		= 'IMss',
	GET_SETTINGS_TEMPLATE	= 'IMgt',
	SETTINGS_TEMPLATE	= 'IMst',
	
	/*
		Information about protocols
	*/
	GET_LOADED_PROTOCOLS	= 'IMgp',
	SERVER_BASED_CONTACT_LIST	= 'IMsl',
	
	/*
		Deskbar icon related messages
	*/
	DESKBAR_ICON_CLICKED	= 'DBcl'
};

#define IM_SERVER_SIG "application/x-vnd.beclan.im_kit"

// Valid IM:status texts
#define ONLINE_TEXT		"available"
#define AWAY_TEXT		"away"
#define OFFLINE_TEXT	"offline"

};

#endif
