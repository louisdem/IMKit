#ifndef IM_PROTOCOL_H
#define IM_PROTOCOL_H

#include <OS.h>

namespace IM {

class Protocol
{
	public:
		Protocol();
		virtual ~Protocol();
		
		/**
			Called after loading the add-on. Should start connection
			to IM service and return B_OK if all's well.
		*/
		virtual status_t Init( BMessenger )=0;
		
		/**
			Called before shutting down, should disconnect from the
			IM service in a clean way and return B_OK
		*/
		virtual status_t Shutdown()=0;
		
		/**
			Handle a message from the im_server, see im_server protocol
			specification for more info on messages and their contents.
		*/
		virtual status_t Process( BMessage * )=0;
		
		/**
			Return the signature of the protocol, "ICQ" for ICQ etc.
		*/
		virtual const char * GetSignature()=0;
		
		/**
			Return a BMessage containing the various settings that can
			be made to affect the service. See im_server specification
			for more info.
		*/
		virtual BMessage GetSettingsTemplate()=0;
		
		/**
			Settings have been changed, update internal settings to match
			those in the message. Also called before the initial Init() call 
			so the protocols don't have to worry about saving settings, the
			im_server takes care of that.
			Return B_OK if the message is valid, B_ERROR if it is not. An
			invalid message is not saved by the im_server.
		*/
		virtual status_t UpdateSettings( BMessage & )=0;
		
		/**
			Get the preferred charset encoding. The im_server will make sure
			that any "message" fields in messages that are sent to the
			protocol for processing are converted to this encoding. To get
			messages in UTF-8 return 0xffff, otherwise return the desired
			B_xxx_CONVERSION constant as defined in <UTF8.h>
		*/
		virtual uint32 GetEncoding()=0;
};

};

// extern IM::Protocol * load_protocol();

#endif
