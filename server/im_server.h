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
		
		void	reply_GET_SETTINGS_TEMPLATE(BMessage*);
		void	reply_GET_LOADED_PROTOCOLS(BMessage*);
		void	reply_SERVER_BASED_CONTACT_LIST(BMessage*);
		status_t	GetSettings(const char * protocol, BMessage*);
		status_t	SetSettings(const char * protocol, BMessage*);
		void	reply_GET_SETTINGS( BMessage * );
		void	reply_SET_SETTINGS( BMessage * );
		
		BMessage	GenerateSettingsTemplate();
		status_t	UpdateOwnSettings( BMessage );
		
		void	MessageToProtocols(BMessage*);
		void	MessageFromProtocols(BMessage*);
		
		void	UpdateStatus(BMessage*,Contact &);
		void	SetAllOffline();
		
		void	GetContactsForProtocol( const char * protocol, BMessage * msg );
		
		void	StartAutostartApps();
		void	StopAutostartApps();
		void	reply_ADD_AUTOSTART_APPSIG( BMessage * );
		void	reply_REMOVE_AUTOSTART_APPSIG( BMessage * );
		
		BBitmap	*GetBitmap(const char *name, type_code type = 'BBMP');
		BBitmap *GetBitmapFromAttribute(const char *name, const char *attribute, 
			type_code type = 'BBMP');
		
		BQuery						fQuery;
		list<BMessenger>			fMessengers;
		map<string,Protocol*>		fProtocols;
		map<Protocol*,AddOnInfo>	fAddOnInfo;
		map<string,string>			fStatus;// proto_id_string,status_string
		
		map<node_ref,list<pair<string,string> > >	contacts_protocols_ids;

		BMessage					fIcons;
};

};

#define LOG_PREFIX "im_server"
#define BEOS_SMALL_ICON "BEOS:M:STD_ICON"
#define BEOS_LARGE_ICON "BEOS:L:STD_ICON"

#endif
