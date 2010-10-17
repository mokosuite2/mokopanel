INCLUDES = $(GLIB_CFLAGS) $(DBUS_CFLAGS) $(ELEMENTARY_CFLAGS) $(FSO_CFLAGS) $(MOKOSUITE_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\" -DDATADIR=\""$(datadir)"\" -DSYSCONFDIR=\""$(sysconfdir)"\" -Wall

bin_PROGRAMS = mokopanel

mokopanel_SOURCES = \
	main.c \
	globals.h

mokopanel_LDADD = \
	$(GLIB_LIBS) $(DBUS_LIBS) $(ELEMENTARY_LIBS) \
	$(FSO_LIBS) $(MOKOSUITE_LIBS)

MAINTAINERCLEANFILES = \
	Makefile.in
