kde4_header()




########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kmail/about FILES top-right-kmail.png  	main.html  	kmail.css )

kde4_footer()



#original Makefile.am contents follow:

#about_DATA = \
#	top-right-kmail.png \
#	main.html \
#	kmail.css
#
#aboutdir = $(kde_datadir)/kmail/about
