#include "im_server.h"

#include <libim/Constants.h>
#include "DeskbarIcon.h"

#include <image.h>
#include <Roster.h>
#include <Path.h>
#include <Directory.h>
#include <stdio.h>
#include <NodeMonitor.h>
#include <VolumeRoster.h>
#include <Volume.h>
#include <UTF8.h>
#include <libim/Helpers.h>
#include <Deskbar.h>
#include <Roster.h>

using namespace IM;


void
_ERROR( const char * error, BMessage * msg )
{
	LOG(error,msg);
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
		LOG("ERROR: %s",text);
	}
}

/**
	Default constructor. (Starts People-query and ?) loads add-ons
*/
Server::Server()
:	BApplication(IM_SERVER_SIG)
{
	BDirectory dir;
	dir.CreateDirectory("/boot/home/config/settings/im_kit", NULL);
	dir.CreateDirectory("/boot/home/config/settings/im_kit/add-ons", NULL);
	dir.CreateDirectory("/boot/home/config/settings/im_kit/add-ons/protocols", NULL);
	
	SetAllOffline();
	
	LoadAddons();
	
	// add deskbar icon
	BDeskbar db;

	if ( db.RemoveItem( DESKBAR_ICON_NAME ) != B_OK )
		LOG("Error removing deskbar icon (this is ok..)");
	
	IM_DeskbarIcon * i = new IM_DeskbarIcon();
	
	if ( db.AddItem( i ) != B_OK )
	{ // couldn't add BView, try entry_ref
		entry_ref ref;
		
		if ( be_roster->FindApp(IM_SERVER_SIG,&ref) == B_OK )
		{
			if ( db.AddItem( &ref ) != B_OK )
				LOG("Error adding icon to deskbar!");
		} else
		{
			LOG("be_roster->FindApp() failed");
		}
	}
	
	delete i;
	
	Run();
}

/**
	Default destructor. Unloads add-ons and frees any aquired resources.
*/
Server::~Server()
{
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
		// People messages. Both query and node monitor.
		case B_NODE_MONITOR:
		case B_QUERY_UPDATE:
			HandleContactUpdate(msg);
			break;

		// IM-Kit specified messages:
		
		case SERVER_BASED_CONTACT_LIST:
			ServerBasedContactList(msg);
			break;
		
		case GET_SETTINGS_TEMPLATE:
			GetSettingsTemplate(msg);
			break;
		
		case GET_SETTINGS:
			GetSettings(msg);
			break;
		
		case SET_SETTINGS:
			SetSettings(msg);
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
			GetLoadedProtocols(msg);
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
	Initializes People-query and fetches initial list of matches
	
	NOT IMPLEMENTED
*/
void
Server::StartQuery()
{
/*	// make sure we get the whole list before any query updates
	// show up in MessageReceived()
	Lock();
	
	entry_ref entry;
	
	fQuery.SetTarget( BMessenger(this) );
	fQuery.SetPredicate("IM:connections=*");
	
	// get initial list of matches
	while ( fQuery.GetNextRef( &entry ) == B_OK )
	{
		Contact contact(entry);
		
		for ( int i=0; i<contact.CountConnections(); i++ )
		{
			char connection[256];
			if ( contact.ConnectionAt(i,connection) != B_OK )
				continue;
			
			fContactNodes[connection] = contact;
		}
	}
	
	Unlock();
*/
}

/**
*/
void
Server::LoadAddons()
{
	UnloadAddons(); // make sure we don't load any addons twice

	// try loading all files in ./add-ons
	
	// get path
	app_info info;
    be_app->GetAppInfo(&info); 

	BPath path;
	BEntry entry(&info.ref); 
	entry.GetPath(&path); 
	path.GetParent(&path);
	path.Append("add-ons");
	
	// setup Directory to get list of files
	BDirectory dir( path.Path() );
	
	LOG("add-on directory: %s", path.Path());
	
	while( dir.GetNextEntry( (BEntry*)&entry, TRUE ) == B_NO_ERROR )
	{ // continue until no more files
		if( entry.InitCheck() != B_NO_ERROR )
			continue;
		
		if( entry.GetPath(&path) != B_NO_ERROR )
			continue;
		
		image_id curr_image = load_add_on( path.Path() );
		if( curr_image < 0 )
		{
			LOG("load_add_on() fail, file [%s]", path.Path());
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
			LOG("get_image_symbol(load_protocol) fail, file [%s]", path.Path());
			unload_add_on( curr_image );
			continue;
		}
		
		Protocol * protocol = load_protocol();
		
		if ( !protocol )
		{
			LOG("load_protocol() fail, file [%s]", path.Path());
			unload_add_on( curr_image );
		}
		
		LOG("Protocol loaded: [%s]", protocol->GetSignature());
		
		// add to list
		fProtocols[protocol->GetSignature()] = protocol;
		
		// add to fAddOnInfo
		AddOnInfo pinfo;
		pinfo.protocol = protocol;
		pinfo.image = curr_image;
		pinfo.signature = protocol->GetSignature();
		strcpy(pinfo.path,path.Path());
		fAddOnInfo[protocol] = pinfo;
		
		// try to read settings from protocol attribute
		char settings_path[512];
		sprintf(settings_path,"/boot/home/config/settings/im_kit/add-ons/protocols/%s",pinfo.signature);
		BDirectory sdir;
		sdir.CreateFile(settings_path,NULL,true);
		BNode node(settings_path);
		char settings[1024*1024];
		int32 num_read=0;
		
		num_read = node.ReadAttr(
			"im_settings", B_RAW_TYPE, 0,
			settings, sizeof(settings)
		);
		if ( num_read > 0 )
		{
			LOG("Read settings data");
			
			BMessage settings_msg;
			if ( settings_msg.Unflatten(settings) == B_OK )
			{
				protocol->UpdateSettings(settings_msg);
			} else
			{
				_ERROR("Error unflattening settings");
/*				for ( int i=0; i<num_read; i++ )
				{
					printf("%c",settings[i], settings[i]);
					if ( i%16 == 15 )
						printf("\n");
				}
				printf("\n");
				for ( int i=0; i<num_read; i++ )
				{
					printf("%01x,",settings[i], settings[i]);
					if ( i%16 == 15 )
						printf("\n");
				}
				printf("\n");
*/			}
		} else
		{
			_ERROR("No settings found");
		}
		
		// we should care about the result here..
		protocol->Init( BMessenger(this) );
	}
	LOG("All add-ons loaded.");
}

/**
	Currently does nothing. Will eventually unload add-on images.
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
*/
void
Server::AddEndpoint( BMessenger msgr )
{
//	printf("Endpoint added\n");
	fMessengers.push_back(msgr);
}

/**
*/
void
Server::RemoveEndpoint( BMessenger msgr )
{
//	printf("Endpoint removed\n");
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
		{
			MessageFromProtocols(msg);
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
	
	switch ( opcode )
	{
		case B_ENTRY_CREATED:
			// .. add to contact list
			break;
		case B_ENTRY_REMOVED:
			// .. remove from contact list
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
	BVolume			boot_volume;

	vroster.GetBootVolume(&boot_volume);
	
	Contact result;
	
	BQuery query;
	
	string pred = 
		string( 
			string("IM:connections=\"*") + 
			string(proto_id) +
			string("*\"")
		);
	
	query.SetPredicate( pred.c_str() );
	
	
	query.SetVolume(&boot_volume);
	
	query.Fetch();
	
	entry_ref entry;
	
	if ( query.GetNextRef(&entry) == B_OK )
	{
		result.SetTo(entry);
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
	LOG("Creating new contact for connection [%s]", proto_id);
	
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
		LOG("Error: While creating a new contact, dir.FindEntry() failed. filename was [%s]",filename);
		return result;
	}
	
	LOG("  created file [%s]", filename);
	
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
	
	LOG("  wrote type");
	
	// file created. set type and add connection
	result.SetTo( entry );
	
	if ( result.AddConnection(proto_id) != B_OK )
	{
		return Contact();
	}
	
	LOG("  wrote connection");
	
	if ( result.SetStatus("offline") != B_OK )
	{
		return Contact();
	}
	
	LOG("  wrote status");
	
	// post request info about this contact
	BMessage msg(MESSAGE);
	msg.AddInt32("im_what", GET_CONTACT_INFO);
	msg.AddRef("contact", result);
	
	PostMessage(&msg);
	
	LOG("  done.");
	
	return result;
}

void
Server::GetSettingsTemplate( BMessage * msg )
{
	const char * p = msg->FindString("protocol");
			
	if ( !p )
	{
		p = "";
	}
	
	BMessage t;
	
	if ( strlen(p) > 0 )
	{ // protocol settings
		if (fProtocols.find(p) == fProtocols.end() )
		{
			_SEND_ERROR("ERROR: GET_SETTINGS_TEMPLATE: Protocol not loaded",msg);
			return;
		}
		
		Protocol * protocol = fProtocols[p];
			
		t = protocol->GetSettingsTemplate();
	} else
	{ // im_server settings
		t = GenerateSettingsTemplate();
	}
	
	if ( !msg->ReturnAddress().IsValid() )
	{
		_ERROR("Invalid return address in GetSettingsTemplate()", msg);
	}

	msg->SendReply( &t );
}

void
Server::GetLoadedProtocols( BMessage * msg )
{
	BMessage reply( ACTION_PERFORMED );
			
	map<string,Protocol*>::iterator i;
	
	for ( i = fProtocols.begin(); i != fProtocols.end(); i++ )
	{
		reply.AddString("protocol", i->second->GetSignature() );
	}
			
	msg->SendReply( &reply );
}

void
Server::ServerBasedContactList( BMessage * msg )
{
//	printf("ServerBasedContactList\n");
//	return;
	
	Tracer t("ServerBasedContactList");
	
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

void
Server::GetSettings( BMessage * msg )
{
	const char * p = msg->FindString("protocol");
	
	if ( !p )
	{
		p = "";
	}
	
	char settings_path[512];
	
	// get path to settings file
	if ( strlen(p) == 0 )
	{ // im_server settings
		strcpy(settings_path,"/boot/home/config/settings/im_kit/im_server.settings");
	} else
	{ // protocol settings
		if ( fProtocols.find(p) == fProtocols.end() )
		{
			_SEND_ERROR("ERROR: GET_SETTINGS: Protocol not loaded",msg);
			return;
		}
		
		Protocol * protocol = fProtocols[p];
		
		sprintf(
			settings_path,
			"/boot/home/config/settings/im_kit/add-ons/protocols/%s",
			fAddOnInfo[protocol].signature
		);
	}
	
	char data[1024*1024];
	
	LOG("Reading settings from file: %s", settings_path);
	
	BNode node( settings_path );
	
	int32 num_read = node.ReadAttr(
		"im_settings", B_RAW_TYPE, 0,
		data, sizeof(data)
	);
			
	if ( num_read <= 0 )
	{
		_SEND_ERROR("ERROR: GET_SETTINGS: Error reading settings",msg);
		return;
	}
	
//	LOG("Read settings data");
	
	BMessage settings;
	
	if ( settings.Unflatten(data) != B_OK )
	{
		_SEND_ERROR("ERROR: GET_SETTINGS: Error unflattening settings",msg);
		return;
	}
	
	if ( msg->ReturnAddress().IsValid() )
	{
//		LOG("Sending settings");
		msg->SendReply(&settings);
	} else
	{
		_ERROR("Invalid return address");
	}
}

void
Server::SetSettings( BMessage * msg )
{
	const char * p = msg->FindString("protocol");
			
	if ( !p )
	{
		p = "";
	}
			
	BMessage settings;
	status_t res = B_ERROR;
	char settings_path[512];
		
	if ( msg->FindMessage("settings",&settings) != B_OK )
	{
		_SEND_ERROR("ERROR: SET_SETTINGS: Settings not specified", msg);
		return;
	}
			
	if ( settings.what != SETTINGS )
	{
		_SEND_ERROR("ERROR: SET_SETTINGS: Malformed settings message", msg);
		return;
	}
			
	// check settings and get path to settings file
	if ( strlen(p) == 0 )
	{ // im_server settings
		res = UpdateOwnSettings(settings);
		
		strcpy(settings_path,"/boot/home/config/settings/im_kit/im_server.settings");
	} else
	{ // protocol settings
		if ( fProtocols.find(p) == fProtocols.end() )
		{
			_SEND_ERROR("ERROR: GET_SETTINGS: Protocol not loaded",msg);
			return;
		}
			
		Protocol * protocol = fProtocols[p];
			
		res = protocol->UpdateSettings(settings);
			
		sprintf(
			settings_path,
			"/boot/home/config/settings/im_kit/add-ons/protocols/%s",
			fAddOnInfo[protocol].signature
		);
	}
	
	// check if settings are ok
	if ( res != B_OK )
	{
		_SEND_ERROR("ERROR: SET_SETTINGS: Protocol says settings not valid", msg);
		return;
	}

	// save settings
	char data[1024*1024];
	int32 data_size=settings.FlattenedSize();
	
	if ( settings.Flatten(data,data_size) != B_OK )
	{ // error flattening message
		_SEND_ERROR("ERROR: SET_SETTINGS: Error flattening settings message", msg);
		return;
	}
	
	BDirectory dir;
	dir.CreateFile(settings_path,NULL,true);

	BNode node( settings_path );
			
	if ( node.InitCheck() != B_OK )
	{
		_SEND_ERROR("ERROR: SET_SETTINGS: Error opening save file", msg);
		return;
	}
			
	int32 num_written = node.WriteAttr(
		"im_settings", B_RAW_TYPE, 0,
		data, data_size
	);
			
	if ( num_written != data_size )
	{ // error saving settings
		_SEND_ERROR("ERROR: SET_SETTINGS: Error saving settings", msg);
		return;
	}
			
	if ( msg->ReturnAddress().IsValid() )
	{
		LOG("Settings applied",&settings);
		msg->SendReply(ACTION_PERFORMED);
	}
}

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
			char connection[255];
			
			if ( contact.ConnectionAt(0,connection) != B_OK )
			{ // trying to send to a Contact with no connections
				_SEND_ERROR("Target contact has no connections", msg);
				return;
			}
			
			// truncate id part of protocol:id pair to get protocol
			for ( int i=0; connection[i]; i++ )
				if ( connection[i] == ':' )
				{
					connection[i] = 0;
					break;
				}
			
			msg->AddString("protocol",connection);
		} // done chosing protocol
		
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
		LOG("No protocol specified");
		
		int32 im_what=-1;
		msg->FindInt32("im_what", &im_what);
		
		switch ( im_what )
		{ // send these messages to all loaded protocols
			case SET_STATUS:
			{
				LOG("  SET_STATUS - Sending to all protocols");
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
			
			for ( int i=0; msg->GetInfo(B_STRING_TYPE, i, &name, &_type, &count) == B_OK; i++ )
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
	
//	client_side_msg.PrintToStream();
//	msg->PrintToStream();
	
	// broadcast client_side_msg, since clients want utf8 text, and that has
	// been replaced in msg with protocol-specific data
	Broadcast( &client_side_msg );
}

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
		for ( int i=0; msg->GetInfo(B_STRING_TYPE, i, &name, &_type, &count) == B_OK; i++ )
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
		}
	}
	
	if ( contact.InitCheck() == B_OK )
	{
		msg->AddRef("contact",contact);
	}
	
	if ( im_what == STATUS_CHANGED && protocol != NULL && id != NULL )
	{ // update status list on STATUS_CHANGED
		UpdateStatus(msg,contact);
	}
	
	if ( im_what == STATUS_SET && protocol != NULL )
	{ // own status set for protocol, register the id's we're interested in
		if ( !msg->FindString("status") )
		{
			_ERROR("ERROR: STATUS_SET: status not in message",msg);
			return;
		}
		
		if ( strcmp(ONLINE_TEXT,msg->FindString("status")) == 0 )
		{ // we're online. register contacts. (should be: only do this if we were offline)
			if ( fProtocols.find(protocol) == fProtocols.end() )
			{
				_ERROR("ERROR: STATUS_SET: Protocol not loaded",msg);
				return;
			}
			
			Protocol * p = fProtocols[protocol];
			
			BMessage connections(MESSAGE);
			connections.AddInt32("im_what",REGISTER_CONTACTS);
			GetContactsForProtocol( p->GetSignature(), &connections );
			
			p->Process( &connections );
		}
	}
	
	// send it
	Broadcast(msg);
}

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
	
	LOG("STATUS_CHANGED [%s] is now %s",proto_id.c_str(),new_status.c_str());
	
/*	for ( int i=0; i<contact.CountConnections(); i++ )
	{ // calc total status
		char connection[512];
		
		contact.ConnectionAt(i,connection);
		
		if ( proto_id != connection )
		{ // not this connection, check status
			string curr = fStatus[connection];
			
			if ( curr == AWAY_TEXT && new_status == OFFLINE_TEXT )
			{
				new_status = AWAY_TEXT;
			}
			if ( curr == ONLINE_TEXT )
			{
				new_status = ONLINE_TEXT;
			}
		}
	}
	
	// update status
	fStatus[proto_id] = new_status;
*/	
	// update status attribute
	BNode node(contact);
	
	if ( node.InitCheck() != B_OK )
	{
		_ERROR("ERROR: Invalid node when setting new status", msg);
	} else
	{ // node exists, write status
		status = new_status.c_str();
		
		if ( node.WriteAttr(
			"IM:status", B_STRING_TYPE, 0,
			status, strlen(status)+1
		) != (int32)strlen(status)+1 )
		{
			_ERROR("Error writing status attribute",msg);
		}
	}
}

/**
	Query for all files with a IM:status and set it to OFFLINE_TEXT
*/
void
Server::SetAllOffline()
{
	BVolumeRoster 	vroster;
	BVolume			boot_volume;

	vroster.GetBootVolume(&boot_volume);
	
	BQuery query;
	
	query.SetPredicate( "IM:connections=*" );
	query.SetVolume(&boot_volume);
	query.Fetch();
	
	entry_ref entry;
	
	BMessage msg;
	
	while ( query.GetNextRef(&entry) == B_OK )
	{
		msg.AddRef("contact",&entry);
	}
	
	char nickname[512], name[512], filename[512];
	
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
		
//		LOG("Setting %s (%s) offline, filename: %s", name, nickname, filename);
		
		if ( c.SetStatus(OFFLINE_TEXT) != B_OK )
			LOG("  error.");
	}
}

/**
	Add the ID of all contacts that have a connections for this protocol
	to msg.
*/
void
Server::GetContactsForProtocol( const char * protocol, BMessage * msg )
{
	BVolumeRoster 	vroster;
	BVolume			boot_volume;
	
	vroster.GetBootVolume(&boot_volume);
	
	Contact result;
	
	BQuery query;
	
	char pred[256];
	
	sprintf(pred,"IM:connections=*%s*",protocol);
	
	query.SetPredicate( pred );
	query.SetVolume( &boot_volume );
	query.Fetch();
	
	entry_ref entry;
	
	while ( query.GetNextRef(&entry) == B_OK )
	{
		Contact c(&entry);
		char conn[256];
		char * id = NULL;
		
		if ( c.FindConnection(protocol,conn) == B_OK )
		{
			for ( int i=0; conn[i]; i++ )
				if ( conn[i] == ':' )
				{
					id = &conn[i+1];
					break;
				}
			
			msg->AddString("id",id);
		}
	}
}

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
	
	return main_msg;
}

status_t
Server::UpdateOwnSettings( BMessage settings )
{
	return B_OK;
}
