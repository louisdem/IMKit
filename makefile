# makefile for all sample code

SUBDIRS = \
	libim \
	server \
	sample_clients \
	protocols \
	utils \
	Preflet

default .DEFAULT :
	# These two lines shouldn't be here. They should be in libim/makefile somehow.
	-mkdir -p /boot/home/config/include/libim
	-cp -u libim/*.h /boot/home/config/include/libim
	
	# copy the engine to a known location
	-cp -u makefile-engine.IMkit $(BUILDHOME)/etc/makefile-engine.IMkit

	# Unpack the Server icons.
	-mkdir -p /boot/home/config/settings/im_kit	# Yes, it won't work if it does not exist.
	-unzip -n server/Icons.zip -d /boot/home/config/settings/im_kit/icons

	# Create the _APP_ link needed by all protocols. Note you must do make install
	# for this to work.
	-ln -sf `pwd`/build/im_server protocols/ICQ/_APP_
	-ln -sf `pwd`/build/im_server protocols/AIM/_APP_

	# must install lib to make the rest.
	$(MAKE) -C libim install

	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f -f makefile $@ || exit -1; \
	done

clean:
	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f -f makefile clean || exit -1; \
	done
