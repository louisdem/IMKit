#ifndef _SmileTextRender_H_
#define _SmileTextRender_H_

#include "TextRender.h"
#include <Font.h>
#include <View.h>

#include <stdio.h>
#include <TranslationUtils.h>
#include <Resources.h>
#include <String.h>

#include "Emoticor.h"

class SmileTextRender : public TextRender
{
    public:
          SmileTextRender():TextRender(){};
                        
       
       virtual void Render(BView *target,const char* txt,int16 num,BPoint pos)  {
           
            BBitmap *pointer=NULL;
            BString f(txt,num);
            
          	if(emoticor->config->FindPointer(f.String(),(void**)&pointer)==B_OK)
          	{
          	 	target->DrawBitmapAsync(pointer,pos-BPoint(0,12));
          	}
        }; 
       
       
       virtual float Size(){ return 24 ;}
       
       virtual void GetHeight(font_height *h)
       { 
       		h->ascent=12;
       		h->descent=12;
       		h->leading=0;
       	};
    
    
	   virtual void		
	   GetEscapements(const char charArray[], int32 numChars,float escapementArray[])
	   {
  			//font.GetEscapements(charArray,numChars,escapementArray);
  			escapementArray[0]=1;
  			for(int i=1;i<numChars;i++) escapementArray[i]=0;
  	   }
};
#endif
