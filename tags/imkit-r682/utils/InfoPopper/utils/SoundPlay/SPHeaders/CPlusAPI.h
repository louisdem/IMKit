
/*

	This is the all new C++ filter plugin API. Other plugin-categories
	will follow shortly.

	To use it, you create a PluginFactory class that returns FilterPlugin
	(derived) objects. Your plugin should then export a function like this:
		extern "C" status_t _EXPORT GetPluginFactories(BList *out_list);
	The function should fill out_list with pointers to PluginFactory objects
	(usually you'll add just one, but you can add more).
	Finally, you need to link with the CPlus add-on. 

	See the 'NoVoice++' project for an example implementation.

*/

#ifndef _CPLUSPLUGINAPI_H
#define _CPLUSPLUGINAPI_H

#include <SupportDefs.h>
#include "pluginproto.h"


typedef enum {
	FUNCTION_CONFIGURE = 1,
	FUNCTION_ABOUT     = 2
} plugin_function_ident;

class Plugin
{
	private:
		friend class FilterPlugin;
		friend class CPlusAPIAccessor;

		void *fData;
		Plugin();

	public:
		virtual ~Plugin();

		void Disable(plugin_function_ident which);
};

class FilterPlugin: public Plugin
{
	public:
		FilterPlugin();
		virtual ~FilterPlugin();

		// one of these must be implemented
		virtual status_t FilterShort(short *buffer,int32 framecount, void *info);
		virtual status_t FilterFloat(float **input, float **output, int32 framecount, void *info);

		virtual void 	FileChange(const char *name, const char *path);
		virtual BView*	Configure();
		virtual void	SetConfig(BMessage *config);
		virtual void	GetConfig(BMessage *config);

		virtual void _reservedfilter1();
		virtual void _reservedfilter2();
		virtual void _reservedfilter3();
		virtual void _reservedfilter4();
};

class PluginFactory
{
	public:
		PluginFactory();
		virtual ~PluginFactory();

		void Disable(plugin_function_ident which);

		virtual const char*	ID()=0;
		virtual uint32		Flags()=0;
		virtual const char*	Name()=0;
		virtual const char*	AboutString();
		virtual void		About();
		virtual BView*		Configure(BMessage *config);
		virtual Plugin* 	CreatePlugin(const char *name,
									const char *header,
									uint32 size,
									plugin_info *pluginfo)=0;

	private:
		friend class CPlusAPIAccessor;
		void *fData;

		virtual void _reservedpluginfactory1();
		virtual void _reservedpluginfactory2();
		virtual void _reservedpluginfactory3();
		virtual void _reservedpluginfactory4();
};

// Your C++ plugin should export this function. It should fill out_list
// with pointers to PluginFactory* for all the plugins it supports.
extern "C" status_t _EXPORT GetPluginFactories(BList *out_list);
#endif
