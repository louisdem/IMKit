#ifndef IM_SERVER_H
#define IM_SERVER_H

#include <list>
#include <map>
#include <string>
#include <Application.h>
#include <Messenger.h>
#include <Query.h>

#include "../common/IMKitUtilities.h"

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
		
		status_t	GetSettings(const char * protocol, BMessage*);
		status_t	SetSettings(const char * protocol, BMessage*);
		BMessage	GenerateSettingsTemplate();
		status_t	UpdateOwnSettings( BMessage );
		
		void	handleDeskbarMessage( BMessage * );
		
		void	MessageToProtocols(BMessage*);
		void	MessageFromProtocols(BMessage*);
		
		void	UpdateStatus(BMessage*,Contact &);
		void	SetAllOffline();
		void	handle_STATUS_SET( BMessage * );
		
		void	GetContactsForProtocol( const char * protocol, BMessage * msg );
		
		void	StartAutostartApps();
		void	StopAutostartApps();
		
		void	reply_ADD_AUTOSTART_APPSIG( BMessage * );
		void	reply_REMOVE_AUTOSTART_APPSIG( BMessage * );
		void	reply_GET_SETTINGS_TEMPLATE(BMessage*);
		void	reply_GET_LOADED_PROTOCOLS(BMessage*);
		void	reply_SERVER_BASED_CONTACT_LIST(BMessage*);
		void	reply_GET_SETTINGS( BMessage * );
		void	reply_SET_SETTINGS( BMessage * );
		
		BBitmap	*GetBitmap(const char *name, type_code type = 'BBMP');
		BBitmap *GetBitmapFromAttribute(const char *name, const char *attribute, 
			type_code type = 'BBMP');
		
		string	FindBestProtocol( Contact & contact );
		
		BQuery						fQuery;
		list<BMessenger>			fMessengers;
		map<string,Protocol*>		fProtocols;
		map<Protocol*,AddOnInfo>	fAddOnInfo;
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
