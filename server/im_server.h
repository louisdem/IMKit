#ifndef IM_SERVER_H
#define IM_SERVER_H

#include <list>
#include <map>
#include <string>
#include <Application.h>
#include <Messenger.h>
#include <Query.h>

#include <libim/Contact.h>
#include <libim/Protocol.h>
#include "AddOnInfo.h"

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
		
		Contact	FindContact( const char * proto_id );
		Contact	CreateContact( const char * proto_id );
		
		void	LoadAddons();
		void	UnloadAddons();
		
		void	Process( BMessage * );
		void	Broadcast( BMessage * );
		
		void	AddEndpoint( BMessenger );
		void	RemoveEndpoint( BMessenger );
		
		void	GetSettingsTemplate(BMessage*);
		void	GetLoadedProtocols(BMessage*);
		void	ServerBasedContactList(BMessage*);
		void	GetSettings(BMessage*);
		void	SetSettings(BMessage*);
		
		BMessage	GenerateSettingsTemplate();
		status_t	UpdateOwnSettings( BMessage );
		
		void	MessageToProtocols(BMessage*);
		void	MessageFromProtocols(BMessage*);
		
		void	UpdateStatus(BMessage*,Contact &);
		void	SetAllOffline();
		
		void	GetContactsForProtocol( const char * protocol, BMessage * msg );
		
		BQuery						fQuery;
		list<BMessenger>			fMessengers;
		map<string,Protocol*>		fProtocols;
		map<Protocol*,AddOnInfo>	fAddOnInfo;
		map<string,string>			fStatus;// proto_id_string,status_string
		
		map<node_ref,list<pair<string,string> > >	contacts_protocols_ids;
};

};

#endif
