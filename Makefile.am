bin_PROGRAMS =

all-local :

AM_CFLAGS = $(GCC_CFLAGS)

AM_CPPFLAGS = 					\
	-I$(top_srcdir)/toytoolkit		\
	-DDATADIR='"$(datadir)"'

CLEANFILES =

bin_PROGRAMS += wlmessage

wlmessage_LDFLAGS = -export-dynamic
wlmessage_CPPFLAGS = $(AM_CPPFLAGS) -Wno-unused-result
wlmessage_CFLAGS = $(GCC_CFLAGS) $(PNG_CFLAGS) $(PIXMAN_CFLAGS) $(CLIENT_CFLAGS) $(CAIRO_EGL_CFLAGS) $(GLIB_CFLAGS)
wlmessage_LDADD = $(DLOPEN_LIBS) $(PNG_LIBS) $(PIXMAN_LIBS) $(CLIENT_LIBS) $(CAIRO_EGL_LIBS) $(JPEG_LIBS) $(GLIB_LIBS) -lm

wlmessage_SOURCES =					\
	wlmessage.c					\
	toytoolkit/shared/frame.c			\
	toytoolkit/shared/image-loader.c		\
	toytoolkit/shared/cairo-util.c			\
	toytoolkit/shared/os-compatibility.c		\
	toytoolkit/xdg-shell-protocol.c			\
	toytoolkit/text-cursor-position-protocol.c	\
	toytoolkit/workspaces-protocol.c		\
	toytoolkit/window.c
