#ifndef IM_SERVER_H
#define IM_SERVER_H

#include <list>
#include <map>
#include <string>
#include <Application.h>
#include <Messenger.h>
#include <Query.h>
#include <Entry.h>
#include <Node.h>
#include <String.h>

#include "../common/IMKitUtilities.h"

#include <libim/Contact.h>
#include <libim/Protocol.h>
#include "AddOnInfo.h"

/**
	Used by Contact monitor.
*/
class ContactHandle
{
	public:
		ino_t node;
		entry_ref entry;
		
		ContactHandle()
		{
		}
		
		ContactHandle( const ContactHandle & c )
		:	node( c.node )
		{
			entry.directory = c.entry.directory;
			entry.device = c.entry.device;
			entry.set_name( c.entry.name );
		}
		
		bool operator < ( const ContactHandle & c ) const
		{
			if ( entry.device != c.entry.device )
				return entry.device < c.entry.device;
			
			return node < c.node;
		}
		
		bool operator == ( const ContactHandle & c ) const
		{
/*			printf("%Ld, %ld vs %Ld, %ld\n", 
				node, entry.device, 
				c.node, c.entry.device
			);
*/			
			return node == c.node && entry.device == c.entry.device;
		}
};

namespace IM {

class Server : public BApplication
{
	public:
		Server();
		virtual ~Server();
		
		virtual bool QuitRequested();
		
		virtual void MessageReceived( BMessage * );
		
	private:
		void	StartQuery();
		void	HandleContactUpdate( BMessage * );
		
		Contact			FindContact( const char * proto_id );
		list<Contact>	FindAllContacts( const char * proto_id );
		Contact			CreateContact( const char * proto_id , const char *namebase );
		
		void		RegisterSoundEvents();
		status_t	LoadAddons();
		void		UnloadAddons();
		
		void	Process( BMessage * );
		void	Broadcast( BMessage * );
		
		void	AddEndpoint( BMessenger );
		void	RemoveEndpoint( BMessenger );
		
		status_t	GetSettings(const char * protocol, BMessage*);
		status_t	SetSettings(const char * protocol, BMessage*);
		BMessage	GenerateSettingsTemplate();
		status_t	UpdateOwnSettings( BMessage );
		void		InitSettings();
		
		void	handleDeskbarMessage( BMessage * );
		
		void	MessageToProtocols(BMessage*);
		void	MessageFromProtocols(BMessage*);
		
		void	UpdateStatus(BMessage*);
		void	SetAllOffline();
		void	handle_STATUS_SET( BMessage * );
		void	UpdateContactStatusAttribute( Contact & );
		
		void	GetContactsForProtocol( const char * protocol, BMessage * msg );
		
		void	StartAutostartApps();
		void	StopAutostartApps();
		
		void	reply_GET_LOADED_PROTOCOLS(BMessage*);
		void	reply_SERVER_BASED_CONTACT_LIST(BMessage*);
		void	reply_GET_CONTACT_STATUS( BMessage * );
		void	reply_UPDATE_CONTACT_STATUS( BMessage * );
		void	reply_GET_OWN_STATUSES(BMessage *msg);
		void	reply_GET_CONTACTS_FOR_PROTOCOL( BMessage * );
		
		void	handle_SETTINGS_UPDATED(BMessage *);
		
		string	FindBestProtocol( Contact & contact );
		
		/**
			Contact monitoring functions
		*/
		void ContactMonitor_Added( ContactHandle );
		void ContactMonitor_Modified( ContactHandle );
		void ContactMonitor_Moved( ContactHandle from, ContactHandle to );
		void ContactMonitor_Removed( ContactHandle );
		
		// Variables
		
		list<BQuery*>				fQueries;
		list<BMessenger>			fMessengers;
		map<string,Protocol*>		fProtocols;
		map<Protocol*,AddOnInfo>	fAddOnInfo;
		/**
			entry_ref, list of connections.
			Used to store connections for contacts, so we can notify the protocols
			of any changes.
		*/
		//list<ContactHandle, list<string> >	fContacts;
		list< pair<ContactHandle, list<string>* > > fContacts;
		
		/*	Used to store both <protocol>:<id> and <protocol> status.
			In other words, both own status per protocol and contact
			status per connection */
		map<string,string>			fStatus;// proto_id_string,status_string
		
		map<Contact,string>			fPreferredProtocol;
		
		BMessage					fIcons;
		
		BMessenger					fDeskbarMsgr;
};

};

#define LOG_PREFIX "im_server"

#endif
