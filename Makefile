SUBDIRS	= bk ibfs

subdirs:
		for dir in $(SUBDIRS); do \
			make -C $$dir; \
		done
