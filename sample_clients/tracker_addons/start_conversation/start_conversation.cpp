#include <Node.h>
#include <NodeInfo.h>
#include <Message.h>
#include <Messenger.h>
#include <TrackerAddOn.h>

void
process_refs(entry_ref dir, BMessage * msg, void * )
{
	entry_ref ref;
	
	for ( int i=0; msg->FindRef("refs",i,&ref)==B_OK; i++ )
	{
		BNode node(&ref);
		BNodeInfo info(&node);
		
		char type[512];
		
		if ( info.GetType(type) != B_OK )
			continue;
		
		if ( strcmp(type,"application/x-person") != 0 )
			continue;
		
		BMessage msg('newc');
		msg.AddRef("contact",&ref);
		
		BMessenger msgr("application/x-vnd.m_eiman.sample_im_client");
		msgr.SendMessage(&msg);
	}
}
