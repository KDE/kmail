kde4_header()

add_subdirectory( icons ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kmail/pics FILES kmmsgdel.png kmmsgnew.png kmmsgunseen.png kmmsgread.png  	kmmsgreplied.png kmmsgforwarded.png kmmsgqueued.png kmmsgflag.png  	kmmsgsent.png kmmsgwatched.png kmmsgignored.png kmmsgtodo.png  	kmmsgread_fwd.png kmmsgread_replied.png kmmsgread_fwd_replied.png  	pgp-keys.png  	kmmsgpartiallyencrypted.png  	kmmsgpartiallysigned.png kmmsgfullyencrypted.png  	kmmsgfullysigned.png kmmsgundefinedencrypted.png  	kmmsgundefinedsigned.png  	kmmsgspam.png kmmsgham.png kmmsgattachment.png  	kmwizard.png  	quotecollapse.png quoteexpand.png )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = icons
#
#pics_DATA = kmmsgdel.png kmmsgnew.png kmmsgunseen.png kmmsgread.png \
#	kmmsgreplied.png kmmsgforwarded.png kmmsgqueued.png kmmsgflag.png \
#	kmmsgsent.png kmmsgwatched.png kmmsgignored.png kmmsgtodo.png \
#	kmmsgread_fwd.png kmmsgread_replied.png kmmsgread_fwd_replied.png \
#	pgp-keys.png \
#	kmmsgpartiallyencrypted.png \
#	kmmsgpartiallysigned.png kmmsgfullyencrypted.png \
#	kmmsgfullysigned.png kmmsgundefinedencrypted.png \
#	kmmsgundefinedsigned.png \
#	kmmsgspam.png kmmsgham.png kmmsgattachment.png \
#	kmwizard.png \
#	quotecollapse.png quoteexpand.png
#
#
#picsdir = $(kde_datadir)/kmail/pics
#
#EXTRA_DIST = $(pics_DATA)
#
