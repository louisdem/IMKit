# makefile for all sample code

SUBDIRS = \
	libim \
	server \
	sample_clients \
	protocols \
	utils

default .DEFAULT :
	# These two lines shouldn't be here. They should be in libim/makefile somehow.
	-mkdir /boot/home/config/include/libim
	-cp -u libim/*.h /boot/home/config/include/libim
	
	# copy the engine to a known location
	-cp -u makefile-engine.IMkit $(BUILDHOME)/etc/makefile-engine.IMkit

	# This seems to fail from the Makefile, but the exact same command works
	# if executed from Terminal, anyone?
	-unzip -n server/StatusIcons.zip -d ~/config/settings/im_kit/icons

	# must install lib to make the rest.
	$(MAKE) -C libim install

	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f -f makefile $@ || exit -1; \
	done

clean:
	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f -f makefile clean || exit -1; \
	done
