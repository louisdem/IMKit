#include <Mime.h>
#include <TextControl.h>
#include <Box.h>
#include <String.h>
#include <Path.h>
#include <NodeInfo.h>
#include <Application.h>

#include "PeopleWindow.h"

PeopleWindow::PeopleWindow(entry_ref *ref) : BWindow(BRect(300,400,500,520),(ref == NULL) ? "New Person" : ref->name,B_TITLED_WINDOW,B_NOT_ZOOMABLE | B_NOT_V_RESIZABLE | B_ASYNCHRONOUS_CONTROLS,B_CURRENT_WORKSPACE) {
	if (ref != NULL)
		node.SetTo(ref);
		
	BRect rect(Bounds());
	rect.bottom++; rect.right++;
	
	BBox *gray = new BBox(rect,"person",B_FOLLOW_ALL_SIDES);
	
	BMimeType people_type("application/x-person");
	BMessage attributes;
	people_type.GetAttrInfo(&attributes);
	const char *attr_name, *human;
	BString concat, value;
	rect.Set(10,10,190,25);
	for (int32 i = 0; attributes.FindString("attr:name",i,&attr_name) == B_OK; i++) {
		/*if (strlen(attr_name) < 5 || strncmp(attr_name,"META:",5) != 0)
			continue;*/
			
		attributes.FindString("attr:public_name",i,&human);
		value = "";
		node.ReadAttrString(attr_name,&value);
		concat = human;
		concat << ":  ";
		BTextControl *attr = new BTextControl(rect,attr_name,concat.String(),value.String(),NULL,B_FOLLOW_ALL_SIDES);
		attr->SetDivider(be_plain_font->StringWidth(concat.String()));
		attr->SetEnabled(attributes.FindBool("attr:editable",i));
		gray->AddChild(attr);
		rect.top += 25;
		rect.bottom += 25;
	}
	gray->ResizeTo(gray->Bounds().Width(),rect.bottom - 10);
	ResizeTo(gray->Bounds().Width(),gray->Bounds().Height());
	AddChild(gray);
}

bool PeopleWindow::QuitRequested() {
	BTextControl *next;
	BString text;
	
	if (node.InitCheck() != B_OK) {
		BPath path("/boot/home/people");
		const char *leaf = ((BTextControl *)(ChildAt(0)->FindView("META:name")))->Text();
		path.Append(leaf);
		if (leaf[0] == 0) {
			if (be_app->CountWindows() == 1 /* us */)
				be_app->PostMessage(B_QUIT_REQUESTED);
			return true;
		}
		BFile(path.Path(),B_CREATE_FILE);
		node.SetTo(path.Path());
		BNodeInfo(&node).SetType("application/x-person");
	}
	
	for (int32 i = 0; (next = (BTextControl *)(ChildAt(0)->ChildAt(i))) != NULL; i++) {
		text = next->Text();
		node.WriteAttrString(next->Name(),&text);
	}
	
	if (be_app->CountWindows() == 1 /* us */)
		be_app->PostMessage(B_QUIT_REQUESTED);
	
	return true;
}
