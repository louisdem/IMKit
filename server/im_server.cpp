#include "im_server.h"

#include <libim/Constants.h>
#include <libim/Helpers.h>
#include "DeskbarIcon.h"
#include <TranslationUtils.h>

#include <image.h>
#include <Roster.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <stdio.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <VolumeRoster.h>
#include <Volume.h>
#include <UTF8.h>
#include <libim/Helpers.h>
#include <Deskbar.h>
#include <Roster.h>
#include <be/kernel/fs_attr.h>
#include <Alert.h>
#include <String.h>
#include <Invoker.h>
#include <algorithm>

using namespace IM;

#define AUTOSTART_APPSIG_SETTING "autostart_appsig"

void
_ERROR( const char * error, BMessage * msg )
{
	LOG("im_server", liHigh, error, msg);
}

void
_ERROR( const char * error )
{
	_ERROR(error,NULL);
}

void
_SEND_ERROR( const char * text, BMessage * msg )
{
	if ( msg->ReturnAddress().IsValid() )
	{
		BMessage err(ERROR);
		err.AddString("error",text);
		msg->SendReply(&err);
	} else
	{ // no recipient for message replies, write to stdout
		LOG("im_server", liHigh, "ERROR: %s",text);
	}
}

/**
	Default constructor. (Starts People-query and ?) loads add-ons
*/
Server::Server()
:	BApplication(IM_SERVER_SIG)
{
	LOG("im_server", liHigh, "Starting im_server");
	
	BPath prefsPath;
	
	// Create our settings directories
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&prefsPath,true,NULL) == B_OK)
	{
		BDirectory dir(prefsPath.Path());
		if (dir.InitCheck() == B_OK)
		{
			dir.CreateDirectory("im_kit", NULL);
			dir.CreateDirectory("im_kit/icons", NULL);
			dir.CreateDirectory("im_kit/add-ons", NULL);
			dir.CreateDirectory("im_kit/add-ons/protocols", NULL);
			dir.CreateDirectory("im_kit/clients", NULL);
		}
	}

	InitSettings();
	
	LoadAddons();
	
	// add deskbar icon
	BDeskbar db;

	if ( db.RemoveItem( DESKBAR_ICON_NAME ) != B_OK )
		LOG("im_server", liDebug, "Error removing deskbar icon (this is ok..)");
	
	IM_DeskbarIcon * i = new IM_DeskbarIcon();
	
	if ( db.AddItem( i ) != B_OK )
	{ // couldn't add BView, try entry_ref
		entry_ref ref;
		
		if ( be_roster->FindApp(IM_SERVER_SIG,&ref) == B_OK )
		{
			BPath p(&ref);
			
			if ( db.AddItem( &ref ) != B_OK )
				LOG("im_server", liHigh, "Error adding icon to deskbar!");
		} else
		{
			LOG("im_server", liHigh, "be_roster->FindApp() failed");
		}
	}
	
	delete i;
	
	prefsPath.Append("im_kit/icons");
	
	// load icons for "change icon depending on state"
	BString iconPath = prefsPath.Path();
	iconPath << "/Online";
	
	fIcons.AddPointer(ONLINE_TEXT "_small", (const void *)GetBitmapFromAttribute(
		iconPath.String(), BEOS_SMALL_ICON_ATTRIBUTE));
	fIcons.AddPointer(ONLINE_TEXT "_large", (const void *)GetBitmapFromAttribute(
		iconPath.String(), BEOS_LARGE_ICON_ATTRIBUTE));
		
	iconPath = prefsPath.Path();
	iconPath << "/Away";
	fIcons.AddPointer(AWAY_TEXT "_small", (const void *)GetBitmapFromAttribute(
		iconPath.String(), BEOS_SMALL_ICON_ATTRIBUTE));
	fIcons.AddPointer(AWAY_TEXT "_large", (const void *)GetBitmapFromAttribute(
		iconPath.String(), BEOS_LARGE_ICON_ATTRIBUTE));

	iconPath = prefsPath.Path();
	iconPath << "/Offline";
	fIcons.AddPointer(OFFLINE_TEXT "_small", (const void *)GetBitmapFromAttribute(
		iconPath.String(), BEOS_SMALL_ICON_ATTRIBUTE));
	fIcons.AddPointer(OFFLINE_TEXT "_large", (const void *)GetBitmapFromAttribute(
		iconPath.String(), BEOS_LARGE_ICON_ATTRIBUTE));
	
	StartQuery();
	
	SetAllOffline();
	
	StartAutostartApps();
	
	Run();
}

/**
	Default destructor. Unloads add-ons and frees any aquired resources.
*/
Server::~Server()
{
	StopAutostartApps();

	UnloadAddons();
	
	SetAllOffline();

	BDeskbar db;
	db.RemoveItem( DESKBAR_ICON_NAME );
}

/**
*/
bool
Server::QuitRequested()
{
	return true;
}

/**
*/
void
Server::MessageReceived( BMessage * msg )
{
	switch ( msg->what )
	{
		// Contact messages. Both query and node monitor.
		case B_QUERY_UPDATE:
		{
			int32 opcode=0;
			
			msg->FindInt32("opcode", &opcode);
			
			ContactHandle handle;
			const char	* name;
			
			switch ( opcode )
			{
				case B_ENTRY_CREATED:
					// .. add to contact list
					msg->FindString("name", &name );
					handle.entry.set_name(name);
					msg->FindInt64("directory", &handle.entry.directory);
					msg->FindInt32("device", &handle.entry.device);
					msg->FindInt64("node", &handle.node);
					
					ContactMonitor_Added( handle );
					break;
				
				default:
					break;
			}
		}	break;
		
		case B_NODE_MONITOR:
			HandleContactUpdate(msg);
			break;
		
		// IM-Kit specified messages:
		
		case GET_CONTACT_STATUS:
			reply_GET_CONTACT_STATUS(msg);
			break;
		case GET_OWN_STATUSES: {
			reply_GET_OWN_STATUSES(msg);
		} break;
		case UPDATE_CONTACT_STATUS:
			reply_UPDATE_CONTACT_STATUS(msg);
			break;
		case GET_CONTACTS_FOR_PROTOCOL:
			reply_GET_CONTACTS_FOR_PROTOCOL(msg);
			break;
			
		case FLASH_DESKBAR:
		case STOP_FLASHING:
		case REGISTER_DESKBAR_MESSENGER:
			handleDeskbarMessage(msg);
			break;
		
		case SERVER_BASED_CONTACT_LIST:
			reply_SERVER_BASED_CONTACT_LIST(msg);
			break;
		
		case SETTINGS_UPDATED:
			handle_SETTINGS_UPDATED(msg);
			break;
		
		case ADD_ENDPOINT:
		{
			BMessenger msgr;
			if ( msg->FindMessenger("messenger",&msgr) == B_OK )
			{
				AddEndpoint( msgr );
				
				msg->SendReply( ACTION_PERFORMED );
			}
		}	break;
		
		case REMOVE_ENDPOINT:
		{
			BMessenger msgr;
			if ( msg->FindMessenger("messenger",&msgr) == B_OK )
			{
				RemoveEndpoint( msgr );
				
				msg->SendReply( ACTION_PERFORMED );
			}
		}	break;
		
		case GET_LOADED_PROTOCOLS:
			reply_GET_LOADED_PROTOCOLS(msg);
			break;
		
		case MESSAGE:
			Process(msg);
			break;
		
		// Other messages
		default:
			BApplication::MessageReceived(msg);
			break;
	}
}

/**
	Initializes Contact-query and fetches initial list of matches
*/
void
Server::StartQuery()
{
	BVolumeRoster 	vroster;
	BVolume			vol;
	char 			volName[B_FILE_NAME_LENGTH];
	
	vroster.Rewind();
	
	BMessage msg;
	
	ContactHandle handle;
	
	// query for all contacts on all drives
	while ( vroster.GetNextVolume(&vol) == B_OK )
	{
		if ((vol.InitCheck() != B_OK) || (vol.KnowsQuery() != true)) 
			continue;
		
		vol.GetName(volName);
		LOG("im_server", liLow, "StartQuery: Getting contacts on %s", volName);
		
		BQuery * query = new BQuery();
		
		query->SetTarget( BMessenger(this) );
		query->SetPredicate( "IM:connections=*" );
		query->SetVolume(&vol);
		query->Fetch();
		
		fQueries.push_back( query );
		
		while ( query->GetNextRef(&handle.entry) == B_OK )
		{
			node_ref nref;
			
			BNode node(&handle.entry);
			if ( node.GetNodeRef( &nref ) == B_OK )
			{
				handle.node = nref.node;
				
				ContactMonitor_Added( handle );
			}
		}
	}

}

/**
	Load protocol add-ons and init them
*/
status_t
Server::LoadAddons()
{
	BDirectory settingsDir; // base directory for protocol settings
	BDirectory addonsDir; // directory for protocol addons
	status_t rc;
	
	// STEP 1: Check if we can access settings for the protocols!
	BPath path;
	if ((rc=find_directory(B_USER_SETTINGS_DIRECTORY,&path,true)) != B_OK ||
		(rc=path.Append("im_kit/add-ons/protocols")) != B_OK ||
		(rc=settingsDir.SetTo(path.Path())) != B_OK)
	{ // we couldn't access the settings directory for the protocols!
		LOG("im_server", liHigh, "cannot access protocol settings directory: %s, error 0x%lx (%s)!", path.Path(), rc, strerror(rc));
		return rc;
	}
	
	// STEP 2: Check if we can access the add-on directory for the protocols!
	if ((rc=find_directory(B_USER_ADDONS_DIRECTORY, &path, true)) != B_OK ||
		(rc=path.Append("im_kit/protocols")) != B_OK ||
		(rc=addonsDir.SetTo(path.Path())) != B_OK)
	{ // we couldn't access the addons directory for the protocols!
		LOG("im_server", liHigh, "cannot access protocol addon directory: %s, error 0x%lx (%s)!", path.Path(), rc, strerror(rc));
		return rc;
	}
	
	// Okies, we've been able to access our critical dirs, so now we should be sure we can load any addons that are there
	UnloadAddons(); // make sure we don't load any addons twice

	// try loading all files in ./add-ons

	// get path
	BEntry entry;
	addonsDir.Rewind();
	while( addonsDir.GetNextEntry( (BEntry*)&entry, TRUE ) == B_NO_ERROR )
	{ // continue until no more files
		if( entry.InitCheck() != B_NO_ERROR )
			continue;
	
		if( entry.GetPath(&path) != B_NO_ERROR )
			continue;

		
		image_id curr_image = load_add_on( path.Path() );
		if( curr_image < 0 )
		{
			LOG("im_server", liHigh, "load_add_on() fail, file [%s]", path.Path());
			continue;
		}
		
		status_t res;
		// get name
		Protocol * (* load_protocol) ();
		res = get_image_symbol( 
			curr_image, "load_protocol", 
			B_SYMBOL_TYPE_TEXT, 
			(void **)&load_protocol
		);
		
		if ( res != B_OK )
		{
			LOG("im_server", liHigh, "get_image_symbol(load_protocol) fail, file [%s]", path.Path());
			unload_add_on( curr_image );
			continue;
		}
		
		Protocol * protocol = load_protocol();
		
		if ( !protocol )
		{
			LOG("im_server", liHigh, "load_protocol() fail, file [%s]", path.Path());
			unload_add_on( curr_image );
			continue;
		}
		
		LOG("im_server", liHigh, "Protocol loaded: [%s]", protocol->GetSignature());
		
		// try to read settings from protocol attribute
		BNode node(&settingsDir,protocol->GetSignature());
		if (node.InitCheck() == B_OK)
		{ // okies, ready to roll
			attr_info info;

			fStatus[protocol->GetSignature()] = OFFLINE_TEXT;

			if ((rc=node.GetAttrInfo("im_settings", &info)) == B_OK &&
				info.type == B_RAW_TYPE && info.size > 0)
			{ // found an attribute with data
				char* settings = new char[info.size]; // FIXME: does new[] throw when memory exhausted?
				node.ReadAttr("im_settings", info.type, 0, settings, info.size);

				LOG("im_server", liLow, "Read settings data");
				
				BMessage settings_msg;
				if ( settings_msg.Unflatten(settings) == B_OK )
				{
					protocol->UpdateSettings(settings_msg);
				}
				else
				{
					_ERROR("Error unflattening settings");
				}
				
				delete settings;
			}
			else
			{
				_ERROR("No settings found");
			}
		} // done handling settings

		if ((rc=protocol->Init( BMessenger(this) )) != B_OK)
		{
			LOG("im_server", liHigh, "Error initializing protocol '%s' (error 0x%ld/%s)!", protocol->GetSignature(), rc, strerror(rc));
			//FIXME: does protocol->Shutdown() have to be called?
			delete protocol;
			unload_add_on( curr_image );
		}
		else
		{
			// add to list
			fProtocols[protocol->GetSignature()] = protocol;
			
			// add to fAddOnInfo
			AddOnInfo pinfo;
			pinfo.protocol = protocol;
			pinfo.image = curr_image;
			pinfo.signature = protocol->GetSignature();
			pinfo.path = path.Path();
			fAddOnInfo[protocol] = pinfo;
			
			// get protocol settings template
			BMessage tmplate = protocol->GetSettingsTemplate();
			
			im_save_protocol_template( protocol->GetSignature(), &tmplate );
		}
	} // while()
	
	LOG("im_server", liMedium, "All add-ons loaded.");
	
	return B_OK;
}

/**
	Unloads add-on images after shutting them down.
*/
void
Server::UnloadAddons()
{
	map<Protocol*,AddOnInfo>::iterator i;
	
	for ( i=fAddOnInfo.begin(); i != fAddOnInfo.end(); i++ )
	{
		i->first->Shutdown();
		
		unload_add_on(i->second.image);
	}
	
	fAddOnInfo.clear();
	fProtocols.clear();
}

/**
	Add a listener endpoint that will receive all messages
	broadcasted from the im_server
*/
void
Server::AddEndpoint( BMessenger msgr )
{
	LOG("im_server", liDebug, "Endpoint added");
	fMessengers.push_back(msgr);
}

/**
	Remove a listener endpoint.
*/
void
Server::RemoveEndpoint( BMessenger msgr )
{
	LOG("im_server", liDebug, "Endpoint removed");
	fMessengers.remove(msgr);
}

/**
	Process an IM_MESSAGE BMessage, broadcasting or posting to protocols
	as needed to perform the requested operation. Called from MessageReceived()
	
	@param msg The message to process
*/
void
Server::Process( BMessage * msg )
{
	int32 im_what = -1;
	
	if ( msg->FindInt32("im_what",&im_what) != B_OK )
	{ // malformed message, skip
		_ERROR("Server::Process(): Malformed message", msg);
		return;
	}
	
	switch ( im_what )
	{
		// messages to protocols
		case GET_CONTACT_LIST:
		case SET_STATUS:
		case SEND_MESSAGE:
		case GET_CONTACT_INFO:
		case SEND_AUTH_ACK:
		case USER_STARTED_TYPING:
		case USER_STOPPED_TYPING:
		{
			MessageToProtocols(msg);
		}	break;
		
		// messages from protocols
		case STATUS_SET:
		case MESSAGE_SENT:
		case MESSAGE_RECEIVED:
		case STATUS_CHANGED:
		case CONTACT_LIST:
		case CONTACT_INFO:
		case CONTACT_AUTHORIZED:
		case CONTACT_STARTED_TYPING:
		case CONTACT_STOPPED_TYPING:
		{
			MessageFromProtocols(msg);
		}	break;
		// authorization requests
		case AUTH_REQUEST:
		{ // this should probably be in the client at a later time.
			BString authProtocol;
			BString authUIN;
			BString authMessage;

			BString authText;
			
			msg->FindString("protocol", &authProtocol);
			msg->FindString("id", &authUIN);
			msg->FindString("message", &authMessage);
			
			authText  = "Authorization request from ";
			authText += authUIN;
			authText += " :\n\n";
			authText += authMessage;
			authText += "\n\nDo you want to accept it?";

			BMessage * authReply = new BMessage(MESSAGE);
			authReply->AddInt32("im_what", SEND_AUTH_ACK);
			authReply->AddString("protocol", authProtocol.String());
			authReply->AddString("id", authUIN.String()); 

			BInvoker * authInv = new BInvoker(authReply, this);
			
			BAlert * authAlert = new BAlert("Auth Request Alert",
				authText.String(), "Yes", "No", NULL, 
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
			authAlert->Go(authInv);
		}	break;
		default:
			// unknown im_what opcode, skip and report
			_ERROR("Unknown im_what code",msg);
			return;
	}
}

/**
	Send a BMessage to all registered BMessengers after converting 'message' encoding if needed.
	
	@param msg The message to send
*/
void
Server::Broadcast( BMessage * msg )
{
	list<BMessenger>::iterator i;
	
	for ( i=fMessengers.begin(); i != fMessengers.end(); i++ )
	{
		if ( !(*i).IsTargetLocal() )
		{
			(*i).SendMessage(msg);
		} else
		{
			_ERROR("Broadcast(): messenger local");
		}
	}
}

/**
	Handle a query update, adding or removing Contacts as needed.
	
	@param msg The message containing the update information
*/
void
Server::HandleContactUpdate( BMessage * msg )
{
	int32 opcode=0;
	
	if ( msg->FindInt32("opcode",&opcode) != B_OK )
	{ // malformed message, skip
		return;
	}
	
	ContactHandle handle, from, to;
	const char	* name;
	
	switch ( opcode )
	{
		case B_ENTRY_CREATED:
			// .. add to contact list
			msg->FindString("name", &name );
			handle.entry.set_name(name);
			msg->FindInt64("directory", &handle.entry.directory);
			msg->FindInt32("device", &handle.entry.device);
			msg->FindInt64("node", &handle.node);
			
			ContactMonitor_Added( handle );
			break;
		case B_ENTRY_MOVED:
			// .. contact moved
			msg->FindString("name", &name );
			from.entry.set_name(name);
			to.entry.set_name(name);
			msg->FindInt64("from directory", &from.entry.directory);
			msg->FindInt64("to directory", &to.entry.directory);
			msg->FindInt32("device", &from.entry.device);
			msg->FindInt32("device", &to.entry.device);
			msg->FindInt64("node", &from.node);
			msg->FindInt64("node", &to.node);
			
			ContactMonitor_Moved( from, to );
			break;
		case B_ATTR_CHANGED:
			// .. add to contact list
			msg->FindInt32("device", &handle.entry.device);
			msg->FindInt64("node", &handle.node);
			
			ContactMonitor_Modified( handle );
			break;
		case B_ENTRY_REMOVED:
			// .. remove from contact list
			msg->FindInt64("directory", &handle.entry.directory);
			msg->FindInt32("device", &handle.entry.device);
			msg->FindInt64("node", &handle.node);
			
			ContactMonitor_Removed( handle );
			break;
	}
}

/**
	Find a Contact associated with a given protocol:id pair
	
	@param proto_id The protocol:id pair to search for
*/
Contact
Server::FindContact( const char * proto_id )
{
	BVolumeRoster 	vroster;
	BVolume			vol;
	char 			volName[B_FILE_NAME_LENGTH];

	vroster.Rewind();
	
	Contact result;
	
	string pred = 
		string( 
			string("IM:connections=\"*") + 
			string(proto_id) +
			string("*\"")
		);
	
	while ( vroster.GetNextVolume(&vol) == B_OK )
	{
		if ((vol.InitCheck() != B_OK) || (vol.KnowsQuery() != true)) 
			continue;
		
		vol.GetName(volName);
		
		BQuery query;
		
		query.SetPredicate( pred.c_str() );
		
		query.SetVolume(&vol);
		
		query.Fetch();
		
		entry_ref entry;
		
		if ( query.GetNextRef(&entry) == B_OK )
		{
			result.SetTo(entry);
			break;
		}
	}
	
	return result;
}

/**
	Create a new People file with a unique name on the form "Unknown contact X"
	and add the specified proto_id to it
	
	@param proto_id The protocol:id connection of the new contact
*/
Contact
Server::CreateContact( const char * proto_id )
{
	LOG("im_server", liHigh, "Creating new contact for connection [%s]", proto_id);
	
	Contact result;
	
	BDirectory dir("/boot/home/people");
	BFile file;
	BEntry entry;
	char filename[512];
	
	// make sure that the target directory exists before we try to create
	// new files

	// TO BE ADDED
	
	// create a new 
	for (int i=1; file.InitCheck() != B_OK; i++ )
	{
		sprintf(filename,"Unknown contact %d",i);
		
		dir.CreateFile(filename,&file,true);
	}
	
	if ( dir.FindEntry(filename,&entry) != B_OK )
	{
		LOG("im_server", liHigh, "Error: While creating a new contact, dir.FindEntry() failed. filename was [%s]",filename);
		return result;
	}
	
	LOG("im_server", liDebug, "  created file [%s]", filename);
	
	// file created. set type and add connection
	if ( file.WriteAttr(
		"BEOS:TYPE", B_STRING_TYPE, 0,
		"application/x-person", 21
	) != 21 ) 
	{ // error writing type
		entry.Remove();
		_ERROR("Error writing type to created contact");
		return result;
	}
	
	LOG("im_server", liDebug, "  wrote type");
	
	// file created. set type and add connection
	result.SetTo( entry );
	
	if ( result.AddConnection(proto_id) != B_OK )
	{
		return Contact();
	}
	
	LOG("im_server", liDebug, "  wrote connection");
	
	if ( result.SetStatus(OFFLINE_TEXT) != B_OK )
	{
		return Contact();
	}
	
	LOG("im_server", liDebug, "  wrote status");
	
	// post request info about this contact
	BMessage msg(MESSAGE);
	msg.AddInt32("im_what", GET_CONTACT_INFO);
	msg.AddRef("contact", result);
	
	BMessenger(this).SendMessage(&msg);
	
	LOG("im_server", liDebug, "  done.");
	
	return result;
}

/**
	Select the 'best' protocol for sending a message to contact
*/
string
Server::FindBestProtocol( Contact & contact )
{
	char connection[255];
	
	string protocol = "";
	
	// first of all, check if source of last message is still online
	// if it is, we use it.
	
	if ( fPreferredProtocol[contact].length() > 0 )
	{
		protocol = fPreferredProtocol[contact];
		
		if ( contact.FindConnection(protocol.c_str(), connection) == B_OK )
		{
			if ( fStatus[connection] == AWAY_TEXT || fStatus[connection] == ONLINE_TEXT )
			{
				if ( fStatus[protocol] != OFFLINE_TEXT )
				{
					LOG("im_server", liDebug, "Using preferred protocol %s", protocol.c_str() );
					return protocol;
				}
			}
		}
	}
	
	// look for an online protocol
	for ( int i=0; contact.ConnectionAt(i,connection) == B_OK; i++ )
	{
		string curr = connection;
		
		if ( fStatus[curr] == AWAY_TEXT || fStatus[curr] == ONLINE_TEXT )
		{
			// extract protocol part of connection
			int separator_pos = curr.find(":");
			curr.erase(separator_pos, strlen(connection)-separator_pos);
			
			if ( fStatus[curr] != OFFLINE_TEXT )
			{ // make sure WE'RE online on this protocol too
				protocol = curr;
				LOG("im_server", liDebug, "Using online protocol %s", protocol.c_str() );
				return protocol;
			}
		}
	}
	
	// no online protocol found, look for one capable of offline messaging
	for ( map<string,Protocol*>::iterator p = fProtocols.begin();
			p != fProtocols.end(); p++ )
	{ // loop over protocols
		if ( p->second->HasCapability( Protocol::OFFLINE_MESSAGES ) )
		{ // does this protocol handle offline messages?
			if ( contact.FindConnection( p->second->GetSignature(), connection ) == B_OK )
			{ // check if contact has a connection for this protocol
				if ( fStatus[p->second->GetSignature()] != OFFLINE_TEXT )
				{ // make sure we're online with this protocol
					protocol = p->first;
					LOG("im_server", liDebug, "Using offline protocol %s", protocol.c_str() );
					return protocol;
				}
			}
		}
	}
	
	// No matching protocol!
	
	return "";
}

/**
	Forward message from client-side to protocol-side
*/
void
Server::MessageToProtocols( BMessage * msg )
{
	entry_ref entry;
	
	if ( msg->FindRef("contact",&entry) == B_OK )
	{ // contact present, select protocol and ID
		Contact contact(entry);
		
		if ( contact.InitCheck() != B_OK )
		{ // invalid target
			_SEND_ERROR("Invalid target, no such contact", msg);
			return;
		}
		
		if ( msg->FindString("protocol") == NULL )
		{ // no protocol specified, figure one out
			string protocol = FindBestProtocol(contact);
			
			if ( protocol == "")
			{ // No available connection, can't send message!
				LOG("im_server", liHigh, "Can't send message, no possible connection");
				
				// send ERROR message here..
				BMessage error(ERROR);
				error.AddRef("contact", contact);
				error.AddString("error", "Can't send message, no available connections. Go online!");
				error.AddMessage("message", msg);
				
				Broadcast( &error );
				return;
			}
			
			msg->AddString("protocol", protocol.c_str() );
		}// done chosing protocol
		
		if ( fStatus[msg->FindString("protocol")] == OFFLINE_TEXT )
		{ // selected protocol is offline, impossible to send message
			BString error_str;
			error_str << "Not connected to selected protocol [";
			error_str << msg->FindString("protocol");
			error_str << "], cannot send message";
			
			_ERROR(error_str.String(), msg);
			
			BMessage err( IM::ERROR );
			err.AddRef("contact", contact );
			err.AddString("error", error_str.String() );
			
			Broadcast( &err );
			
			return;
		}
		
		if ( msg->FindString("id") == NULL )
		{ // add protocol-specific ID from Contact if not present
			char connection[255];
			
			if ( contact.FindConnection(msg->FindString("protocol"),connection) != B_OK )
			{
				_ERROR("Couldn't get connection for protocol",msg);
				return;
			}
			
			const char * id=NULL;
			
			for ( int i=0; connection[i]; i++ )
				if ( connection[i] == ':' )
				{
					id = &connection[i+1];
					break;
				}
			
			if ( !id )
			{
				_ERROR("Couldn't get ID from connection",msg);
				return;
			}
			
			msg->AddString("id", id );
		}
	} // done selecting protocol and ID
	
	// copy message so we can broadcast it later, with data intact
	BMessage client_side_msg(*msg);
	
	if ( msg->FindString("protocol") == NULL )
	{ // no protocol specified, send to all?
		LOG("im_server", liLow, "No protocol specified");
		
		int32 im_what=-1;
		msg->FindInt32("im_what", &im_what);
		
		switch ( im_what )
		{ // send these messages to all loaded protocols
			case SET_STATUS:
			{
				LOG("im_server", liLow, "  SET_STATUS - Sending to all protocols");
				map<string,Protocol*>::iterator i;
				
				for ( i=fProtocols.begin(); i != fProtocols.end(); i++ )
				{
					if ( i->second->Process(msg) != B_OK )
					{
						_ERROR("Protocol reports error processing message");
					}
				}
			}	break;
			default:
				_ERROR("Invalid message", msg);
				return;
		}
	} else
	{ // protocol mapped
		if ( fProtocols.find(msg->FindString("protocol")) == fProtocols.end() )
		{ // invalid protocol, report and skip
			_ERROR("Protocol not loaded or not installed", msg);
			
			printf("looking for [%s], loaded protocols:\n", msg->FindString("protocol") );
			
			map<string,Protocol*>::iterator i;
			
			for ( i=fProtocols.begin(); i != fProtocols.end(); i++ )
			{
				printf("  [%s]\n", i->first.c_str());
			}
			
			printf("  end of list.\n");
			
			_SEND_ERROR("Protocol not loaded or not installed", msg);
			return;
		}
		
		Protocol * p = fProtocols[msg->FindString("protocol")];		
		
		if ( p->GetEncoding() != 0xffff )
		{ // convert to desired charset
			int32 charset = p->GetEncoding();
			
			msg->AddInt32("charset", charset );
			client_side_msg.AddInt32("charset", charset);
			
			type_code _type;
			char * name;
			int32 count;

#if B_BEOS_VERSION > B_BEOS_VERSION_5			
			for ( int i=0; msg->GetInfo(B_STRING_TYPE, i, (const char **)&name, &_type, &count) == B_OK; i++ )
#else
			for ( int i=0; msg->GetInfo(B_STRING_TYPE, i, &name, &_type, &count) == B_OK; i++ )
#endif
			{ // get string names
				for ( int x=0; x<count; x++ )
				{ // replace all matching strings
					const char * data = msg->FindString(name,x);
					
					int32 src_len = strlen(data);
					int32 dst_len = strlen(data)*5;
					int32 state = 0;
					
					char * new_data = new char[dst_len];
					
					if ( convert_from_utf8(
						charset,
						data,		&src_len,
						new_data,	&dst_len,
						&state				
					) == B_OK )
					{
						new_data[dst_len] = 0;
						
						msg->ReplaceString(name,x,new_data);
					}
					
					delete[] new_data;
				}
			}
		} // done converting charset
		
		if ( p->Process(msg) != B_OK )
		{
			_SEND_ERROR("Protocol reports error processing message", msg);
			return;
		}
	}
	
	// broadcast client_side_msg, since clients want utf8 text, and that has
	// been replaced in msg with protocol-specific data
	Broadcast( &client_side_msg );
}


/**
	Handle a message coming from protocol-side to client-side
*/
void
Server::MessageFromProtocols( BMessage * msg )
{
	const char * protocol = msg->FindString("protocol");
	
	// convert strings to utf8
	type_code _type;
	char * name;
	int32 count;
	int32 charset;
	
	if ( msg->FindInt32("charset",&charset) == B_OK )
	{ // charset present, convert all strings
#if B_BEOS_VERSION > B_BEOS_VERSION_5
		for ( int i=0; msg->GetInfo(B_STRING_TYPE, i, (const char **)&name, &_type, &count) == B_OK; i++ )
#else
		for ( int i=0; msg->GetInfo(B_STRING_TYPE, i, &name, &_type, &count) == B_OK; i++ )
#endif
		{ // get string names
			for ( int x=0; x<count; x++ )
			{ // replace all matching strings
				const char * data = msg->FindString(name,x);
				
				int32 src_len = strlen(data);
				int32 dst_len = strlen(data)*5;
				int32 state = 0;
				
				char * new_data = new char[dst_len];
				
				if ( convert_to_utf8(
					charset,
					data,		&src_len,
					new_data,	&dst_len,
					&state				
				) == B_OK )
				{
					new_data[dst_len] = 0;
					
					msg->ReplaceString(name,x,new_data);
				}
				
				delete[] new_data;
			}
		}
	}	
	// done converting
	
	int32 im_what;
	
	msg->FindInt32("im_what",&im_what);
	
	entry_ref testc;
	
	if ( msg->FindRef("contact",&testc) == B_OK )
	{
		_ERROR("contact already present in message supposedly from protocol",msg);
		return;
	}
	
	// find out which contact this message originates from
	Contact contact;
	
	const char * id = msg->FindString("id");
	
	if ( id != NULL )
	{ // ID present, find out which Contact it belongs to
		if ( protocol == NULL )
		{ // malformed message. report and skip.
			_ERROR("Malformed message in Server::Process", msg);
			return;
		}
		
		string proto_id( string(protocol) + string(":") + string(id) );
		
		contact = FindContact(proto_id.c_str());
		
		if ( contact.InitCheck() != B_OK )
		{ // No matching contact, create a new one!
			contact.SetTo( CreateContact( proto_id.c_str() ) );
			
			// register the contact we created
			BMessage connection(MESSAGE);
			connection.AddInt32("im_what", REGISTER_CONTACTS);
			connection.AddString("id", id);
			
			Protocol * p = fProtocols[protocol];
			
			p->Process( &connection );			
		}
	}
	
	if ( contact.InitCheck() == B_OK )
	{
		msg->AddRef("contact",contact);
		
		if ( im_what == MESSAGE_RECEIVED )
		{ // message received from contact, store the protocol in fPreferredProtocol
			char status[256];
			contact.GetStatus(status,sizeof(status));
			
			if ( strcmp(status,BLOCKED_TEXT) == 0 )
			{ // contact blocked, dropping message!
				LOG("im_server", liHigh, "Dropping message from blocked contact [%s:%s]", protocol, id);
				return;
			}
			
			fPreferredProtocol[contact] = protocol;
			LOG("im_server", liLow, "Setting preferred protocol for [%s:%s] to %s", protocol, id, protocol );
		}
	}
	
	if ( im_what == STATUS_CHANGED && protocol != NULL && id != NULL )
	{ // update status list on STATUS_CHANGED
		UpdateStatus(msg,contact);
	}
	
	if ( im_what == STATUS_SET && protocol != NULL )
	{ // own status set for protocol, register the id's we're interested in etc
		handle_STATUS_SET(msg);
	}
	
	if ( im_what == CONTACT_AUTHORIZED && protocol != NULL ) {
		LOG("im_server", liLow, "Creating new contact on authorization. ID : %s", id);
	} else {
		// send it
		Broadcast(msg);
	}
}

/**
	Update the status of a given contact. Set im_server internal
	status and calls UpdateContactStatusAttribute to update IM:status attribute
	
	@param msg 			The message is a STATUS_CHANGED message.
	@param contact 		The contact to update
*/
void
Server::UpdateStatus( BMessage * msg, Contact & contact )
{
	const char * status = msg->FindString("status");
	const char * protocol = msg->FindString("protocol");
	const char * id = msg->FindString("id");
	
	if ( !status )
	{
		_ERROR("Missing 'status' in STATUS_CHANGED message",msg);
		return;
	}
	
	string proto_id( string(protocol) + string(":") + string(id) );
	
	string new_status = status;
	
	LOG("im_server", liMedium, "STATUS_CHANGED [%s] is now %s",proto_id.c_str(),new_status.c_str());
	
	// add old status to msg
	if ( fStatus[proto_id] != "" )
		msg->AddString( "old_status", fStatus[proto_id].c_str() );
	else
		msg->AddString( "old_status", OFFLINE_TEXT );
	
	// update status
	fStatus[proto_id] = new_status;
	
	// Add old total status to msg, to remove duplicated message to user
	char total_status[512];
	if ( contact.GetStatus(total_status, sizeof(total_status)) == B_OK )
		msg->AddString("old_total_status", total_status);
	
	// calculate total status for contact
	UpdateContactStatusAttribute(contact);

	// Add new total status to msg, to remove duplicated message to user
	if ( contact.GetStatus(total_status, sizeof(total_status)) == B_OK )
		msg->AddString("total_status", total_status);
}


/**
	Calculate total status for contact and update IM:status attribute
	and the icons too.
*/
void
Server::UpdateContactStatusAttribute( Contact & contact )
{
	// calculate total status for contact
	string new_status = OFFLINE_TEXT;
	
	for ( int i=0; i<contact.CountConnections(); i++ )
	{
		char connection[512];
		
		contact.ConnectionAt(i,connection);
		
		string curr = fStatus[connection];
		
		if ( curr == ONLINE_TEXT )
		{
			new_status = ONLINE_TEXT;
			break;
		}
		
		if ( curr == AWAY_TEXT && new_status == OFFLINE_TEXT )
		{
			new_status = AWAY_TEXT;
		}
	}
	
	//LOG("im_server", liMedium, "STATUS_CHANGED total status is now %s", new_status.c_str());
	
	// update status attribute
	BNode node(contact);
	
	if ( node.InitCheck() != B_OK )
	{
		_ERROR("ERROR: Invalid node when setting new status");
	} else
	{ // node exists, write status
		const char * status = new_status.c_str();
		
		// check if blocked
		char old_status[256];
		bool is_blocked = false;
		
		if ( contact.GetStatus(old_status, sizeof(old_status)) == B_OK )
		{
			if ( strcmp(old_status, BLOCKED_TEXT) == 0 )
				is_blocked = true;
		}
		
		if ( !is_blocked )
		{ // only update IM:status if not blocked
			if ( strcmp(old_status, status) == 0 )
			{ // status not changed, done
				return;
			}
			
			contact.SetStatus( status );
		}
		
		BBitmap *large = NULL;
		BBitmap *small = NULL;
		
		BString pointerName = status;
		pointerName << "_small";
		
		if ( fIcons.FindPointer(pointerName.String(), reinterpret_cast<void **>(&small)) != B_OK )
		{
			LOG("im_server", liDebug, "Couldn't find large icon..");
		}
		
		pointerName = status;
		pointerName << "_large";
		
		if ( fIcons.FindPointer(pointerName.String(), reinterpret_cast<void **>(&large)) != B_OK )
		{
			LOG("im_server", liDebug, "Couldn't find small icon..");
		}
		
		if (large != NULL) {
			if ( node.WriteAttr(BEOS_LARGE_ICON_ATTRIBUTE, 'ICON', 0, large->Bits(), 
					large->BitsLength()) != large->BitsLength() )
			{
				LOG("im_server", liDebug, "Couldn't write large icon..");
			}
		} else {
			node.RemoveAttr(BEOS_LARGE_ICON_ATTRIBUTE);
		};	

		if (small != NULL) {
			if ( node.WriteAttr(BEOS_SMALL_ICON_ATTRIBUTE, 'MICN', 0, small->Bits(), 
				small->BitsLength()) != small->BitsLength() )
			{
				LOG("im_server", liDebug, "Couldn't write small icon..");
			}
		} else {
			node.RemoveAttr(BEOS_SMALL_ICON_ATTRIBUTE);
		};
	}
	
	node.Unset();
}


/**
	Query for all files with a IM:status and set it to OFFLINE_TEXT
*/
void
Server::SetAllOffline()
{
	BVolumeRoster 	vroster;
	BVolume			vol;
	char 			volName[B_FILE_NAME_LENGTH];

	vroster.Rewind();
	
	BMessage msg;
	entry_ref entry;
	
	// query for all contacts on all drives first
	//
	while ( vroster.GetNextVolume(&vol) == B_OK )
	{
		if ((vol.InitCheck() != B_OK) || (vol.KnowsQuery() != true)) 
			continue;
		
		vol.GetName(volName);
		LOG("im_server", liLow, "SetAllOffline: Getting contacts on %s", volName);
		
		BQuery query;
		
		query.SetPredicate( "IM:connections=*" );
		query.SetVolume(&vol);
		query.Fetch();
		
		while ( query.GetNextRef(&entry) == B_OK )
		{
			msg.AddRef("contact",&entry);
		}
	}
	
	// set status of all found contacts to OFFLINE_TEXT (skipping blocked ones)
	char nickname[512], name[512], filename[512], status[512];
	
	Contact c;
	for ( int i=0; msg.FindRef("contact",i,&entry) == B_OK; i++ )
	{
		c.SetTo(&entry);
		
		if ( c.InitCheck() != B_OK )
			_ERROR("SetAllOffline: Contact invalid");
		
		if ( c.GetNickname(nickname,sizeof(nickname)) != B_OK )
			strcpy(nickname,"<no nick>");
		if ( c.GetName(name,sizeof(name)) != B_OK )
			strcpy(name,"<no name>");
		BEntry e(&entry);
		if ( e.GetName(filename) != B_OK )
			strcpy(filename,"<no filename?!>");
			
		if ( c.GetStatus(status, sizeof(status)) == B_OK )
		{
			if ( strcmp(status, BLOCKED_TEXT) == 0 )
			{
				LOG("im_server", liDebug, "Skipping blocked contact %s (%s), filename: %s", name, nickname, filename);
				continue;
			}
		}
		
		LOG("im_server", liDebug, "Setting %s (%s) offline, filename: %s", name, nickname, filename);
		
		if ( c.SetStatus(OFFLINE_TEXT) != B_OK )
			LOG("im_server", liDebug, "  error.");
		
		BNode node(&entry);
		
		BBitmap *large = NULL;
		BBitmap *small = NULL;
		
		fIcons.FindPointer(OFFLINE_TEXT "_small", reinterpret_cast<void **>(&small));
		fIcons.FindPointer(OFFLINE_TEXT "_large", reinterpret_cast<void **>(&large));
		
		if (large != NULL) {
			node.WriteAttr(BEOS_LARGE_ICON_ATTRIBUTE, 'ICON', 0, large->Bits(), 
				large->BitsLength());
		} else {
			node.RemoveAttr(BEOS_LARGE_ICON_ATTRIBUTE);
		};	

		if (small != NULL) {
			node.WriteAttr(BEOS_SMALL_ICON_ATTRIBUTE, 'MICN', 0, small->Bits(), 
				small->BitsLength());
		} else {
			node.RemoveAttr(BEOS_SMALL_ICON_ATTRIBUTE);
		};
		
		node.Unset();
	}
}

/**
	Add the ID of all contacts that have a connections for this protocol
	to msg.
*/
void
Server::GetContactsForProtocol( const char * protocol, BMessage * msg )
{
	BVolumeRoster vroster;
	BVolume	vol;
	Contact result;
	BQuery query;
	list <entry_ref> refs;
	char volName[B_FILE_NAME_LENGTH];

	vroster.Rewind();

	while (vroster.GetNextVolume(&vol) == B_OK) {
		if ((vol.InitCheck() != B_OK) || (vol.KnowsQuery() != true))
			continue;
		
		vol.GetName(volName);
		LOG("im_server", liLow, "GetContactsForProtocol: Looking for contacts on %s", volName);
		query.PushAttr("IM:connections");
		query.PushString(protocol);
		query.PushOp(B_CONTAINS);
		
		query.SetVolume(&vol);
		
		query.Fetch();
		
		entry_ref entry;
		
		while (query.GetNextRef(&entry) == B_OK) {
			refs.push_back(entry);
		};
		
		query.Clear();
	};
	
//	refs.sort();
//	refs.unique();
	
	list <entry_ref>::iterator iter;
	for (iter = refs.begin(); iter != refs.end(); iter++) {
		Contact c(*iter);
		char conn[256];
		char *id = NULL;
		
		if (c.FindConnection(protocol, conn) == B_OK) {
			for (int i = 0; conn[i]; i++) {
				if (conn[i] == ':') {
					id = &conn[i+1];
					break;
				};
			};

			msg->AddString("id", id);
		};
	};
	
	//LOG("im_server", liDebug, "GetConnectionsForProcol(%s)", msg, protocol );
	
	refs.clear();
}

/**
	Generate a settings template for im_server settings
*/
BMessage
Server::GenerateSettingsTemplate()
{
	BMessage main_msg(SETTINGS_TEMPLATE);
	
	BMessage blink_db;
	blink_db.AddString("name", "blink_db");
	blink_db.AddString("description", "Blink Deskbar icon");
	blink_db.AddInt32("type", B_BOOL_TYPE );
	blink_db.AddBool("default", true );
	
	main_msg.AddMessage("setting", &blink_db);
	
	BMessage auto_start;
	auto_start.AddString("name", "auto_start");
	auto_start.AddString("description", "Auto-start im_server");
	auto_start.AddInt32("type", B_BOOL_TYPE );
	auto_start.AddBool("default", true );
		
	main_msg.AddMessage("setting", &auto_start);
	
	BMessage log_level;
	log_level.AddString("name", "log_level");
	log_level.AddString("description", "Debug log threshold");
	log_level.AddInt32("type", B_STRING_TYPE );
	log_level.AddString("valid_value", "Debug");
	log_level.AddString("valid_value", "Low");
	log_level.AddString("valid_value", "Medium");
	log_level.AddString("valid_value", "High");
	log_level.AddString("valid_value", "Quiet");
	log_level.AddBool("default", "Debug" );
	
	main_msg.AddMessage("setting", &log_level);
	
	BMessage default_away;
	default_away.AddString("name", "default_away");
	default_away.AddString("description", "Away Message");
	default_away.AddInt32("type", B_STRING_TYPE);
	default_away.AddString("default", "I'm not here right now");
	default_away.AddBool("multi_line", true);

	main_msg.AddMessage("setting", &default_away);
	
	BMessage appsig;
	appsig.AddString("name", "app_sig");
	appsig.AddString("description", "Application signature");
	appsig.AddInt32("type", B_STRING_TYPE);
	appsig.AddBool("default", IM_SERVER_SIG );
	
	main_msg.AddMessage("setting", &appsig );
	
	return main_msg;
}

/**
	Update im_server settings from message
*/
status_t
Server::UpdateOwnSettings( BMessage settings )
{
	bool auto_start=false;
	
	if ( settings.FindBool("auto_start", &auto_start ) == B_OK )
	{
		if ( auto_start )
		{ // add auto-start
			if ( system("grep -q AAA_im_server_BBB /boot/home/config/boot/UserBootscript") )
			{ // not present in auto-start, add
				system("echo \"# Added by IM Kit.      AAA_im_server_BBB\" >> /boot/home/config/boot/UserBootscript");
				system("echo \"/boot/home/config/servers/im_server &  # AAA_im_server_BBB\" >> /boot/home/config/boot/UserBootscript");
			}
		} else
		{ // remove auto-start
			if ( system("grep -q AAA_im_server_BBB /boot/home/config/boot/UserBootscript") == 0 )
			{ // present in auto-start, remove
				system("grep -v AAA_im_server_BBB /boot/home/config/boot/UserBootscript > /tmp/im_kit_temp");
				system("cp /tmp/im_kit_temp /boot/home/config/boot/UserBootscript");
				system("rm /tmp/im_kit_temp");
			}
		}
	}
	
	return B_OK;
}

/**
	Start all registered auto-start apps
*/
void
Server::StartAutostartApps()
{
	BMessage clients;
	
	im_get_client_list( &clients );
	
	for ( int i=0; clients.FindString("client", i); i++ )
	{
		const char * client = clients.FindString("client", i);
		
		if ( strcmp("im_server", client) == 0 )
			continue;
		
		BMessage settings;
		
		if ( im_load_client_settings(client, &settings) == B_OK )
		{
			bool auto_start = false;
			const char * app_sig = NULL;
			
			settings.FindBool("auto_start", &auto_start);
			app_sig = settings.FindString("app_sig");
			
			if ( auto_start && app_sig)
			{
				LOG("im_server", liLow, "Starting app [%s]", app_sig );
				be_roster->Launch( app_sig );
			}
		}
	}
	
/*	for ( int i=0; settings.FindString(AUTOSTART_APPSIG_SETTING,i); i++ )
	{
		LOG("im_server", liLow, "Starting app [%s]", settings.FindString(AUTOSTART_APPSIG_SETTING,i) );
		be_roster->Launch( settings.FindString(AUTOSTART_APPSIG_SETTING,i) );
	}
*/
}

/**
	Stop all registered auto-start apps
*/
void
Server::StopAutostartApps()
{
	BMessage clients;
	
	im_get_client_list( &clients );
	
	for ( int i=0; clients.FindString("client", i); i++ )
	{
		const char * client = clients.FindString("client", i);
		
		if ( strcmp("im_server", client) == 0 )
			continue;
		
		BMessage settings;
		
		if ( im_load_client_settings(client, &settings) == B_OK )
		{
			bool auto_start = false;
			const char * app_sig = NULL;
			
			settings.FindBool("auto_start", &auto_start);
			app_sig = settings.FindString("app_sig");
			
			if ( auto_start && app_sig)
			{
				LOG("im_server", liLow, "Stopping app [%s]", app_sig );
				BMessenger msgr( app_sig );
				msgr.SendMessage( B_QUIT_REQUESTED );
			}
		}
	}
	
/*	BMessage settings;
	
	GetSettings( NULL, &settings );
	
	for ( int i=0; settings.FindString(AUTOSTART_APPSIG_SETTING,i); i++ )
	{
		LOG("im_server", liLow, "Stopping app [%s]", settings.FindString(AUTOSTART_APPSIG_SETTING,i) );
		BMessenger msgr( settings.FindString(AUTOSTART_APPSIG_SETTING,i) );
		msgr.SendMessage( B_QUIT_REQUESTED );
	}
*/
}

/**
	Forward a message to deskbar icon
*/
void
Server::handleDeskbarMessage( BMessage * msg )
{
	switch ( msg->what )
	{
		case REGISTER_DESKBAR_MESSENGER:
			LOG("im_server", liDebug, "Got Deskbar messenger");
			msg->FindMessenger("msgr", &fDeskbarMsgr);
			break;
		
		default:
			LOG("im_server", liDebug, "Forwarding message to Deskbar");
			if ( fDeskbarMsgr.SendMessage(msg) != B_OK )
			{
				LOG("im_server", liMedium, "Error sending message to Deskbar");
			}
			break;
	}
}

/**
	Handle a STATUS_SET message, update per-protocol and total status
*/
void
Server::handle_STATUS_SET( BMessage * msg )
{
	const char * protocol = msg->FindString("protocol");
	
	const char * status = msg->FindString("status");
		
	if ( !status )
	{
		_ERROR("ERROR: STATUS_SET: status not in message",msg);
		return;
	}
	
	if ( strcmp(ONLINE_TEXT,status) == 0 )
	{ // we're online. register contacts. (should be: only do this if we were offline)
		if ( fProtocols.find(protocol) == fProtocols.end() )
		{
			_ERROR("ERROR: STATUS_SET: Protocol not loaded",msg);
			return;
		}
			
		Protocol * p = fProtocols[protocol];
			
		// THIS IS NOT CORRECT! We need to do this on a "connected" message, not on
		// status changed!
		BMessage connections(MESSAGE);
		connections.AddInt32("im_what", REGISTER_CONTACTS);
		GetContactsForProtocol( p->GetSignature(), &connections );
		
		p->Process( &connections );
	}
	
	if ( strcmp(OFFLINE_TEXT,status) == 0 )
	{ // we're offline. set all connections for protocol to offline
		if ( fProtocols.find(protocol) == fProtocols.end() )
		{
			_ERROR("ERROR: STATUS_SET: Protocol not loaded",msg);
			return;
		}
		
		BMessage contacts;
		
		GetContactsForProtocol(protocol, &contacts );
		
		for ( int i=0; contacts.FindString("id", i); i++ )
		{
			string proto_id;
			proto_id += protocol;
			proto_id += ":";
			proto_id += contacts.FindString("id", i);
			
			if ( fStatus[proto_id] != OFFLINE_TEXT  && fStatus[proto_id] != "" )
			{ // only send a message if there's been a change.
				BMessage update(MESSAGE);
				update.AddInt32("im_what", STATUS_CHANGED);
				update.AddString("protocol", protocol);
				update.AddString("id", contacts.FindString("id",i) );
				update.AddString("status", OFFLINE_TEXT);
				
				BMessenger(this).SendMessage( &update );
				
				fStatus[proto_id] = OFFLINE_TEXT;
			}
		}
	}
	//fAddOnInfo[protocol].online_status = status;
	
	// Find out 'total' online status
	fStatus[protocol] = status;
	
	string total_status = OFFLINE_TEXT;
	
	for ( map<string,Protocol*>::iterator i = fProtocols.begin(); i != fProtocols.end(); i++ )
	{
		
		if ( fStatus[i->second->GetSignature()] == ONLINE_TEXT )
		{
			total_status = ONLINE_TEXT;
			break;
		}
		
		if ( fStatus[i->second->GetSignature()] == AWAY_TEXT )
		{
			total_status = AWAY_TEXT;
		}
	}
	
	msg->AddString("total_status", total_status.c_str() );
	LOG("im_server", liMedium, "Total status changed to %s", total_status.c_str() );
	// end 'Find out total status'
	
	handleDeskbarMessage(msg);
}

/**
	Get current online status per-protocol for a contact
*/
void
Server::reply_GET_CONTACT_STATUS( BMessage * msg )
{
	entry_ref ref;
	
	if ( msg->FindRef("contact",&ref) != B_OK )
	{
		_ERROR("Missing contact in GET_CONTACT_STATUS",msg);
		return;
	}
	
	Contact contact(ref);
	char connection[255];
	
	BMessage reply;
	reply.AddRef("contact", &ref);
	
	for ( int i=0; contact.ConnectionAt(i,connection) == B_OK; i++ )
	{
		reply.AddString("connection", connection);
		if ( fStatus[connection] == "" )
			reply.AddString("status", OFFLINE_TEXT );
		else
			reply.AddString("status", fStatus[connection].c_str() );
	}
	
	msg->SendReply(&reply);
}

/**
	Get list of current own online status per protocol
*/
void Server::reply_GET_OWN_STATUSES(BMessage *msg) {
	LOG("im_server", liLow, "Got own status request. There are %i statuses",
		fStatus.size());
	BMessage reply(B_REPLY);

	map <string, Protocol *>::iterator pit;
	for (pit = fProtocols.begin(); pit != fProtocols.end(); pit++) {
		Protocol *p = pit->second;
		reply.AddString("protocol", p->GetSignature());
		reply.AddString("status", fStatus[p->GetSignature()].c_str());
	};

	msg->SendReply(&reply);
};

/**
	Returns a list of currently loaded protocols
*/
void
Server::reply_GET_LOADED_PROTOCOLS( BMessage * msg )
{
	BMessage reply( ACTION_PERFORMED );
	
	map<string,Protocol*>::iterator i;
	
	for ( i = fProtocols.begin(); i != fProtocols.end(); i++ )
	{
		reply.AddString("protocol", i->second->GetSignature() );
		entry_ref ref;
		get_ref_for_path(fAddOnInfo[i->second].path.String(), &ref);
		reply.AddRef("ref", &ref);
	}
	
	msg->SendReply( &reply );
}

/**
	?
*/
void
Server::reply_SERVER_BASED_CONTACT_LIST( BMessage * msg )
{
	const char * protocol = msg->FindString("protocol");
	
	if ( !protocol )
	{
		_ERROR("ERROR: Malformed SERVER_BASED_CONTACT_LIST message",msg);
		return;
	}
	
	for ( int i=0; msg->FindString("id",i); i++ )
	{ // for each ID
		const char * id = msg->FindString("id",i);
		
		string proto_id( string(protocol) + string(":") + string(id) );
		
		Contact c = FindContact( proto_id.c_str() );
		
		if ( c.InitCheck() != B_OK )
		{
			CreateContact( proto_id.c_str() );
		}
	}
}

/**
*/
void
Server::reply_UPDATE_CONTACT_STATUS( BMessage * msg )
{
	entry_ref ref;
	
	if ( msg->FindRef("contact", &ref) != B_OK )
	{
		_ERROR("Missing contact in UPDATE_CONTACT_STATUS",msg);
		return;
	}
	
	Contact contact(ref);
	
	UpdateContactStatusAttribute(contact);
}

/**
*/
void
Server::handle_SETTINGS_UPDATED( BMessage * msg )
{
	BMessage settings;
	
	const char * sig;
	
	if ( sig = msg->FindString("protocol") )
	{ // notify protocol of change in settings
		if ( fProtocols.find(sig) == fProtocols.end() )
		{
			_ERROR("Cannot notify protocol of changed settings, not loaded");
			return;
		}
		
		if ( im_load_protocol_settings(sig, &settings) != B_OK )
			return;
		
		Protocol * protocol = fProtocols[sig];
		
		if ( protocol->UpdateSettings(settings) != B_OK )
		{
			_ERROR("Protocol settings invalid", msg);
		}
	} else
	if ( sig = msg->FindString("client") )
	{ // notify client of change in settings
		Broadcast(msg);
	} else
	{ // malformed
		_ERROR("Malformed message in SETTINGS_UPDATED", msg);
	}
}

void
Server::InitSettings()
{
	// Save settings template
	BMessage tmplate = GenerateSettingsTemplate();
	
	im_save_client_template("im_server", &tmplate);
	
	// Make sure default settings are there
	BMessage settings;
	bool temp;
	const char * str;
	im_load_client_settings("im_server", &settings);
	if ( !settings.FindString("app_sig") )
		settings.AddString("app_sig", IM_SERVER_SIG);
	if ( settings.FindBool("auto_start", &temp) != B_OK )
		settings.AddBool("auto_start", false );
	if ( settings.FindString("log_level", &str) != B_OK )
		settings.AddString("log_level", "High" );
	im_save_client_settings("im_server", &settings);
	// done with template and settings.
}

void
Server::reply_GET_CONTACTS_FOR_PROTOCOL( BMessage * msg )
{
	if ( msg->FindString("protocol") == NULL )
	{
		msg->SendReply( ERROR );
		
		return;
	}
	
	BMessage reply(ACTION_PERFORMED);
	
	GetContactsForProtocol( msg->FindString("protocol"), &reply );
	
	msg->SendReply( &reply );
}

void
Server::ContactMonitor_Added( ContactHandle handle )
{
	for ( list< pair<ContactHandle, list<string>* > >::iterator i = fContacts.begin(); i != fContacts.end(); i++ )
	{
		if ( (*i).first == handle )
		{ // already in the list, skip
			return;
		}
	}
	
	Contact contact(&handle.entry);
	
	list<string> * connections = new list<string>();
	
	char connection[512];
	
	for ( int i=0; contact.ConnectionAt(i,connection) == B_OK; i++ )
	{
		connections->push_back( connection );
		
		Connection conn( connection );
		
		if ( fProtocols.find(conn.Protocol()) != fProtocols.end() )
		{ // protocol loaded, register connection
			BMessage remove(MESSAGE);
			remove.AddInt32("im_what", UNREGISTER_CONTACTS);
			remove.AddString("id", conn.ID() );
			
			(fProtocols[conn.Protocol()])->Process(&remove);
		}
	}
	
	connections->sort();
	
	node_ref nref;
	nref.device = handle.entry.device;
	nref.node = handle.node;
	
	watch_node( &nref, B_WATCH_ALL, BMessenger(this) );
	
	fContacts.push_back( pair<ContactHandle,list<string>* >(handle, connections) );

	LOG("im_server", liDebug, "Contact added (%s)", handle.entry.name);
}

void
Server::ContactMonitor_Modified( ContactHandle handle )
{
	for ( list< pair<ContactHandle, list<string>* > >::iterator i = fContacts.begin(); i != fContacts.end(); i++ )
	{
		if ( (*i).first == handle )
		{
			// update stuff here..
			list<string> connections;
			
			Contact contact( &(*i).first.entry );
			char connection[512];
			
			for ( int x=0; contact.ConnectionAt(x,connection) == B_OK; x++ )
			{
				connections.push_back( connection );
			}
			
			connections.sort();
			
			list<string> removed;
			
			// look for removed connections
			for ( list<string>::iterator j=(*i).second->begin(); j != (*i).second->end(); j++ )
			{ // for each connection in old contact data
				list<string>::iterator k = find(connections.begin(), connections.end(), *j);
				
				if ( k == connections.end() )
				{ // connection removed!
					removed.push_back( j->c_str() );
					LOG("im_server", liDebug, "Connection removed: %s\n", j->c_str() );
					
					Connection conn( j->c_str() );
					
					if ( fProtocols.find(conn.Protocol()) != fProtocols.end() )
					{ // protocol loaded, unregister connection
						BMessage remove(MESSAGE);
						remove.AddInt32("im_what", UNREGISTER_CONTACTS);
						remove.AddString("id", conn.ID() );
						
						(fProtocols[conn.Protocol()])->Process(&remove);
						
						// remove from fStatus too
						map<string,string>::iterator iter = fStatus.find(*j);
						if ( iter != fStatus.end() )
							fStatus.erase( iter );
					}
				}
			}
			
			for ( list<string>::iterator j=removed.begin(); j != removed.end(); j++ )
			{ // actually remove the connections from the list
				(*i).second->remove( *j );
			}
			
			// look for added connections
			for ( list<string>::iterator j=connections.begin(); j != connections.end(); j++ )
			{ // for each connection in old contact data
				list<string>::iterator k = find((*i).second->begin(), (*i).second->end(), *j);
				
				if ( k == (*i).second->end() )
				{ // connection added!
					(*i).second->push_back( j->c_str() );
					LOG("im_server", liDebug, "Connection added: %s\n", j->c_str() );
					
					Connection conn( j->c_str() );
					
					if ( fProtocols.find(conn.Protocol()) != fProtocols.end() )
					{ // protocol loaded, register connection
						BMessage remove(MESSAGE);
						remove.AddInt32("im_what", REGISTER_CONTACTS);
						remove.AddString("id", conn.ID() );
						
						(fProtocols[conn.Protocol()])->Process(&remove);
					}
				}
			}
			
			UpdateContactStatusAttribute( contact );
			
			return;
		}
	}
}

void
Server::ContactMonitor_Moved( ContactHandle from, ContactHandle to )
{
	for ( list< pair<ContactHandle, list<string>* > >::iterator i = fContacts.begin(); i != fContacts.end(); i++ )
	{
		if ( (*i).first == from )
		{
			(*i).first = to;
			return;
		}
	}
}

void
Server::ContactMonitor_Removed( ContactHandle handle )
{
	node_ref nref;
	nref.device = handle.entry.device;
	nref.node = handle.node;
	
	watch_node( &nref, B_STOP_WATCHING, BMessenger(this) );
	
	for ( list< pair<ContactHandle, list<string>* > >::iterator i = fContacts.begin(); i != fContacts.end(); i++ )
	{
		if ( (*i).first == handle )
		{
			LOG("im_server", liDebug, "Contact removed (%s)", (*i).first.entry.name);
			
			for ( list<string>::iterator j = i->second->begin(); j != i->second->end(); j++ )
			{ // unregister connections
				Connection conn( j->c_str() );
						
				if ( fProtocols.find(conn.Protocol()) != fProtocols.end() )
				{ // protocol loaded, unregister connection
					BMessage remove(MESSAGE);
					remove.AddInt32("im_what", UNREGISTER_CONTACTS);
					remove.AddString("id", conn.ID() );
					
					(fProtocols[conn.Protocol()])->Process(&remove);
				}

				// remove from fStatus too
				map<string,string>::iterator iter = fStatus.find(*j);
				if ( iter != fStatus.end() )
					fStatus.erase( iter );
			}
			
			delete i->second;
			
			fContacts.erase(i);
			return;
		}
	}

}
