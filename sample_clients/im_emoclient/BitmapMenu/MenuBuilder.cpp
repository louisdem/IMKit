#include <Font.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <stdio.h>
#include <string.h>
#include <String.h>

//#include "TestIcons.h"
#include "MenuBuilder.h"
#include "BitmapMenuItem.h"

#include "PopUpMenu.h"

#define NUMX 8
#define NUMY 10

#define ICON	22
#define PAD	4

#define TOTICON (ICON+PAD)

BMenu* 
MenuBuilder::CreateMenu(BMessage* faces,int32 messid)
{
	
	BMenu* xMenu;
	BBitmap *xBitmap;
	BitmapMenuItem *xItem;
	
	float menuWidth = NUMX*TOTICON;
	float menuHeight = NUMY*TOTICON;
	
	
	xMenu = new BMenu("emoticons", menuWidth, menuHeight);
	
	for (int32 i=0; i<NUMY; i++) {
		for (int32 j=0; j<NUMX; j++) {
	
		xBitmap = LoadBitmap(i, j, faces);
		
		if(xBitmap){
			xItem = new BitmapMenuItem("", xBitmap,new BMessage(messid), 0, 0);
			xMenu->AddItem(xItem, BRect(j*TOTICON,i*TOTICON,j*TOTICON+TOTICON-1,i*TOTICON+TOTICON-1));
		 }
		
		}
	}
	return xMenu;
	
}

BPopUpMenu* 
MenuBuilder::CreateMenuP(BMessage* faces,int32 messid)
{
	
	BPopUpMenu* 	xMenu;
	BBitmap*		xBitmap;
	BitmapMenuItem*	xItem;
	
	float menuWidth = NUMX*TOTICON;
	float menuHeight = NUMY*TOTICON;
	
	
	xMenu = new BPopUpMenu("emoticons", menuWidth, menuHeight,false,false);
	
	for (int32 i=0; i<NUMY; i++) {
		for (int32 j=0; j<NUMX; j++) {
	
		xBitmap = LoadBitmap(i, j, faces);
		
		if(xBitmap){
			xItem = new BitmapMenuItem("", xBitmap,new BMessage(messid), 0, 0);
			xMenu->AddItem(xItem, BRect(j*TOTICON,i*TOTICON,j*TOTICON+TOTICON-1,i*TOTICON+TOTICON-1));
		 }
		
		}
	}
	return xMenu;
	
}

BBitmap* 
MenuBuilder::LoadBitmap(int32 i, int32 j,BMessage*faces)
{
	BBitmap* pBitmap;
	BString f;
	int index=NUMX*i + j;
	faces->FindString("face",index,&f);
	faces->FindPointer(f.String(),(void**)&pBitmap);
		
	return pBitmap;		
}
