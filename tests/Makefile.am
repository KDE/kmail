INCLUDES = -I$(top_srcdir)/kmail -I$(top_srcdir)/mimelib $(all_includes)
AM_CPPFLAGS = -DKDESRCDIR=\"$(srcdir)\"
METASOURCES = AUTO

check_LTLIBRARIES = kunittest_storagelayermodule.la \
        kunittest_utilmodule.la \
        kunittest_mimelibmodule.la

kunittest_storagelayermodule_la_SOURCES = storagelayermodule.cpp messagedicttests.cpp ../kmdict.cpp
kunittest_utilmodule_la_SOURCES = utiltests.cpp ../util.cpp
kunittest_utilmodule_la_LIBADD = -lkunittest ../../mimelib/libmimelib.la
kunittest_mimelibmodule_la_SOURCES = mimelibtests.cpp ../util.cpp
kunittest_mimelibmodule_la_LIBADD = -lkunittest ../../mimelib/libmimelib.la

LIBADD = -lkunittest
LDFLAGS = -module $(KDE_CHECK_PLUGIN) $(all_libraries)

check-local:
	kunittestmodrunner

guicheck:
	kunittestguimodrunner
