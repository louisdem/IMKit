#include "Contact.h"

#include <string.h>
#include <TypeConstants.h>
#include <stdio.h>

#include "Helpers.h"

using namespace IM;

Contact::Contact()
	: fEntry(-1,-1, "")
{
}

Contact::~Contact()
{
	Clear();
}

Contact::Contact( const entry_ref & entry )
{
	SetTo(entry);
}

Contact::Contact( const BEntry & entry )
{
	SetTo(entry);
}

Contact::Contact( const Contact & contact )
{
	SetTo( contact.fEntry );
}

void
Contact::SetTo( const entry_ref & entry )
{
	fEntry = entry;
	Update();
}

void
Contact::SetTo( const BEntry & entry )
{
	entry.GetRef(&fEntry);
	Update();
}

void
Contact::SetTo( const Contact & contact )
{
	fEntry = contact.fEntry;
	Update();
}

status_t
Contact::InitCheck()
{
	BEntry entry(&fEntry);
	
	return entry.InitCheck();
}

void
Contact::Update()
{
	Clear();
}

/**
	Clear memory caches of any data read from attributes
*/
void
Contact::Clear()
{
	for ( int i=0; i<fConnections.CountItems(); i++ )
		delete (char*)fConnections.ItemAt(i);
	
	fConnections.MakeEmpty();
}

status_t
Contact::LoadConnections()
{
	Clear();
	
	BNode node(&fEntry);
	
	if ( node.InitCheck() != B_OK )
		return B_ERROR;
	
	char attr[256];
	
	int32 num_read = node.ReadAttr(
		"IM:connections", B_STRING_TYPE, 0,
		attr, 255
	);
	
	if ( num_read <= 0 )
		return B_ERROR;
	
	attr[num_read] = 0;
	
	// data read. process it.
	int32 start = 0, curr = 0;
	char * conn = new char[256];
	
	while ( attr[curr] )
	{
		if ( attr[curr] != ';' )
		{
			conn[curr-start] = attr[curr];
		} else
		{ // separator
			if ( curr != start )
			{
				conn[curr-start] = 0;
				fConnections.AddItem(conn);
				start = curr+1;
				conn = new char[256];
			}
		}
		curr++;
	}
	
	if ( start != curr )
	{
		conn[curr-start] = 0;
		fConnections.AddItem(conn);
	} else
	{
		delete conn;
	}
	
	return B_OK;
}

/**
	Write list of connections to disk
*/
status_t
Contact::SaveConnections()
{
	char attr[256];
	attr[0] = 0;
	
	for ( int i=0; i<fConnections.CountItems(); i++ )
	{
		strcat(attr,(char*)fConnections.ItemAt(i));
		strcat(attr,";");
	}
	
	BNode node(&fEntry);
	
	if ( node.InitCheck() != B_OK )
		return B_ERROR;
	
	if ( node.WriteAttr(
		"IM:connections", B_STRING_TYPE, 0,
		attr, strlen(attr)+1
	) != (int32)strlen(attr)+1 )
	{ // error writing
		return B_ERROR;
	}
	
	return B_OK;
}

status_t
Contact::AddConnection( const char * proto_id )
{
	LOG("Contact", liLow, "Adding connection %s\n", proto_id);
	
	if ( fConnections.CountItems() == 0 )
		LoadConnections();
	
	char * copy = new char[strlen(proto_id)+1];
	
	strcpy(copy,proto_id);
	
	fConnections.AddItem(copy);
	
	return SaveConnections();
}

status_t
Contact::RemoveConnection( const char * proto_id )
{
	LOG("Contact", liLow, "Removing connection %s\n", proto_id);
	
	if ( fConnections.CountItems() == 0 )
		LoadConnections();
	
	for ( int i=0; i<fConnections.CountItems(); i++ )
	{
		if ( strcmp(proto_id,(char*)fConnections.ItemAt(i)) == 0 )
		{
			delete[] (char*)fConnections.ItemAt(i);
			fConnections.RemoveItem(i);
			
			return SaveConnections();
		}
	}
	
	return B_ERROR;
}

int
Contact::CountConnections()
{
	if ( fConnections.CountItems() == 0 )
		LoadConnections();
	
	return fConnections.CountItems();
}

status_t
Contact::ConnectionAt( int index, char * buffer )
{
	if ( fConnections.CountItems() == 0 )
		LoadConnections();
	
	if ( index >= CountConnections() )
		return B_ERROR;
	
	strcpy( buffer, (char*)fConnections.ItemAt(index) );
	
	return B_OK;
}

status_t
Contact::FindConnection( const char * protocol, char * buffer )
{
	if ( fConnections.CountItems() == 0 )
		LoadConnections();
	
	char proto[256];
	
	sprintf(proto,"%s:",protocol);
	
	for ( int i=0; i<fConnections.CountItems(); i++ )
	{
		const char * conn = (const char*)fConnections.ItemAt(i);
		
		if ( strstr(conn,proto) )
		{
			strcpy(buffer,conn);
			return B_OK;
		}
	}
	
	return B_ERROR;
}

bool
Contact::operator == ( const entry_ref & entry ) const
{
	return fEntry == entry;
}

bool
Contact::operator == ( const BEntry & entry ) const
{
	entry_ref ref;
	
	entry.GetRef(&ref);
	
	return fEntry == ref;
}

bool
Contact::operator == ( const Contact & contact ) const
{
	return fEntry == contact.fEntry;
}

Contact::operator const entry_ref * () const
{
	return &fEntry;
}

status_t
Contact::SetStatus( const char * status )
{
	BNode node(&fEntry);
	
	if ( node.InitCheck() != B_OK )
		return B_ERROR;
	
	if ( node.WriteAttr(
		"IM:status", B_STRING_TYPE, 0,
		status, strlen(status)+1
	) != (int32)strlen(status)+1 )
	{
		return B_ERROR;
	}
	
	node.SetModificationTime( time(NULL) );
	
	return B_OK;
}

status_t
Contact::ReadAttribute( const char * attr, char * buffer, int bufsize )
{
	BNode node(&fEntry);
	
	if ( node.InitCheck() != B_OK )
		return B_ERROR;
	
	int32 num_read = node.ReadAttr(attr, B_STRING_TYPE, 0, buffer, bufsize );
	
	if ( num_read <= 0 )
		return B_ERROR;
	
	buffer[num_read] = 0;
	
	return B_OK;
}

status_t
Contact::GetName( char * buffer, int size )
{
	return ReadAttribute("META:name",buffer,size);
}

status_t
Contact::GetNickname( char * buffer, int size )
{
	return ReadAttribute("META:nickname",buffer,size);
}

status_t
Contact::GetEmail( char * buffer, int size )
{
	return ReadAttribute("META:name",buffer,size);
}

status_t
Contact::GetStatus( char * buffer, int size )
{
	return ReadAttribute("IM:status",buffer,size);
}
