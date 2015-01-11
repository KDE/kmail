install( FILES kmail.upd  DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})
install(PROGRAMS 
  upgrade-transport.pl
  kmail-pgpidentity.pl
  upgrade-signature.pl
  kmail-upd-identities.pl
  kmail-3.1-use-UOID-for-identities.pl
  kmail-3.1-update-new-mail-notification-settings.pl
  kmail-3.1.4-dont-use-UOID-0-for-any-identity.pl
  kmail-3.2-update-loop-on-goto-unread-settings.sh
  kmail-3.2-misc.sh
  kmail-3.3-use-ID-for-accounts.pl
  kmail-3.3-move-identities.pl
  kmail-3.3-aegypten.pl
  kmail-3.3-split-sign-encr-keys.sh
  kmail-3.3-misc.pl
  kmail-3.3b1-misc.pl
  kmail-3.4-misc.pl
  kmail-3.4.1-update-status-filters.pl
  kmail-3.5-trigger-flag-migration.pl
  kmail-4.0-misc.pl 
DESTINATION ${KDE_INSTALL_KCONFUPDATEDIR})


