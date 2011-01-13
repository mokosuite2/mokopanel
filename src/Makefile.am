VALAC_ARGS = \
	--vapidir=$(top_srcdir)/vapi \
	--pkg dbus-glib-1 \
	--pkg panel \
	--pkg config \
	--save-temps \
	--basedir $(top_srcdir)/src

INCLUDES = $(GLIB_CFLAGS) $(DBUS_CFLAGS) $(ELEMENTARY_CFLAGS) $(EFREET_CFLAGS) $(FSO_CFLAGS) $(MOKOSUITE_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\" -DDATADIR=\""$(datadir)"\" -DSYSCONFDIR=\""$(sysconfdir)"\" -Wall

bin_PROGRAMS = mokopanel

BUILT_SOURCES = .vala.stamp

.vala.stamp: $(mokopanel_VALASOURCES)
	$(VALAC) -C $(VALAC_ARGS) -H $(^:.vala=.h) $^
	sed -i 's/\#include <$(^:.vala=.h)>//' $(^:.vala=.c)
	touch $@

mokopanel_VALASOURCES = \
	notifications-service.vala

mokopanel_SOURCES = $(dbus_sources) \
	$(mokopanel_VALASOURCES:.vala=.c) \
	$(mokopanel_VALASOURCES:.vala=.h) \
	notifications-win.c \
	notifications-win.h \
	main.c \
	panel.c \
	panel.h \
	clock.c \
	clock.h \
	battery.c \
	battery.h \
	gsm.c \
	gsm.h \
	gps.c \
	gps.h \
	idle.c \
	idle.h \
	shutdown.c \
	shutdown.h

mokopanel_LDADD = \
	$(FSO_LIBS) $(ELEMENTARY_LIBS) $(EFREET_LIBS) $(DBUS_LIBS) $(GLIB_LIBS) $(MOKOSUITE_LIBS)

EXTRA_DIST = \
	$(BUILT_SOURCES) \
	$(mokopanel_VALASOURCES:.vala=.c) \
	$(mokopanel_VALASOURCES:.vala=.h)

CLEANFILES = \
	$(BUILT_SOURCES) \
	$(mokopanel_VALASOURCES:.vala=.c) \
	$(mokopanel_VALASOURCES:.vala=.h)

MAINTAINERCLEANFILES = \
	Makefile.in
