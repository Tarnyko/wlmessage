m4_define([wlmessage_version], 0.1)

AC_PREREQ([2.64])
AC_INIT([wlmessage],
        [wlmessage_version],
        [https://bugs.tizen.org/],
        [wlmessage],
        [http://www.tizen.org/])

AC_SUBST([WLMESSAGE_VERSION], [wlmessage_version])

AC_CONFIG_AUX_DIR([build-aux])

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE

AM_INIT_AUTOMAKE([1.11 parallel-tests foreign no-dist-gzip dist-xz color-tests subdir-objects])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AC_PROG_SED

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

PKG_PROG_PKG_CONFIG()

AC_CHECK_FUNC([dlopen], [],
              AC_CHECK_LIB([dl], [dlopen], DLOPEN_LIBS="-ldl"))
AC_SUBST(DLOPEN_LIBS)

AC_CHECK_FUNCS([mkostemp strchrnul initgroups posix_fallocate])

AC_ARG_ENABLE(xdg-shell,
              AS_HELP_STRING([--enable-xdg-shell],
                             [build with XDG-Shell support (Weston 1.5.0)]),
                             [],[enable_xdg_shell=no])
AM_CONDITIONAL(ENABLE_XDG_SHELL, test x$enable_xdg_shell = xyes)
if test x$enable_xdg_shell = xyes; then
	AC_DEFINE([ENABLE_XDG_SHELL], [1], [Build with XDG-Shell support])
fi

AC_ARG_ENABLE(egl,
              AS_HELP_STRING([--disable-egl],
                             [do not build with EGL support]),
                             [],[enable_egl=yes])
AM_CONDITIONAL(ENABLE_EGL, test x$enable_egl = xyes)
if test x$enable_egl = xyes; then
	AC_DEFINE([ENABLE_EGL], [1], [Build with EGL support])
	PKG_CHECK_MODULES(EGL, [egl >= 7.10 glesv2])
fi

AC_ARG_WITH(cairo,
	    AS_HELP_STRING([--with-cairo=@<:@image|glesv2@:>@]
			  [which Cairo renderer to use for the clients]),
			   [],[with_cairo="image"])

if test "x$with_cairo" = "ximage"; then
	cairo_modules="cairo"
else
if test "x$with_cairo" = "xglesv2"; then
	cairo_modules="cairo-glesv2"
else
	AC_ERROR([Unknown cairo renderer requested])
fi
fi

# Included for legacy compat
AC_ARG_WITH(cairo-glesv2,
            AS_HELP_STRING([--with-cairo-glesv2],
                           [use GLESv2 cairo]))
if test "x$with_cairo_glesv2" = "xyes"; then
  cairo_modules="cairo-glesv2"
  with_cairo="glesv2"
fi

if test "x$cairo_modules" = "xcairo-glesv2"; then
AC_DEFINE([USE_CAIRO_GLESV2], [1], [Use the GLESv2 GL cairo backend])
fi

PKG_CHECK_MODULES(PIXMAN, [pixman-1])
PKG_CHECK_MODULES(PNG, [libpng])

AC_CHECK_LIB([jpeg], [jpeg_CreateDecompress], have_jpeglib=yes)
if test x$have_jpeglib = xyes; then
  JPEG_LIBS="-ljpeg"
else
  AC_ERROR([libjpeg not found])
fi
AC_SUBST(JPEG_LIBS)

PKG_CHECK_MODULES(GLIB, [glib-2.0 gio-2.0])

PKG_CHECK_MODULES(CLIENT, [wayland-client cairo >= 1.10.0 xkbcommon wayland-cursor])

  # Only check for cairo-egl if a GL or GLES renderer requested
  AS_IF([test "x$cairo_modules" = "xcairo-glesv2"], [
    PKG_CHECK_MODULES(CAIRO_EGL, [wayland-egl egl >= 7.10 cairo-egl >= 1.11.3 $cairo_modules],
                      [have_cairo_egl=yes], [have_cairo_egl=no])
    AS_IF([test "x$have_cairo_egl" = "xyes"],
	  [AC_DEFINE([HAVE_CAIRO_EGL], [1], [Have cairo-egl])],
	  [AC_ERROR([cairo-egl not used because $CAIRO_EGL_PKG_ERRORS])])],
  [have_cairo_egl=no])

AM_CONDITIONAL(HAVE_CAIRO_GLESV2,
	       [test "x$have_cairo_egl" = "xyes" -a "x$cairo_modules" = "xcairo-glesv2" -a "x$enable_egl" = "xyes"])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT

AC_MSG_RESULT([
	libxkbcommon			${enable_xkbcommon}
])
