# makefile for all sample code

SUBDIRS = \
	im_client \
	logger \
	tracker_addons \
	infopopper \
	im_binlogger \
	im_binlog_viewer

default .DEFAULT :
	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f -f makefile $@ || exit -1; \
	done

clean:
	-@for f in $(SUBDIRS) ; do \
		$(MAKE) -C $$f -f makefile clean || exit -1; \
	done
