AM_CFLAGS = -Wall -DPACKAGE_LOCALE_DIR="\"@PACKAGE_LOCALE_DIR@\""

bin_PROGRAMS = ift-extract ift-export

# extractor
ift_extract_SOURCES = extract.c
ift_extract_CFLAGS = $(IFT_CFLAGS) $(AM_CFLAGS)
ift_extract_LDADD = $(IFT_LIBS) -lgcrypt


# export
ift_export_SOURCES = export.c
ift_export_CFLAGS = $(IFT_CFLAGS) $(AM_CFLAGS)
ift_export_LDADD = $(IFT_LIBS)

# UDEV
libudevdir = @udevdir@
libudev_PROGRAMS = ift-load

ift_load_SOURCES = udev.c load.c load.h
ift_load_CFLAGS = $(IFT_CFLAGS) $(LOAD_CFLAGS) $(AM_CFLAGS)
ift_load_LDADD = $(IFT_LIBS) $(LOAD_LIBS)

rulesdir = ${sysconfdir}/udev/rules.d
rules_DATA = isight.rules

# AC_SUBST sucks
isight.rules: isight.rules.in Makefile
	sed 's|$${exec_prefix}|${exec_prefix}|' isight.rules.in > isight.rules;

CLEANFILES = isight.rules
