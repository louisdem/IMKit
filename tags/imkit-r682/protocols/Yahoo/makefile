## BeOS Generic Makefile v2.2 ##

## Fill in this file to specify the project being created, and the referenced
## makefile-engine will do all of the hard work for you.  This handles both
## Intel and PowerPC builds of the BeOS.

## Application Specific Settings ---------------------------------------------

ifeq ($(wildcard /boot/home/config/include/yahoo2.h),)

.PHONY: default
default:
	@echo Yahoo require libyahoo2. Yahoo add-on ignored.

else

ifeq ($(wildcard /boot/develop/headers/be/bone/arpa/inet.h),)

.PHONY: default
default:
	@echo Yahoo require bone. Yahoo add-on ignored.

else

# specify the name of the binary
NAME= yahoo
SVG_ICON=Yahoo.svg
TOPDIR=../..
TARGET_DIR=$(TOPDIR)/build/protocols

# specify the type of binary
#	APP:	Application
#	SHARED:	Shared library or add-on
#	STATIC:	Static library archive
#	DRIVER: Kernel Driver
TYPE= SHARED

#	add support for new Pe and Eddie features
#	to fill in generic makefile

#%{
# @src->@ 

#	specify the source files to use
#	full paths or paths relative to the makefile can be included
# 	all files, regardless of directory, will have their object
#	files created in the common object directory.
#	Note that this means this makefile will not work correctly
#	if two source files with the same name (source.c or source.cpp)
#	are included from different directories.  Also note that spaces
#	in folder names do not work well with this makefile.
SRCS= Yahoo.cpp YahooCallbacks.cpp YahooConnection.cpp $(TOPDIR)/common/htmlparse.cpp

#	specify the resource files to use
#	full path or a relative path to the resource file can be used.
RSRCS= yahoo.rsrc

# @<-src@ 
#%}

#	end support for Pe and Eddie

#	specify additional libraries to link against
#	there are two acceptable forms of library specifications
#	-	if your library follows the naming pattern of:
#		libXXX.so or libXXX.a you can simply specify XXX
#		library: libbe.so entry: be
#		
#	- 	if your library does not follow the standard library
#		naming scheme you need to specify the path to the library
#		and it's name
#		library: my_lib.a entry: my_lib.a or path/my_lib.a
LIBS= be im stdc++.r4 _APP_  yahoo2

#	specify additional paths to directories following the standard
#	libXXX.so or libXXX.a naming scheme.  You can specify full paths
#	or paths relative to the makefile.  The paths included may not
#	be recursive, so include all of the paths where libraries can
#	be found.  Directories where source files are found are
#	automatically included.
LIBPATHS= /boot/home/config/lib ../../build

#	additional paths to look for system headers
#	thes use the form: #include <header>
#	source file directories are NOT auto-included here
SYSTEM_INCLUDE_PATHS = /boot/home/config/include /boot/develop/headers/be/bone/sys ../..  . libyahoo2


#	additional paths to look for local headers
#	thes use the form: #include "header"
#	source file directories are automatically included
LOCAL_INCLUDE_PATHS = $(TOPDIR)/common

#	specify the level of optimization that you desire
#	NONE, SOME, FULL
OPTIMIZE= SOME

#	specify any preprocessor symbols to be defined.  The symbols will not
#	have their values set automatically; you must supply the value (if any)
#	to use.  For example, setting DEFINES to "DEBUG=1" will cause the
#	compiler option "-DDEBUG=1" to be used.  Setting DEFINES to "DEBUG"
#	would pass "-DDEBUG" on the compiler's command line.
DEFINES=

#	specify special warning levels
#	if unspecified default warnings will be used
#	NONE = supress all warnings
#	ALL = enable all warnings
WARNINGS = ALL 

#	specify whether image symbols will be created
#	so that stack crawls in the debugger are meaningful
#	if TRUE symbols will be created
SYMBOLS = TRUE

#	specify debug settings
#	if TRUE will allow application to be run from a source-level
#	debugger.  Note that this will disable all optimzation.
DEBUGGER = TRUE 

#	specify additional compiler flags for all files
#COMPILER_FLAGS = -g -fcheck-memory-usage -D_NO_INLINE_ASM=1

#	specify additional linker flags
LINKER_FLAGS =

#	specify the version of this particular item
#	(for example, -app 3 4 0 d 0 -short 340 -long "340 "`echo -n -e '\302\251'`"1999 GNU GPL") 
#	This may also be specified in a resource.
APP_VERSION = 

#	(for TYPE == DRIVER only) Specify desired location of driver in the /dev
#	hierarchy. Used by the driverinstall rule. E.g., DRIVER_PATH = video/usb will
#	instruct the driverinstall rule to place a symlink to your driver's binary in
#	~/add-ons/kernel/drivers/dev/video/usb, so that your driver will appear at
#	/dev/video/usb when loaded. Default is "misc".
DRIVER_PATH = 

ifeq ($(CHECK_MEM), true)
# 	COMPILER_FLAGS += -fcheck-memory-usage -D_NO_INLINE_ASM=1 -D_KERNEL_MODE=1
	COMPILER_FLAGS += -fcheck-memory-usage -D_NO_INLINE_ASM=1
	DEBUG_BUILD=true
	LIBS += socket bind
endif

#   Custom overrides that can be set from the command line.
ifeq ($(DEBUG_BUILD), true)
	SYMBOLS := TRUE
	DEBUGGER := TRUE
	OPTIMIZE := NONE
	COMPILER_FLAGS += -DDEBUG=1
	
	ifeq ($(shell uname -r), 5.1)
		COMPILER_FLAGS += -fno-debug-opt
	endif

endif



#	Detect BONE
ifeq ($(shell ls 2>/dev/null -1 /boot/develop/headers/be/bone/bone_api.h), /boot/develop/headers/be/bone/bone_api.h)
	SYSTEM_INCLUDE_PATHS += /boot/develop/headers/be/bone
	LIBS += socket bind
	DEFINES += BONE
endif

INSTALL_DIR= /boot/home/config/add-ons/im_kit/protocols

include $(TOPDIR)/makefile-engine.IMkit


# $(TARGET): _APP_
# 
# _APP_: $(TOPDIR)/build/im_server
# 	-ln -sf $< $@

endif #bone sanity check.

endif #yahoo2 sanity check.
