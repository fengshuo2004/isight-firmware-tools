DISTCHECK_CONFIGURE_FLAGS = --enable-udev

SUBDIRS = src doc po tests

doc_DATA = HOWTO

EXTRA_DIST = \
	intltool-extract.in	\
	intltool-update.in	\
	intltool-merge.in	\
	po/Makefile.in.in	\
	po/POTFILES.in		\
	$(doc_DATA)

DISTCLEANFILES = \
	intltool-extract	\
	intltool-update		\
	intltool-merge
