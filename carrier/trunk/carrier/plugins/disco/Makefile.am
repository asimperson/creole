plugindir = $(libdir)/carrier

xmppdisco_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = \
        Makefile.mingw

if PLUGINS

plugin_LTLIBRARIES = xmppdisco.la

xmppdisco_la_SOURCES = \
	gtkdisco.c \
	gtkdisco.h \
	xmppdisco.c \
	xmppdisco.h

xmppdisco_la_LIBADD = $(GTK_LIBS)

endif

AM_CPPFLAGS = \
	-DDATADIR=\"$(datadir)\" \
	-I$(top_srcdir)/libpurple \
	-I$(top_builddir)/libpurple \
	-I$(top_srcdir)/carrier \
	$(DEBUG_CFLAGS) \
	$(GTK_CFLAGS)
