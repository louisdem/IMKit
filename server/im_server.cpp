#include "im_server.h"

#include <libim/Constants.h>
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

using namespace IM;

#define AUTOSTART_APPSIG_SETTING "autostart_appsig"

void
_ERROR( const char * error, BMessage * msg )
{
	LOG("im_server", LOW, error, msg);
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
		LOG("im_server", LOW, "ERROR: %s",text);
	}
}

/**
	Default constructor. (Starts People-query and ?) loads add-ons
*/
Server::Server()
:	BApplication(IM_SERVER_SIG)
{
	BPath prefsPath;
	
	// Create our settings directories
	if (find_directory(B_USER_SETTINGS_DIRECTORY,&prefsPath,true,NULL) == B_OK)
	{
		BDirectory dir(prefsPath.Path());
		if (dir.InitCheck() == B_OK)
		{
			dir.CreateDirectory("im_kit", &dir);
			dir.CreateDirectory("im_kit/icons", &dir);
			dir.CreateDirectory("im_kit/add-ons", &dir);
			dir.CreateDirectory("im_kit/add-ons/protocols", &dir);
		}
	}
	
	LoadAddons();
	
	// add deskbar icon
	BDeskbar db;

	if ( db.RemoveItem( DESKBAR_ICON_NAME ) != B_OK )
		LOG("im_server", DEBUG, "Error removing deskbar icon (this is ok..)");
	
	IM_DeskbarIcon * i = new IM_DeskbarIcon();
	
	if ( db.AddItem( i ) != B_OK )
	{ // couldn't add BView, try entry_ref
		entry_ref ref;
		
		if ( be_roster->FindApp(IM_SERVER_SIG,&ref) == B_OK )
		{
			if ( db.AddItem( &ref ) != B_OK )
				LOG("im_server", LOW, "Error adding icon to deskbar!");
		} else
		{
			LOG("im_server", LOW, "be_roster->FindApp() failed");
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

// Loads attribute named 'attribute' from the file 'name'defaults to a type of 'BBMP'
// Doesn't work on symlinks

BBitmap *
Server::GetBitmapFromAttribute(const char *name, const char *attribute, 
	type_code type = 'BBMP') {
	BBitmap 	*bitmap = NULL;
	size_t 		len = 0;
	status_t 	error;	

	if ((name == NULL) || (attribute == NULL)) return NULL;

	BNode node(name);
	
	if (node.InitCheck() != B_OK) {
		return NULL;
	};
	
	attr_info info;
		
	if (node.GetAttrInfo(attribute, &info) != B_OK) {
		node.Unset();
		return NULL;
	};
		
	char *data = (char *)calloc(info.size, sizeof(char));
	len = (size_t)info.size;
		
	if (node.ReadAttr(attribute, 'BBMP', 0, data, len) != len) {
		node.Unset();
		free(data);
	
		return NULL;
	};
	
//	Icon is a square, so it's right / bottom co-ords are the root of the bitmap length
//	Offset is 0
	BRect bound = BRect(0, 0, 0, 0);
	bound.right = sqrt(len) - 1;
	bound.bottom = bound.right;
	
	bitmap = new BBitmap(bound, B_COLOR_8_BIT);
	bitmap->SetBits(data, len, 0, B_COLOR_8_BIT);

//	make sure it's ok
	if(bitmap->InitCheck() != B_OK) {
		free(data);
		delete bitmap;
		return NULL;
	};
	
	return bitmap;
}

BBitmap *
Server::GetBitmap(const char *name, type_code type = 'BBMP') {
	BResources *res = AppResources();

	BBitmap 	*bitmap = NULL;
	size_t 		len = 0;
	status_t 	error;	

	// load resource
	const void *data = res->LoadResource(type, name, &len);
	
	BMemoryIO stream(data, len);
	
	// unflatten it
	BMessage archive;
	error = archive.Unflatten(&stream);
	if (error != B_OK)
		return NULL;

	// make a bbitmap from it
	bitmap = new BBitmap(&archive);
	if(!bitmap)
		return NULL;

	// make sure it's ok
	if(bitmap->InitCheck() != B_OK)
	{
		delete bitmap;
		return NULL;
	}
	
	return bitmap;
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
		
		case GET_CONTACT_STATUS:
			reply_GET_CONTACT_STATUS(msg);
			break;
		case GET_OWN_STATUSES: {
			reply_GET_OWN_STATUSES(msg);
		} break;
		
		case FLASH_DESKBAR:
		case STOP_FLASHING:
		case REGISTER_DESKBAR_MESSENGER:
			handleDeskbarMessage(msg);
			break;
		
		case ADD_AUTOSTART_APPSIG:
			reply_ADD_AUTOSTART_APPSIG(msg);
			break;
		
		case REMOVE_AUTOSTART_APPSIG:
			reply_REMOVE_AUTOSTART_APPSIG(msg);
			break;
		
		case SERVER_BASED_CONTACT_LIST:
			reply_SERVER_BASED_CONTACT_LIST(msg);
			break;
		
		case GET_SETTINGS_TEMPLATE:
			reply_GET_SETTINGS_TEMPLATE(msg);
			break;
		
		case GET_SETTINGS:
			reply_GET_SETTINGS(msg);
			break;
		
		case SET_SETTINGS:
			reply_SET_SETTINGS(msg);
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
		LOG("im_server", HIGH, "cannot access protocol settings directory: %s, error 0x%lx (%s)!", path.Path(), rc, strerror(rc));
		return rc;
	}
	
	// STEP 2: Check if we can access the add-on directory for the protocols!
	if ((rc=find_directory(B_USER_ADDONS_DIRECTORY, &path, true)) != B_OK ||
		(rc=path.Append("im_kit/protocols")) != B_OK ||
		(rc=addonsDir.SetTo(path.Path())) != B_OK)
	{ // we couldn't access the addons directory for the protocols!
		LOG("im_server", HIGH, "cannot access protocol addon directory: %s, error 0x%lx (%s)!", path.Path(), rc, strerror(rc));
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
			LOG("im_server", LOW, "load_add_on() fail, file [%s]", path.Path());
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
			LOG("im_server", MEDIUM, "get_image_symbol(load_protocol) fail, file [%s]", path.Path());
			unload_add_on( curr_image );
			continue;
		}
		
		Protocol * protocol = load_protocol();
		
		if ( !protocol )
		{
			LOG("im_server", LOW, "load_protocol() fail, file [%s]", path.Path());
			unload_add_on( curr_image );
			continue;
		}
		
		LOG("im_server", MEDIUM, "Protocol loaded: [%s]", protocol->GetSignature());
		
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

				LOG("im_server", HIGH, "Read settings data");
				
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
			LOG("im_server", HIGH, "Error initializing protocol '%s' (error 0x%ld/%s)!", protocol->GetSignature(), rc, strerror(rc));
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
		}
	} // while()
	
	LOG("im_server", MEDIUM, "All add-ons loaded.");
	
	return B_OK;
}

/**
	Unloads add-on images after shutting the down.
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
	LOG("im_server", DEBUG, "Endpoint added");
	fMessengers.push_back(msgr);
}

/**
*/
void
Server::RemoveEndpoint( BMessenger msgr )
{
	LOG("im_server", DEBUG, "Endpoint removed");
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
	LOG("im_server", LOW, "Creating new contact for connection [%s]", proto_id);
	
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
		LOG("im_server", LOW, "Error: While creating a new contact, dir.FindEntry() failed. filename was [%s]",filename);
		return result;
	}
	
	LOG("im_server", HIGH, "  created file [%s]", filename);
	
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
	
	LOG("im_server", DEBUG, "  wrote type");
	
	// file created. set type and add connection
	result.SetTo( entry );
	
	if ( result.AddConnection(proto_id) != B_OK )
	{
		return Contact();
	}
	
	LOG("im_server", DEBUG, "  wrote connection");
	
	if ( result.SetStatus(OFFLINE_TEXT) != B_OK )
	{
		return Contact();
	}
	
	LOG("im_server", DEBUG, "  wrote status");
	
	// post request info about this contact
	BMessage msg(MESSAGE);
	msg.AddInt32("im_what", GET_CONTACT_INFO);
	msg.AddRef("contact", result);
	
	PostMessage(&msg);

	LOG("im_server", DEBUG, "  done.");
	
	return result;
}

void
Server::reply_GET_SETTINGS_TEMPLATE( BMessage * msg )
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

status_t
Server::GetSettings( const char * protocol_sig, BMessage * settings )
{
	char settings_path[512];
	
	// get path to settings file
	if ( !protocol_sig )
	{ // im_server settings
		strcpy(settings_path,"/boot/home/config/settings/im_kit/im_server.settings");
	} else
	{ // protocol settings
		if ( fProtocols.find(protocol_sig) == fProtocols.end() )
		{
			_ERROR("ERROR: GET_SETTINGS: Protocol not loaded");
			return B_ERROR;
		}
		
		Protocol * protocol = fProtocols[protocol_sig];
		
		sprintf(
			settings_path,
			"/boot/home/config/settings/im_kit/add-ons/protocols/%s",
			fAddOnInfo[protocol].signature
		);
	}
	
	char data[1024*1024];
	
	BNode node( settings_path );
	
	int32 num_read = node.ReadAttr(
		"im_settings", B_RAW_TYPE, 0,
		data, sizeof(data)
	);
	
	if ( num_read <= 0 )
	{
		_ERROR("ERROR: GET_SETTINGS: Error reading settings");
		return B_ERROR;
	}
	
	if ( settings->Unflatten(data) != B_OK )
	{
		_ERROR("ERROR: GET_SETTINGS: Error unflattening settings");
		return B_ERROR;
	}
	
	LOG("im_server", DEBUG, "Read settings from file: %s", settings_path);
	
	return B_OK;
}

status_t
Server::SetSettings( const char * protocol_sig, BMessage * settings )
{
	status_t res = B_ERROR;
	char settings_path[512];
		
	if ( settings->what != SETTINGS )
	{
		_ERROR("ERROR: SET_SETTINGS: Malformed settings message");
		return B_ERROR;
	}
			
	// check settings and get path to settings file
	if ( !protocol_sig )
	{ // im_server settings
		res = UpdateOwnSettings(*settings);
		
		strcpy(settings_path,"/boot/home/config/settings/im_kit/im_server.settings");
	} else
	{ // protocol settings
		if ( fProtocols.find(protocol_sig) == fProtocols.end() )
		{
			_ERROR("ERROR: GET_SETTINGS: Protocol not loaded");
			return B_ERROR;
		}
			
		Protocol * protocol = fProtocols[protocol_sig];
			
		res = protocol->UpdateSettings(*settings);
			
		sprintf(
			settings_path,
			"/boot/home/config/settings/im_kit/add-ons/protocols/%s",
			fAddOnInfo[protocol].signature
		);
	}
	
	// check if settings are ok
	if ( res != B_OK )
	{
		_ERROR("ERROR: SET_SETTINGS: Protocol says settings not valid");
		return B_ERROR;
	}
	
	// save settings
	char data[1024*1024];
	int32 data_size=settings->FlattenedSize();
	
	if ( settings->Flatten(data,data_size) != B_OK )
	{ // error flattening message
		_ERROR("ERROR: SET_SETTINGS: Error flattening settings message");
		return B_ERROR;
	}
	
	BDirectory dir;
	dir.CreateFile(settings_path,NULL,true);

	BNode node( settings_path );
			
	if ( node.InitCheck() != B_OK )
	{
		_ERROR("ERROR: SET_SETTINGS: Error opening save file");
		return B_ERROR;
	}
			
	int32 num_written = node.WriteAttr(
		"im_settings", B_RAW_TYPE, 0,
		data, data_size
	);
			
	if ( num_written != data_size )
	{ // error saving settings
		_ERROR("ERROR: SET_SETTINGS: Error saving settings");
		return B_ERROR;
	}

	LOG("im_server", DEBUG, "Wrote settings to file: %s", settings_path);
	
	return B_OK;			
}

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
				return protocol;
			}
		}
	}
	
	// look for an online protocol
	for ( int i=0; contact.ConnectionAt(i,connection) == B_OK; i++ )
	{
		string curr = connection;
		
		if ( fStatus[curr] == AWAY_TEXT || fStatus[curr] == ONLINE_TEXT )
		{
			int separator_pos = curr.find(":");
		
			curr.erase(separator_pos, strlen(connection)-separator_pos);
		
			protocol = curr;
		}
	}
	
	if ( protocol == "" )
	{ // no online protocol found, look for one capable of offline messaging
		protocol = "ICQ";
	}
	
	return protocol;
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
			/*char connection[255];
			
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
			*/
			msg->AddString("protocol", FindBestProtocol(contact).c_str() );
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
		LOG("im_server", HIGH, "No protocol specified");
		
		int32 im_what=-1;
		msg->FindInt32("im_what", &im_what);
		
		switch ( im_what )
		{ // send these messages to all loaded protocols
			case SET_STATUS:
			{
				LOG("im_server", HIGH, "  SET_STATUS - Sending to all protocols");
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
			
			if ( strcmp(status,"Blocked") == 0 )
			{ // contact blocked, dropping message!
				LOG("im_server", LOW, "Dropping message from blocked contact [%s:%s]", protocol, id);
				return;
			}
			
			fPreferredProtocol[contact] = protocol;
			LOG("im_server", DEBUG, "Setting preferred protocol for [%s:%s] to %s", protocol, id, protocol );
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
		LOG("im_server", DEBUG, "Creating new contact on authorization. ID : %s", id);
	} else {
		// send it
		Broadcast(msg);
	}
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
	
	LOG("im_server", MEDIUM, "STATUS_CHANGED [%s] is now %s",proto_id.c_str(),new_status.c_str());
	
	// update status
	fStatus[proto_id] = new_status;
	
	// calculate total status for contact
	new_status = OFFLINE_TEXT;
	
	UpdateContactStatusAttribute(contact);
}


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
	
	// update status attribute
	BNode node(contact);
	
	if ( node.InitCheck() != B_OK )
	{
		_ERROR("ERROR: Invalid node when setting new status");
	} else
	{ // node exists, write status
		const char * status = new_status.c_str();
		
		if ( node.WriteAttr(
			"IM:status", B_STRING_TYPE, 0,
			status, strlen(status)+1
		) != (int32)strlen(status)+1 )
		{
			_ERROR("Error writing status attribute");
		}
		
		BBitmap *large = NULL;
		BBitmap *small = NULL;
		
		BString pointerName = status;
		pointerName << "_small";

		fIcons.FindPointer(pointerName.String(), reinterpret_cast<void **>(&small));
		
		pointerName = status;
		pointerName << "_large";
		
		fIcons.FindPointer(pointerName.String(), reinterpret_cast<void **>(&large));
		
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
			if ( strcmp(status, "Blocked") == 0 )
			{
				LOG("im_server", DEBUG, "Skipping contact %s (%s), filename: %s", name, nickname, filename);
				continue;
			}
		}
		
		LOG("im_server", DEBUG, "Setting %s (%s) offline, filename: %s", name, nickname, filename);
		
		if ( c.SetStatus(OFFLINE_TEXT) != B_OK )
			LOG("im_server", DEBUG, "  error.");

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
		if ((vol.InitCheck() == B_OK) && (vol.KnowsQuery() == true)) {
			vol.GetName(volName);
			LOG("im_server", LOW, "Querying for Contacts on %s", volName);
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
	};
	
	refs.sort();
	refs.unique();
	
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

	refs.clear();
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
	
	BMessage auto_start;
	auto_start.AddString("name", "auto_start");
	auto_start.AddString("description", "Auto-start im_server");
	auto_start.AddInt32("type", B_BOOL_TYPE );
	auto_start.AddBool("default", true );
	
	main_msg.AddMessage("setting", &auto_start);
	
	return main_msg;
}

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

void
Server::StartAutostartApps()
{
	BMessage settings;
	
	GetSettings( NULL, &settings );
	
	for ( int i=0; settings.FindString(AUTOSTART_APPSIG_SETTING,i); i++ )
	{
		LOG("im_server", LOW, "Starting app [%s]", settings.FindString(AUTOSTART_APPSIG_SETTING,i) );
		be_roster->Launch( settings.FindString(AUTOSTART_APPSIG_SETTING,i) );
	}
}

void
Server::StopAutostartApps()
{
	BMessage settings;
	
	GetSettings( NULL, &settings );
	
	for ( int i=0; settings.FindString(AUTOSTART_APPSIG_SETTING,i); i++ )
	{
		LOG("im_server", LOW, "Stopping app [%s]", settings.FindString(AUTOSTART_APPSIG_SETTING,i) );
		BMessenger msgr( settings.FindString(AUTOSTART_APPSIG_SETTING,i) );
		msgr.SendMessage( B_QUIT_REQUESTED );
	}
}

void
Server::reply_ADD_AUTOSTART_APPSIG( BMessage * msg )
{
	if ( msg->FindString("app_sig") == NULL )
	{
		_SEND_ERROR("No app_sig provided", msg);
		return;
	}
	
	const char * new_appsig = msg->FindString("app_sig");
	
	BMessage settings;
	if ( GetSettings(NULL, &settings) != B_OK )
	{
		_SEND_ERROR("Error reading settings",msg);
		return;
	}
	
	for ( int i=0; settings.FindString(AUTOSTART_APPSIG_SETTING, i); i++ )
	{
		if ( strcmp( new_appsig, settings.FindString(AUTOSTART_APPSIG_SETTING,i) ) == 0 )
		{ // app-sig already present, don't add again
			msg->SendReply(ACTION_PERFORMED);
			LOG("im_server", HIGH, "Auto-start app already present [%s]", new_appsig);
			return;
		}
	}
	
	settings.AddString(AUTOSTART_APPSIG_SETTING, new_appsig);
	
	SetSettings( NULL, &settings );
	
	msg->SendReply(ACTION_PERFORMED);
	
	LOG("im_server", HIGH, "Auto-start app added [%s]", new_appsig);
}

void
Server::reply_REMOVE_AUTOSTART_APPSIG( BMessage * msg )
{
	if ( msg->FindString("app_sig") == NULL )
		_SEND_ERROR("No app_sig provided", msg);
		return;
			
	const char * appsig = msg->FindString("app_sig");
			
	BMessage settings;
	BMessage temp;
	GetSettings(NULL, &settings);
	
	// save app-sigs not to be deleted in temp
	for ( int i=0; settings.FindString(AUTOSTART_APPSIG_SETTING, i); i++ )
	{
		if ( strcmp( appsig, settings.FindString(AUTOSTART_APPSIG_SETTING,i) ) != 0 )
		{ // this is not the app-sig to delete, store it
			temp.AddString(AUTOSTART_APPSIG_SETTING, settings.FindString(AUTOSTART_APPSIG_SETTING,i));
		}
	}
	
	// delete all app-sigs from settings
	settings.RemoveName(AUTOSTART_APPSIG_SETTING);
	
	// copy app-sigs from temp
	for ( int i=0; temp.FindString(AUTOSTART_APPSIG_SETTING,i); i++ )
		settings.AddString(AUTOSTART_APPSIG_SETTING, temp.FindString(AUTOSTART_APPSIG_SETTING,i));
	
	SetSettings( NULL, &settings );

	msg->SendReply(ACTION_PERFORMED);
	
	LOG("im_server", HIGH, "Auto-start app removed [%s]", appsig);
}

void
Server::reply_GET_SETTINGS( BMessage * msg )
{
	const char * protocol = msg->FindString("protocol");
	
	if ( protocol[0] == 0 )
		protocol = NULL;
	
	BMessage settings;
	
	if ( GetSettings( protocol, &settings ) != B_OK )
	{
		_SEND_ERROR("Error getting settings", msg);
	}
	
	msg->SendReply(&settings);
}

void
Server::reply_SET_SETTINGS( BMessage * msg )
{
	const char * protocol = msg->FindString("protocol");
	
	if ( protocol[0] == 0 )
		protocol = NULL;
	
	BMessage settings;
	
	if ( msg->FindMessage("settings", &settings) != B_OK )
	{
		_SEND_ERROR("No settings provided", msg);
		return;
	}
	
	if ( SetSettings( protocol, &settings ) != B_OK )
	{
		_SEND_ERROR("Error setting settings", msg);
	}
	
	msg->SendReply(ACTION_PERFORMED);
}

void
Server::handleDeskbarMessage( BMessage * msg )
{
	switch ( msg->what )
	{
		case REGISTER_DESKBAR_MESSENGER:
			LOG("im_server", DEBUG, "Got Deskbar messenger");
			msg->FindMessenger("msgr", &fDeskbarMsgr);
			break;
		
		default:
			LOG("im_server", DEBUG, "Forwarding message to Deskbar");
			if ( fDeskbarMsgr.SendMessage(msg) != B_OK )
			{
				LOG("im_server", LOW, "Error sending message to Deskbar");
			}
			break;
	}
}

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
		connections.AddInt32("im_what",REGISTER_CONTACTS);
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
			BMessage update(MESSAGE);
			update.AddInt32("im_what", STATUS_CHANGED);
			update.AddString("protocol", protocol);
			update.AddString("id", contacts.FindString("id",i) );
			update.AddString("status", OFFLINE_TEXT);
			
			PostMessage( &update );
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
	LOG("im_server", HIGH, "Total status changed to %s", total_status.c_str() );
	// end 'Find out total status'
	
	handleDeskbarMessage(msg);
}

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

void Server::reply_GET_OWN_STATUSES(BMessage *msg) {
	LOG("im_server", LOW, "Got own status request. There are %i statuses",
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
