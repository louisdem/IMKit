#include "IMInfoApp.h"

#include <stdio.h>

IMInfoApp::IMInfoApp(void)
	: BApplication("application/x-vnd.beclan.IM_InfoPopper") {

	fManager = new IM::Manager(this);
	fManager->StartListening();
	
	BMessage settings;
	bool temp;
	im_load_client_settings("IM-InfoPopper", &settings);
	if ( !settings.FindString("app_sig") )
		settings.AddString("app_sig", "application/x-vnd.beclan.IM_InfoPopper");
	if ( settings.FindBool("auto_start", &temp) != B_OK )
		settings.AddBool("auto_start", true );
	if (settings.FindString("status_text", &fStatusText) != B_OK) {
		fStatusText = "$nickname$ ($protocol$:$id$) is now $status$";
		settings.AddString("status_text", fStatusText);
	};
	if (settings.FindString("msg_text", &fMessageText) != B_OK) {
		fMessageText = "$nickname$ says:\n$shortmsg$";
		settings.AddString("msg_text", fMessageText);
	};
	
	im_save_client_settings("IM-InfoPopper", &settings);
	
	BDirectory dir("/boot/home/config/add-ons/im_kit/protocols");
	entry_ref ref;
	
	dir.Rewind();
	
	while (dir.GetNextRef(&ref) == B_OK) {
		BPath path(&ref);
		BBitmap *icon = ReadNodeIcon(path.Path(), 32);
	
		fProtocolIcons[path.Leaf()] = icon;
	};	
};

IMInfoApp::~IMInfoApp(void) {
	fManager->StopListening();
	delete fManager;
};

void IMInfoApp::MessageReceived(BMessage *msg) {
	switch (msg->what) {
		case IM::ERROR:
		case IM::MESSAGE: {
			int32 im_what = IM::ERROR;
			if (msg->FindInt32( "im_what", &im_what) != B_OK) im_what = IM::ERROR;
			
			msg->PrintToStream();
			
			BString text("");		
			entry_ref ref;
			
			msg->FindRef("contact", &ref);
			
			const char *protocol;
			char contactname[512];
			char nickname[512];
			char email[512];
			char status[512];
			BBitmap *icon = NULL;
			
			if (msg->FindString("protocol", &protocol) == B_OK) {		
				protoicons::iterator pIt = fProtocolIcons.find(protocol);
				if (pIt != fProtocolIcons.end()) icon = pIt->second;
			};
			
			IM::Contact contact(&ref);
			
			if (contact.GetName(contactname, sizeof(contactname) ) != B_OK ) {
				strcpy(contactname, "<unknown contact>");
			};
			if (contact.GetEmail(email, sizeof(email)) != B_OK) {
				strcpy(email, "<unknown email>");
			};
			if (contact.GetNickname(nickname, sizeof(nickname)) != B_OK) {
				strcpy(nickname, "<unknown nick>");
			};
			if (contact.GetNickname(status, sizeof(status)) != B_OK) {
				strcpy(status, "<unknown status>");
			};
			
//			Exit if user is blocked
			if ( strcasecmp(status, "blocked") == 0 ) break;

			InfoPopper::info_type type = InfoPopper::Information;

			switch (im_what) {
				case IM::ERROR:
				{
					BMessage error;
					int32 error_what = -1;
					if ( msg->FindMessage("message", &error ) == B_OK )
					{
						error.FindInt32("im_what", &error_what);
					}
					
					if ( error_what != IM::USER_STARTED_TYPING && 
						error_what != IM::USER_STOPPED_TYPING )
					{ // we ignore errors due to typing notifications.
						text << "Error: " << msg->FindString("error");
						type = InfoPopper::Error;
					}
				}	break;
				
				case IM::MESSAGE_RECEIVED: {
					text = fMessageText;
					BString message = msg->FindString("message");
					BString shortMessage = message;
					
					if ( shortMessage.FindFirst("\n") >= 0 )
					{
						shortMessage.Truncate( shortMessage.FindFirst("\n") );
						shortMessage.Append("...");
					}
					
					if ( shortMessage.Length() > 30 ) {
						shortMessage.Truncate(27);
						shortMessage.Append("...");
					}
					
					text.ReplaceAll("$shortmsg$", shortMessage.String());
					text.ReplaceAll("$message$", message.String());
				
					type = InfoPopper::Important;
				}	break;
				
				case IM::STATUS_CHANGED: {
					const char * new_status = msg->FindString("status");
					const char * old_status = msg->FindString("old_status");
					
					if ( new_status && old_status && strcmp(new_status,old_status) )
					{ // only show if total status has changed
						text = fStatusText;
						
						text.ReplaceAll("$status$", msg->FindString("status"));
					}
				}	break;
				
//				case IM::PROGRESS: {
//					const char * progID = msg->FindString("progressID");
//					
//					if ( !progID )
//						break;
//					
//					InfoView * view = NULL;
//					
//					for ( list<InfoView*>::iterator i=fInfoViews.begin(); i!=fInfoViews.end(); i++ )
//					{
//						if ( (*i)->HasProgressID(progID) )
//							view = *i;
//					}
//					
//					if ( view )
//					{
//						view->MessageReceived(msg);
//						
//						ResizeAll();
//					} else 
//					{
//						if ( !msg->FindString("message") )
//							break;
//						
//						float progress = 0.0;
//						
//						if ( msg->FindFloat("progress", &progress) != B_OK )
//							break;
//						
////						view = new InfoView( 
////							InfoPopper::Progress, 
////							msg->FindString("message"),
////							progID,
////							progress
////						);
////						
////						fInfoViews.push_back( view );
////						
////						fBorder->AddChild( view );
////						
////						ResizeAll();
//					}
//				}	return; // Yes, return here. Progress is a special case.
			};
			
			text.ReplaceAll("\\n", "\n");
			text.ReplaceAll("$nickname$", nickname);
			text.ReplaceAll("$contactname$", contactname);
			text.ReplaceAll("$email$", email);
			text.ReplaceAll("$id$", msg->FindString("id"));
			text.ReplaceAll("$protocol$", msg->FindString("protocol"));
			
//			Message to display
			if ( text != "" ) {
				printf("Displaying message <%s>\n", text.String() );
				
				BMessage msg(InfoPopper::AddMessage);
				msg.AddString("title", "IM Kit");
				msg.AddString("content", text);
				msg.AddInt8("type", (int8)type);
	
				if (icon) {
					BMessage image;
					icon->Archive(&image);
					msg.AddMessage("icon", &image);
				};
	
				msg.AddString("onClickApp", "application/x-person");
				msg.AddRef("onClickRef", &ref);

				BMessenger(InfoPopperAppSig).SendMessage(msg);				
			};
		} break;
		
		case IM::SETTINGS_UPDATED: {	
			BMessage settings;
			im_load_client_settings("InfoPopper", &settings);
			if (settings.FindString("status_text", &fStatusText) != B_OK) {
				fStatusText = "$nickname$ is now $status$";
			};
			if (settings.FindString("msg_text", &fMessageText) != B_OK) {
				fMessageText = "$nickname$ says:\n$shortmsg$";
			};			
		} break;
		
		default: {
			BApplication::MessageReceived(msg);
		};
	};
};
