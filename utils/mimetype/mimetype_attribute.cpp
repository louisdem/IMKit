#include <Mime.h>
#include <Message.h>
#include <String.h>

#include <be/support/SupportDefs.h>
#include <stdio.h>

typedef struct value_type_t {
	BString 	name;
	type_code	type;
};

const value_type_t valid_types[] = {
	{"string", B_STRING_TYPE},
	{"boolean", B_BOOL_TYPE},
	{"int32", B_INT32_TYPE},
	{"float", B_FLOAT_TYPE},
	
	// end of list thingie
	{"", B_ANY_TYPE}
};

void print_usage()
{
	printf("usage: mimetype_attribute <options>\n\n");
	printf("Required options:\n");
	printf("   --mime <MIME type to edit, e.g. application/x-person>\n");
	printf("   --internal <internal name, e.g. IM:connections>\n");
	printf("   --public <public name, e.g. 'IM Connections'>\n");
	printf("   --type <attr type, one of [string, bool, int32, float]>\n");
	printf("Not required options:\n");
	printf("   --editable, --not-editable, sets attribute as user editable or not, editable by default.\n");
	printf("   --public, --private, sets attribute as public or private, public by default.\n");
}

int main( int numarg, const char * argv[] )
{
	BString mimeType;
	BString attributeInternal;
	BString attributePublic;
	BString attributeType;
	bool isEditable = true;
	bool isPublic = true;

	// decode arguments
	for ( int i=1; i<numarg; i++ )
	{
		if ( strcasecmp(argv[i], "--mime") == 0 )
		{ // mime type
			if ( i != numarg - 1 )
			{
				mimeType = argv[++i];
			}
		}

		if ( strcasecmp(argv[i], "--internal") == 0 )
		{ // internal name
			if ( i != numarg - 1 )
			{
				attributeInternal = argv[++i];
			}
		}

		if ( strcasecmp(argv[i], "--public") == 0 )
		{ // public name
			if ( i != numarg - 1 )
			{
				attributePublic = argv[++i];
			}
		}

		if ( strcasecmp(argv[i], "--type") == 0 )
		{ // type
			if ( i != numarg - 1 )
			{
				attributeType = argv[++i];
			}
		}

		if (strcasecmp(argv[i], "--not-editable") == 0) isEditable = false;
		if (strcasecmp(argv[i], "--editable") == 0) isEditable = true;
	
		if (strcasecmp(argv[i], "--public") == 0) isPublic = true;
		if (strcasecmp(argv[i], "--private") == 0) isPublic = false;
	}

	// check arguments
	if ( mimeType == "" || attributeInternal == "" || attributePublic == ""	)
	{
		print_usage();
		return 1;
	}
	
	type_code attributeTypeConst = B_ANY_TYPE;
	for ( int i=0; valid_types[i].name != ""; i++ )
	{
		if ( attributeType == valid_types[i].name )
			attributeTypeConst = valid_types[i].type;
	}
	
	if ( attributeTypeConst == B_ANY_TYPE )
	{
		print_usage();
		return 1;
	}

	// args ok, fetch current attributes
	BMimeType mime( mimeType.String() );
	
	if ( mime.InitCheck() != B_OK )
	{
		printf("Invalid MIME type\n");
		return 2;
	}

	BMessage msg;
	
	if ( mime.GetAttrInfo(&msg) != B_OK )
	{
		printf("Error getting current attributes.\n");
		return 3;
	}
	
	// check if already set

	const char * name = NULL;

	int32 index=0;

	for ( ; msg.FindString("attr:name", index, &name) == B_OK; index++ )
	{
		if ( attributeInternal == name )
		{
			printf("Attribute already set.\n");
			return 4;
		}
	}
	
	// Ok, let's add it.
	msg.AddString("attr:name", attributeInternal.String() );
	msg.AddString("attr:public", attributePublic.String() );
	msg.AddInt32("attr:type", attributeTypeConst );
	msg.AddBool("attr:public", isPublic );
	msg.AddBool("attr:editable", isEditable );

	if ( mime.SetAttrInfo( &msg ) != B_OK )
	{
		printf("Error setting attributes.\n");
		return 5;
	}

	return 0;
}
