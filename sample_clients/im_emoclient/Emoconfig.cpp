#include "Emoconfig.h"

#include <File.h>
#include <stdio.h>
#include <Bitmap.h>
#include <String.h>
#include <Path.h>
#include <TranslationUtils.h>

//tmp
BMessage* faces=NULL;
bool 		valid=false;
bool 		fname=false;
BString filename;
BString face;
BPath 	path;


Emoconfig::Emoconfig(const char* xmlfile):BMessage()
{
	fParser = XML_ParserCreate(NULL);
	numfaces=0;
	XML_SetUserData(fParser, this);
	XML_SetElementHandler(fParser, StartElement, EndElement);
	XML_SetCharacterDataHandler(fParser, Characters);
	
	//path!
	BPath p(xmlfile);
	p.GetParent(&path);
	//printf("** Path() %s \n",p.Path());
	
	// loading the config file..
	BFile* settings=new BFile(xmlfile,B_READ_ONLY);
	off_t size;
	settings->GetSize(&size);
	printf("Original file %lld\n",size);
	if(size)
	{
		void *buffer=malloc(size);
		size=settings->Read(buffer,size);
		printf("In memory file %lld\n",size);
		XML_Parse(fParser, (const char*)buffer, size, true);
		free(buffer);
	}
	delete settings;
	if(fParser)	XML_ParserFree(fParser);
	//PrintToStream();
}

Emoconfig::~Emoconfig()
{
	
}

void 
Emoconfig::StartElement(void * pUserData, const char * pName, const char ** pAttr)
{
	//printf("StartElement %s\n",pName);
	BString name(pName);
	if(name.ICompare("emoticon")==0)
	{
		faces=new BMessage();
	
	}
	else
	if(name.ICompare("text")==0 && faces)
	{
		valid=true;
	}
	else
	if(name.ICompare("file")==0 && faces)
	{
		fname=true;
	}
	
}

void 
Emoconfig::EndElement(void * pUserData, const char * pName)
{
	//printf("EndElement %s\n",pName);
	BString name(pName);
	
	if(name.ICompare("emoticon")==0 && faces)
	{
		//faces->PrintToStream(); //debug
		delete faces;
		faces=NULL;
	
	}
	else
	if(name.ICompare("text")==0 && faces)
	{
		valid=false;
		faces->AddString("face",face);
		//printf("to ]%s[\n",face.String());
		face.SetTo("");
		
	}
	else
	if(name.ICompare("file")==0 && faces)
	{
		//load file
		
		//compose the filename
		BPath p(path);
		p.Append(filename.String());
		BBitmap *icons=BTranslationUtils::GetBitmap(p.Path());
		//printf("Filename %s %s\n",p.Path(),path.Path());
		if(!icons) return; 
		//assign to faces;
		fname=false;
		int 		i=0;
		BString s;
		while(faces->FindString("face",i,&s)==B_OK)
		{
		
			if(i==0)
			{
				((Emoconfig*)pUserData)->menu.AddPointer(s.String(),(const void*)icons);
				((Emoconfig*)pUserData)->menu.AddString("face",s.String());
			}
			((BMessage*)pUserData)->AddPointer(s.String(),(const void*)icons);
			((BMessage*)pUserData)->AddString("face",s.String());
			((Emoconfig*)pUserData)->numfaces++;
			i++;
			
		}
		
		
	}
	 
}

void 
Emoconfig::Characters(void * pUserData, const char * pString, int pLen)
{

		
	BString f(pString,pLen);
	//printf("Characters %s\n",f.String());
	if(faces && valid)
	{
		f.RemoveAll(" ");
		f.RemoveAll("\"");
		if(f.Length()>0)
		face.Append(f);
	}
	else
	if(fname)
	{
		f.RemoveAll(" ");
		filename=f;
		
	}
}



