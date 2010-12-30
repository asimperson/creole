carriersmileypix_in_files = none.theme.in

if INSTALL_PIXMAPS
carriersmileypixdir = $(datadir)/pixmaps/carrier/emotes/none
carriersmileypix_DATA = theme

theme: none.theme.in
	sed -e 's/^_Name=/Name=/' \
	    -e 's/^_Description=/Description=/' \
	    -e 's/^_Author=/Author=/' \
	$< > $@
endif

EXTRA_DIST = \
	$(carriersmileypix_in_files) \
	Makefile.mingw \
	theme
