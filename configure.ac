AC_INIT(isight-firmware-tools, 1.6, https://bugs.launchpad.net/isight-firmware-tools/+filebug)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES(Makefile
	doc/Makefile
	src/isight.rules.in
	src/Makefile
	po/Makefile.in
	tests/Makefile
	)

dnl DEPS
AC_PROG_CC
AM_PROG_CC_C_O

AC_CHECK_HEADERS([gcrypt.h],[],[AC_MSG_ERROR([libgcrypt not found])])
AC_CHECK_PROG(MAKEINFO,makeinfo,makeinfo)

PKG_CHECK_MODULES(IFT, [glib-2.0 >= 2.13.4])
AC_SUBST(IFT_CFLAGS)
AC_SUBST(IFT_LIBS)

PKG_CHECK_MODULES(LOAD, [libusb])
AC_SUBST(LOAD_CFLAGS)
AC_SUBST(LOAD_LIBS)

udevdir=${libdir}/udev
AC_SUBST(udevdir)

dnl GETTEXT
GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [gettext package name])

PACKAGE_LOCALE_DIR="${datadir}/locale"
AC_SUBST(PACKAGE_LOCALE_DIR)
AM_GLIB_GNU_GETTEXT

IT_PROG_INTLTOOL([0.35])

AC_OUTPUT
