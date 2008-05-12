
webkittemplatedir = $(datadir)/carrier/webkit
nobase_webkittemplate_DATA = Template.html \
	defaultstyle/Contents/Resources/background.png \
	defaultstyle/Contents/Resources/main.css \
	defaultstyle/Contents/Resources/Status.html \
	defaultstyle/Contents/Resources/Incoming/buddy_icon.png \
	defaultstyle/Contents/Resources/Incoming/Content.html \
	defaultstyle/Contents/Resources/Incoming/Context.html \
	defaultstyle/Contents/Resources/Incoming/NextContent.html \
	defaultstyle/Contents/Resources/Incoming/NextContext.html \
	defaultstyle/Contents/Resources/Outgoing/buddy_icon.png \
	defaultstyle/Contents/Resources/Outgoing/Content.html \
	defaultstyle/Contents/Resources/Outgoing/Context.html \
	defaultstyle/Contents/Resources/Outgoing/NextContent.html \
	defaultstyle/Contents/Resources/Outgoing/NextContext.html


webkitdir = $(libdir)/pidgin

webkit_la_LDFLAGS = -module -avoid-version

EXTRA_DIST = $(webkittemplate_DATA)

if PLUGINS

webkit_LTLIBRARIES = webkit.la

webkit_la_SOURCES = webkit.c

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
