#ifndef ADD_ON_INFO_H
#define ADD_ON_INFO_H

#include <libim/Protocol.h>
#include <OS.h>

namespace IM {

class AddOnInfo
{
	public:
		Protocol		* protocol;
		const char		* signature;
		char			path[1024];
		image_id		image;
};

};

#endif
