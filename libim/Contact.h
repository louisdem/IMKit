#ifndef IM_CONTACT_H
#define IM_CONTACT_H

#include <Entry.h>
#include <List.h>

namespace IM {

class Contact
{
	public:
		Contact();
		Contact( const entry_ref & );
		Contact( const BEntry & );
//		Contact( const node_ref & );
		Contact( const Contact & );
		~Contact();
		
		// 
		void SetTo( const entry_ref &);
		void SetTo( const BEntry &);
//		void SetTo( const node_ref &);
		void SetTo( const Contact &);
		
		// returns B_OK if the Contact is connected to a valis People-file
		status_t InitCheck();
		
		// call this function to reload information from attributes
		void Update();
		
		// Connection management
		status_t AddConnection( const char * proto_id );
		status_t RemoveConnection( const char * proto_id );
		int CountConnections();
		status_t ConnectionAt( int index, char * );
		status_t FindConnection( const char * protocol, char * );
		
		// Various data
		status_t	SetStatus( const char * );
		status_t	GetName( char * buf, int size );
		status_t	GetNickname( char * buf, int size );
		status_t	GetEmail( char * buf, int size );
		status_t	GetStatus( char * bug, int size );
		
		// comparison, both with Contacts and entry_refs and node_refs
		// representing Contacts
		bool operator == ( const entry_ref & ) const;
		bool operator == ( const BEntry & ) const;
		bool operator == ( const Contact & ) const;
		bool operator < ( const Contact & ) const;
		
		// for easy addition to BMessages
		operator const entry_ref * () const;
//		const node_ref operator Contact();
		
	private:
		status_t	LoadConnections();
		status_t	SaveConnections();
		status_t	ReadAttribute( const char * attr, char * buffer, int bufsize );
		void		Clear();
		
		entry_ref	fEntry;
		BList		fConnections;
		
};

};

#endif
