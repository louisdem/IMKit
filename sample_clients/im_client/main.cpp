#include "main.h"

void
setAttributeIfNotPresent( entry_ref ref, const char * attr, const char * value )
{
//	printf("Setting attribute %s to %s\n", attr, value );
	
	BNode node(&ref);
	char data[512];
	
	if ( node.InitCheck() != B_OK )
	{
		LOG("sample_client", LOW, "Invalid entry_ref in setAttributeIfNotSet");
		return;
	}
	
	if ( node.ReadAttr(attr,B_STRING_TYPE,0,data,sizeof(data)) > 1 )
	{
//		LOG("  value already present");
		return;
	}
	
	int32 num_written = node.WriteAttr(
		attr, B_STRING_TYPE, 0,
		value, strlen(value)+1
	);
	
	if ( num_written != (int32)strlen(value) + 1 )
	{
		LOG("sample_client", MEDIUM, "Error writing attribute %s (%s)\n",attr,value);
	} else
	{
		//LOG("Attribute set");
	}
}

int main(void)
{
	ChatApp app;
	
	app.Run();
	
	return 0;
}
