
webkittemplatedir = $(datadir)/carrier/webkit
nobase_webkittemplate_DATA = Cloudbourne.AdiumMessageStyle/Contents/Resources/background.png \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/main.css \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Status.html \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Template.html \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Incoming/buddy_icon.png \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Incoming/Content.html \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Incoming/Context.html \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Incoming/NextContent.html \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Incoming/NextContext.html \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Outgoing/buddy_icon.png \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Outgoing/Content.html \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Outgoing/Context.html \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Outgoing/NextContent.html \
	Cloudbourne.AdiumMessageStyle/Contents/Resources/Outgoing/NextContext.html


webkitdir = $(libdir)/pidgin

webkit_la_LDFLAGS = -module -avoid-version

EXTRA_DIST =	Makefile.mingw \
		$(webkittemplate_DATA)

if PLUGINS

webkit_LTLIBRARIES = webkit.la

webkit_la_SOURCES = webkit.c prefs.c

endif

webkit_la_LIBADD = $(GTK_LIBS) $(WEBKIT_LIBS)

AM_CPPFLAGS = \
	-DDATADIR=\"$(datadir)\" \
	-I$(top_srcdir)/libpurple \
	-I$(top_builddir)/libpurple \
	-I$(top_srcdir)/carrier \
	$(DEBUG_CFLAGS) \
	$(GTK_CFLAGS) \
	$(WEBKIT_CFLAGS)
