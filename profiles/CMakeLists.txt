kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#profiles = default html high-contrast purist secure
#
## remove the .desktop suffix on install (which is needed for translation)
#install-data-local:
#	$(mkinstalldirs) $(DESTDIR)$(kde_datadir)/kmail
#	for profile in $(profiles); do \
#		$(INSTALL_DATA) $(srcdir)/profile-$$profile-rc.desktop \
#			$(DESTDIR)$(kde_datadir)/kmail/profile-$$profile-rc; \
#	done
#
#uninstall-local:
#	-for profile in $(profiles); do \
#		rm -f $(DESTDIR)$(kde_datadir)/kmail/profile-$$profile-rc; \
#	done
