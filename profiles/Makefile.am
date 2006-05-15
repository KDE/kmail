profiles = default html high-contrast purist secure

# remove the .desktop suffix on install (which is needed for translation)
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(kde_datadir)/kmail
	for profile in $(profiles); do \
		$(INSTALL_DATA) $(srcdir)/profile-$$profile-rc.desktop \
			$(DESTDIR)$(kde_datadir)/kmail/profile-$$profile-rc; \
	done

uninstall-local:
	-for profile in $(profiles); do \
		rm -f $(DESTDIR)$(kde_datadir)/kmail/profile-$$profile-rc; \
	done
