kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( /include/kmail/interfaces FILES observer.h  	observable.h  	htmlwriter.h  	bodypart.h  	bodypartformatter.h  	bodyparturlhandler.h )

kde4_footer()



#original Makefile.am contents follow:

#kmailincludedir = $(includedir)/kmail/interfaces
#kmailinclude_HEADERS = \
#	observer.h \
#	observable.h \
#	htmlwriter.h \
#	bodypart.h \
#	bodypartformatter.h \
#	bodyparturlhandler.h
