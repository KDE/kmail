kde4_header()

include_directories( ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/kmail ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/kmail $(all_includes)
#METASOURCES = AUTO
#
#noinst_HEADERS = messagedicttests.h ../kmdict.h
#
#check_LTLIBRARIES = kunittest_storagelayermodule.la 
#
#kunittest_storagelayermodule_la_SOURCES = storagelayermodule.cpp messagedicttests.cpp ../kmdict.cpp
#kunittest_storagelayermodule_la_LIBADD = -lkunittest
#kunittest_storagelayermodule_la_LDFLAGS = -module $(KDE_CHECK_PLUGIN) $(all_libraries)
#
#check-local:
#	kunittestmodrunner
#
#guicheck:
#	kunittestguimodrunner
