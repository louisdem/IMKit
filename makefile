# makefile for all sample code

VERSION:=0.9.1-alpha
DIST:=imkit-$(VERSION)

SUBDIRS = \
	libim \
	server \
	sample_clients \
	protocols \
	utils \
	Preflet

.PHONY: default clean symlinks dist

default .DEFAULT :
	# Unpack the Server icons.
	-mkdir -p /boot/home/config/settings/im_kit	# Yes, it won't work if it does not exist.
	-unzip -n server/Icons.zip -d /boot/home/config/settings/im_kit/icons

	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f -f makefile $@ || exit -1; \
	done

clean:
	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f -f makefile clean || exit -1; \
	done

symlinks:
	ln -s "`pwd`/build/libim.so" /boot/home/config/lib
	mkdir -p /boot/home/config/servers
	ln -s "`pwd`/build/im_server" /boot/home/config/servers
	ln -s "`pwd`/build/protocols" /boot/home/config/add-ons/imkit
	
dist: all
	mkdir -p "$(DIST)"
	copyattr --data --recursive build/* "$(DIST)"
	mkdir -p "$(DIST)/doc"
	copyattr --data --recursive build/* "$(DIST)/doc"
	zip -r "$(DIST).zip" "$(DIST)/"
	rm -rf "$(DIST)"
	
	