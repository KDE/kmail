INCLUDES = -I$(top_srcdir)/kmail $(all_includes)
METASOURCES = AUTO

check_LTLIBRARIES = kunittest_storagelayermodule.la \
        kunittest_utilmodule.la

kunittest_storagelayermodule_la_SOURCES = storagelayermodule.cpp messagedicttests.cpp ../kmdict.cpp
kunittest_utilmodule_la_SOURCES = utiltests.cpp ../util.cpp
kunittest_utilmodule_la_LIBADD = -lkunittest ../../mimelib/libmimelib.la

LIBADD = -lkunittest
LDFLAGS = -module $(KDE_CHECK_PLUGIN) $(all_libraries)

check-local:
	kunittestmodrunner

guicheck:
	kunittestguimodrunner
