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

COMMON_LIB:=$(shell finddir B_COMMON_LIB_DIRECTORY)
COMMON_SERVERS:=$(shell finddir B_COMMON_SERVERS_DIRECTORY)
COMMON_ADDONS:=$(shell finddir B_COMMON_ADDONS_DIRECTORY)
BUILD:=$(shell pwd)/build

symlinks:
	ln -sf "$(BUILD)/lib/libim.so" "$(COMMON_LIB)"
	-if [ ! -d "$(COMMON_SERVERS)" ]; then \
		mkdir -p "$(COMMON_SERVERS)"; \
	fi
	
	ln -sf "$(BUILD)/im_server"  "$(COMMON_SERVERS)"
	rm -rf "$(COMMON_ADDONS)/im_kit"
	mkdir "$(COMMON_ADDONS)/im_kit"
	ln -sf "$(BUILD)/protocols" "$(COMMON_ADDONS)/im_kit"
	ln -sf "$(BUILD)/tracker-addons/IM_Merge_contacts" "$(COMMON_ADDONS)/Tracker"
	ln -sf "$(BUILD)/tracker-addons/IM_Start_conversation" "$(COMMON_ADDONS)/Tracker"
	ln -sf "$(BUILD)/settings/InstantMessaging" /boot/home/config/be/Preferences

dist: all
	mkdir -p "$(DIST)"
	copyattr --data --recursive build/* "$(DIST)"
	mkdir -p "$(DIST)/doc"
	copyattr --data --recursive build/* "$(DIST)/doc"
	zip -r "$(DIST).zip" "$(DIST)/"
	rm -rf "$(DIST)"
	
	