AC_PREREQ([2.60])
AC_INIT([fbdev_simple],
        [1.0.0],
        [https://www.tizen.org],
        [fbdev_simple])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([Makefile.am])
AM_INIT_AUTOMAKE([1.10 foreign dist-bzip2])
AM_MAINTAINER_MODE([enable])

# Check for programs
AC_PROG_CC

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_FUNC_ALLOCA

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

# Enable quiet compiles on automake 1.11.
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

PKG_CHECK_MODULES(UDEV, libudev, [udev=yes], [udev=no])
if test x"$udev" = xyes; then
	AC_DEFINE(HAVE_UDEV,1,[Enable udev-based monitor hotplug detection])
	TDM_DRM_CFLAGS="$TDM_DRM_CFLAGS $UDEV_CFLAGS"
	TDM_DRM_LIBS="$TDM_DRM_LIBS $UDEV_LIBS"
fi

# For enumerating devices in test case
AC_OUTPUT([
	Makefile
	src/Makefile])

echo ""
echo "$PACKAGE_STRING will be compiled with:"
echo ""
echo "TDM_DRM_CFLAGS : $TDM_DRM_CFLAGS"
echo "TDM_DRM_LIBS   : $TDM_DRM_LIBS"
echo "TDM_MODULE_DIR    : $TDM_MODULE_PATH"
echo ""
