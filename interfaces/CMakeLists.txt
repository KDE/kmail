kde4_header()




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
