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
	STATUS_SET			= 15,  // own status was altered
	AUTH_REQUEST		= 16,   // auth request
	SEND_AUTH_ACK		= 17,	// auth request reply
	CONTACT_AUTHORIZED	= 18,	// trigger contact creation on auth accepted
	REQUEST_AUTH		= 19,
	UNREGISTER_CONTACTS = 20
};

/**
	what-codes for messages sent to and from the im_server
*/
enum message_what_codes {
	/*
		Used for all error messages
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
//	GET_SETTINGS		= 'IMgs',
//	SETTINGS			= 'IMse',
//	SET_SETTINGS		= 'IMss',
//	GET_SETTINGS_TEMPLATE	= 'IMgt',
	SETTINGS_TEMPLATE	= 'IMst',
	SETTINGS_UPDATED	= 'IMs0', // settings updated, notify protocol/client/etc
	
	/*
		Information about protocols
	*/
	GET_LOADED_PROTOCOLS	= 'IMgp',
	SERVER_BASED_CONTACT_LIST	= 'IMsl',
	
	/*
		Contact related messages
	*/
	GET_CONTACT_STATUS		='IMc0',
	GET_OWN_STATUSES		='IMc1',
	UPDATE_CONTACT_STATUS	= 'IMc2',
	GET_CONTACTS_FOR_PROTOCOL = 'IMc3',
	
	/*
		Deskbar icon related messages
	*/
	DESKBAR_ICON_CLICKED	= 'DBcl',
	REGISTER_DESKBAR_MESSENGER = 'DBrg',
	FLASH_DESKBAR			= 'DBfl',
	STOP_FLASHING			= 'DBst',
	
	/*
		Client autostart management
	*/
	ADD_AUTOSTART_APPSIG	= 'Aaas',
	REMOVE_AUTOSTART_APPSIG	= 'Raas'
};

#define IM_SERVER_SIG "application/x-vnd.beclan.im_kit"

// Valid IM:status texts
#define ONLINE_TEXT		"Available"
#define AWAY_TEXT		"Away"
#define OFFLINE_TEXT	"Offline"
#define BLOCKED_TEXT	"Blocked"

};

#endif
