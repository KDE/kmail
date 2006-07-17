project(kmail)

configure_file(config-kmail.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kmail.h )


add_subdirectory( interfaces ) 
add_subdirectory( about ) 
add_subdirectory( pics ) 
add_subdirectory( profiles ) 
add_subdirectory( avscripts ) 
add_subdirectory( tests ) 

include_directories( ${CMAKE_SOURCE_DIR}/libkmime ${CMAKE_SOURCE_DIR}/libkpgp
  ${CMAKE_SOURCE_DIR}/libkdepim ${CMAKE_SOURCE_DIR}/libkpimidentities
  ${CMAKE_SOURCE_DIR}/libksieve
  ${CMAKE_SOURCE_DIR}/mimelib ${CMAKE_SOURCE_DIR}/libkleo
  ${CMAKE_SOURCE_DIR}/indexlib
  ${GPGME_INCLUDES}
   )


########### next target ###############

set(kmailprivate_LIB_SRCS
   kmmessage.cpp 
   kmmainwin.cpp 
   configuredialog.cpp 
   configuredialog_p.cpp 
   klistviewindexedsearchline.cpp 
   simplestringlisteditor.cpp 
   index.cpp 
   identitydrag.cpp 
   identitylistview.cpp 
   identitydialog.cpp 
   kmfolderdia.cpp 
   kmfoldertree.cpp 
   kmtransport.cpp 
   kmfoldercombobox.cpp 
   kmaccount.cpp 
   kmheaders.cpp 
   headeritem.cpp 
   listjob.cpp 
   kmcomposewin.cpp 
   kmfolder.cpp 
   kmmsgpartdlg.cpp 
   kmreaderwin.cpp 
   htmlstatusbar.cpp 
   kmmsgdict.cpp 
   kmgroupware.cpp 
   folderstorage.cpp 
   csshelper.cpp 
   klistboxdialog.cpp 
   actionscheduler.cpp 
   messageproperty.cpp 
   kmmsgpart.cpp 
   kmmsginfo.cpp 
   accountmanager.cpp 
   kmacctfolder.cpp 
   kmdict.cpp 
   kmsystemtray.cpp 
   kmacctlocal.cpp 
   kmfolderdir.cpp 
   kmfoldermgr.cpp 
   kmfoldernode.cpp 
   kmsender.cpp 
   kmacctseldlg.cpp 
   kmfiltermgr.cpp 
   kmsearchpatternedit.cpp 
   kmfilteraction.cpp 
   kmsearchpattern.cpp 
   kmfolderseldlg.cpp 
   kmfilter.cpp 
   kmfilterdlg.cpp 
   kmmsgbase.cpp 
   kmmsglist.cpp 
   kmaddrbook.cpp 
   signatureconfigurator.cpp 
   xfaceconfigurator.cpp 
   networkaccount.cpp 
   imapaccountbase.cpp 
   kmservertest.cpp 
   kmacctimap.cpp 
   kmacctcachedimap.cpp 
   kmfawidgets.cpp 
   kmfoldermbox.cpp 
   kmfolderimap.cpp 
   undostack.cpp 
   kmfoldercachedimap.cpp 
   kmfoldermaildir.cpp 
   popaccount.cpp 
   colorlistbox.cpp 
   kmkernel.cpp 
   accountdialog.cpp 
   searchwindow.cpp 
   vcardviewer.cpp 
   vacationdialog.cpp 
   vacation.cpp 
   sievedebugdialog.cpp 
   sieveconfig.cpp 
   sievejob.cpp 
   kmpopheaders.cpp 
   kmpopfiltercnfrmdlg.cpp 
   kmmimeparttree.cpp 
   mailinglist-magic.cpp 
   kmacctmaildir.cpp 
   attachmentstrategy.cpp 
   headerstrategy.cpp 
   headerstyle.cpp 
   khtmlparthtmlwriter.cpp 
   filehtmlwriter.cpp 
   teehtmlwriter.cpp 
   objecttreeparser.cpp 
   attachmentcollector.cpp 
   bodypartformatter.cpp 
   bodypartformatterfactory.cpp 
   partNode.cpp 
   signatureconfigurationdialogimpl.cpp 
   encryptionconfigurationdialogimpl.cpp 
   mailsourceviewer.cpp 
   kmcommands.cpp 
   kmreadermainwin.cpp 
   kmstartup.cpp 
   kmmainwidget.cpp 
   kmfolderindex.cpp 
   kmfoldersearch.cpp 
   transportmanager.cpp 
   folderjob.cpp 
   cachedimapjob.cpp 
   maildirjob.cpp 
   mboxjob.cpp 
   imapjob.cpp 
   subscriptiondialog.cpp 
   kmailicalifaceimpl.cpp 
   aboutdata.cpp 
   folderIface.cpp 
#   mailserviceimpl.cpp 
   attachmentlistview.cpp 
   kmedit.cpp 
   kmlineeditspell.cpp 
   kmatmlistview.cpp 
   composer.cpp 
   isubject.cpp 
   bodyvisitor.cpp 
   antispamwizard.cpp 
   urlhandlermanager.cpp 
   dictionarycombobox.cpp 
   secondarywindow.cpp 
   filterlog.cpp 
   filterlogdlg.cpp 
   messagecomposer.cpp 
   keyresolver.cpp 
   globalsettings.cpp 
   regexplineedit.cpp 
   rulewidgethandlermanager.cpp 
   headerlistquicksearch.cpp 
   acljobs.cpp 
   folderdiaacltab.cpp 
   partnodebodypart.cpp 
   expirejob.cpp 
   compactionjob.cpp 
   jobscheduler.cpp 
   callback.cpp 
   searchjob.cpp 
   renamejob.cpp 
   annotationjobs.cpp 
   accountcombobox.cpp 
   redirectdialog.cpp 
   foldershortcutdialog.cpp 
   folderrequester.cpp 
   spamheaderanalyzer.cpp 
   antispamconfig.cpp 
   recipientseditor.cpp 
   recipientspicker.cpp 
   kwindowpositioner.cpp 
   distributionlistdialog.cpp 
   expirypropertiesdialog.cpp 
   mailinglistpropertiesdialog.cpp 
   newfolderdialog.cpp 
   accountwizard.cpp 
   textsource.cpp 
   managesievescriptsdialog.cpp 
   chiasmuskeyselector.cpp 
   util.cpp
   keditcl1.cpp keditcl2.cpp )

kde4_automoc(${kmailprivate_LIB_SRCS})

kde4_add_ui3_files(kmailprivate_LIB_SRCS 
   signatureconfigurationdialog.ui 
   encryptionconfigurationdialog.ui 
   composercryptoconfiguration.ui 
   warningconfiguration.ui 
   smimeconfiguration.ui )

#kde4_add_dcop_skels(kmailprivate_LIB_SRCS 
#   kmailIface.h 
#   kmailicalIface.h 
#   folderIface.h )
qt4_add_dbus_adaptor( kmailprivate_LIB_SRCS org.kde.kmail.mailcomposer.xml kmcomposewin.h KMComposeWin )

#kde4_add_dcop_stubs(kmailprivate_LIB_SRCS ${CMAKE_SOURCE_DIR}/korganizer/korganizeriface.h )

kde4_add_kcfg_files(kmailprivate_LIB_SRCS globalsettings_base.kcfgc replyphrases.kcfgc custommimeheader.kcfgc )

kde4_add_library(kmailprivate SHARED ${kmailprivate_LIB_SRCS})

target_link_libraries(kmailprivate  ${KDE4_KDE3SUPPORT_LIBS} khtml kmime kpgp kdepim kpimidentities mimelib ksieve kleopatra ${KDE4_KCAL_LIBS} phononcore )

set_target_properties(kmailprivate PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS kmailprivate  DESTINATION ${LIB_INSTALL_DIR})


########### next target ###############

set(kcm_kmail_PART_SRCS kcm_kmail.cpp )

kde4_automoc(${kcm_kmail_PART_SRCS})

kde4_add_plugin(kcm_kmail ${kcm_kmail_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kcm_kmail )

target_link_libraries(kcm_kmail  ${KDE4_KDECORE_LIBS} kmailprivate )

install(TARGETS kcm_kmail  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kmailpart_PART_SRCS kmail_part.cpp )

kde4_automoc(${kmailpart_PART_SRCS})
qt4_add_dbus_adaptor( kmailpart_PART_SRCS org.kde.kmail.mailcomposer.xml kmcomposewin.h KMComposeWin )

kde4_add_plugin(kmailpart WITH_PREFIX ${kmailpart_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kmailpart )

target_link_libraries(kmailpart  ${KDE4_KDECORE_LIBS} kmailprivate )

install(TARGETS kmailpart  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kmail_bodypartformatter_application_octetstream_PART_SRCS app_octetstream.cpp )

kde4_automoc(${kmail_bodypartformatter_application_octetstream_PART_SRCS})

kde4_add_plugin(kmail_bodypartformatter_application_octetstream WITH_PREFIX ${kmail_bodypartformatter_application_octetstream_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kmail_bodypartformatter_application_octetstream )

target_link_libraries(kmail_bodypartformatter_application_octetstream  ${KDE4_KDECORE_LIBS} )

install(TARGETS kmail_bodypartformatter_application_octetstream  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kmail_SRCS main.cpp )

kde4_automoc(${kmail_SRCS})

kde4_add_executable(kmail ${kmail_SRCS})

target_link_libraries(kmail  ${KDE4_KDECORE_LIBS} kmailprivate )

install(TARGETS kmail  DESTINATION ${BIN_INSTALL_DIR} )


########### next target ###############

set(dcoptest_SRCS dcoptest.cpp )

kde4_automoc(${dcoptest_SRCS})
qt4_add_dbus_adaptor( dcoptest_SRCS org.kde.kmail.mailcomposer.xml kmcomposewin.h KMComposeWin )
qt4_add_dbus_interfaces( dcoptest_SRCS org.kde.kmail.mailcomposer.xml )

#kde4_add_dcop_skels(dcoptest_SRCS kmailIface.h )

#kde4_add_dcop_stubs(dcoptest_SRCS kmailIface.h )

if(KDE4_BUILD_TESTS)

#kde4_add_executable(dcoptest ${dcoptest_SRCS})

#target_link_libraries(dcoptest  ${KDE4_KIO_LIBS} )

endif(KDE4_BUILD_TESTS)

########### next target ###############

set(recipienteditortest_SRCS recipientseditortest.cpp )

kde4_automoc(${recipienteditortest_SRCS})

if(KDE4_BUILD_TESTS)

kde4_add_executable(recipienteditortest ${recipienteditortest_SRCS})

target_link_libraries(recipienteditortest  ${KDE4_KIO_LIBS} kmailprivate kdepim )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install( FILES KMail.desktop kmail_view.desktop  DESTINATION ${XDG_APPS_DIR})
install( FILES kmail.kcfg replyphrases.kcfg custommimeheader.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kmail.antispamrc kmail.antivirusrc  DESTINATION ${CONFIG_INSTALL_DIR})
install( FILES tips  DESTINATION ${DATA_INSTALL_DIR}/kmail)
install( FILES kmail_config_misc.desktop kmail_config_appearance.desktop     kmail_config_identity.desktop kmail_config_accounts.desktop kmail_config_composer.desktop     kmail_config_security.desktop  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES kmcomposerui.rc kmmainwin.rc kmreadermainwin.rc eventsrc kmail_part.rc  DESTINATION ${DATA_INSTALL_DIR}/kmail)
install( FILES dcopmail.desktop dcopimap.desktop  DESTINATION ${SERVICETYPES_INSTALL_DIR})
install( FILES kmailIface.h kmailpartIface.h kmailicalIface.h  DESTINATION ${INCLUDE_INSTALL_DIR} )
install( FILES application_octetstream.desktop  DESTINATION ${DATA_INSTALL_DIR}/kmail/plugins/bodypartformatter)
install( FILES kmail.upd  DESTINATION ${KCONF_UPDATE_INSTALL_DIR})
install(PROGRAMS upgrade-transport.pl kmail-pgpidentity.pl upgrade-signature.pl kmail-upd-identities.pl kmail-3.1-use-UOID-for-identities.pl kmail-3.1-update-new-mail-notification-settings.pl kmail-3.1.4-dont-use-UOID-0-for-any-identity.pl kmail-3.2-update-loop-on-goto-unread-settings.sh kmail-3.2-misc.sh kmail-3.3-use-ID-for-accounts.pl kmail-3.3-move-identities.pl kmail-3.3-aegypten.pl kmail-3.3-split-sign-encr-keys.sh kmail-3.3-misc.pl kmail-3.3b1-misc.pl kmail-3.4-misc.pl kmail-3.4.1-update-status-filters.pl DESTINATION ${KCONF_UPDATE_INSTALL_DIR})


kde4_install_icons( ${ICON_INSTALL_DIR}  hicolor )




#original Makefile.am contents follow:

##KDE_OPTIONS = nofinal
#KDE_CXXFLAGS = $(USE_RTTI)
#
#SUBDIRS = interfaces . about pics profiles avscripts tests
#
#INCLUDES = -I$(top_srcdir)/libkmime \
#        -I$(top_srcdir)/libkpgp \
#         \
#        -I$(top_srcdir)/libkdepim \
#        -I$(top_srcdir)/libkpimidentities \
#        -I$(top_srcdir)/libemailfunctions \
#        -I$(top_srcdir)/libksieve \
#        -I$(top_srcdir)/mimelib \
#        -I$(top_srcdir)/libkleo \
#        -I$(top_srcdir)/indexlib \
#        -I$(top_srcdir) \
#        $(GPGME_CFLAGS) \
#        $(all_includes)
#
#if add_indexlib
#INDEXLIB=../indexlib/libindex.la
#endif
#
#lib_LTLIBRARIES = libkmailprivate.la
#libkmailprivate_la_LDFLAGS = $(all_libraries) -avoid-version -no-undefined
#libkmailprivate_la_LIBADD  = $(LIB_KHTML)  $(LIB_KABC) $(LIB_KDE3SUPPORT) \
#        ../libkmime/libkmime.la ../libkpgp/libkpgp.la ../libkdepim/libkdepim.la \
#        ../libkpimidentities/libkpimidentities.la ../mimelib/libmimelib.la \
#        ../libksieve/libksieve.la ../libemailfunctions/libemailfunctions.la \
#        ../libkleo/libkleopatra.la $(INDEXLIB) \
#        ../libkcal/libkcal.la
#
#kde_module_LTLIBRARIES = kcm_kmail.la libkmailpart.la libkmail_bodypartformatter_application_octetstream.la
#libkmailpart_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version -no-undefined
#libkmailpart_la_LIBADD  = libkmailprivate.la
#
#kcm_kmail_la_SOURCES = kcm_kmail.cpp
#kcm_kmail_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined
#kcm_kmail_la_LIBADD = libkmailprivate.la $(LIB_KDECORE)
#
#libkmail_bodypartformatter_application_octetstream_la_SOURCES = app_octetstream.cpp
#libkmail_bodypartformatter_application_octetstream_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#linkdir = $(kde_datadir)/kmail/plugins/bodypartformatter
#link_DATA = application_octetstream.desktop
#
#bin_PROGRAMS = kmail
#kmail_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#kmail_LDADD   = libkmailprivate.la
#
#libkmailprivate_la_SOURCES = kmmessage.cpp kmmainwin.cpp configuredialog.cpp \
#                configuredialog_p.cpp klistviewindexedsearchline.cpp \
#                simplestringlisteditor.cpp index.cpp  \
#                identitydrag.cpp identitylistview.cpp identitydialog.cpp \
#                kmfolderdia.cpp kmfoldertree.cpp kmtransport.cpp \
#                kmfoldercombobox.cpp kmaccount.cpp kmheaders.cpp \
#                headeritem.cpp listjob.cpp \
#                kmcomposewin.cpp kmfolder.cpp kmmsgpartdlg.cpp \
#                kmreaderwin.cpp htmlstatusbar.cpp kmmsgdict.cpp \
#                kmgroupware.cpp folderstorage.cpp \
#                csshelper.cpp klistboxdialog.cpp \
#                signatureconfigurationdialog.ui \
#                encryptionconfigurationdialog.ui \
#                actionscheduler.cpp messageproperty.cpp \
#                kmmsgpart.cpp kmmsginfo.cpp \
#                accountmanager.cpp kmacctfolder.cpp kmdict.cpp \
#                kmsystemtray.cpp kmacctlocal.cpp kmfolderdir.cpp \
#                kmfoldermgr.cpp kmfoldernode.cpp kmsender.cpp \
#                kmacctseldlg.cpp kmfiltermgr.cpp kmsearchpatternedit.cpp \
#                kmfilteraction.cpp kmsearchpattern.cpp \
#                kmfolderseldlg.cpp kmfilter.cpp kmfilterdlg.cpp \
#                kmmsgbase.cpp kmmsglist.cpp kmaddrbook.cpp  \
#                signatureconfigurator.cpp xfaceconfigurator.cpp \
#                networkaccount.cpp imapaccountbase.cpp \
#                kmservertest.cpp kmacctimap.cpp kmacctcachedimap.cpp \
#                kmfawidgets.cpp kmfoldermbox.cpp kmfolderimap.cpp \
#                undostack.cpp kmfoldercachedimap.cpp \
#                kmfoldermaildir.cpp popaccount.cpp colorlistbox.cpp \
#                kmkernel.cpp kmailIface.skel kmailicalIface.skel \
#                accountdialog.cpp searchwindow.cpp vcardviewer.cpp \
#                vacationdialog.cpp vacation.cpp sievedebugdialog.cpp \
#                sieveconfig.cpp sievejob.cpp \
#                kmpopheaders.cpp kmpopfiltercnfrmdlg.cpp \
#                kmmimeparttree.cpp \
#                mailinglist-magic.cpp kmacctmaildir.cpp \
#                attachmentstrategy.cpp \
#                headerstrategy.cpp headerstyle.cpp khtmlparthtmlwriter.cpp \
#                filehtmlwriter.cpp teehtmlwriter.cpp \
#                mailcomposerIface.skel objecttreeparser.cpp \
#                attachmentcollector.cpp \
#                bodypartformatter.cpp bodypartformatterfactory.cpp \
#                partNode.cpp signatureconfigurationdialogimpl.cpp \
#                encryptionconfigurationdialogimpl.cpp mailsourceviewer.cpp \
#                kmcommands.cpp kmreadermainwin.cpp \
#                kmstartup.cpp kmmainwidget.cpp \
#                kmfolderindex.cpp \
#                kmfoldersearch.cpp transportmanager.cpp \
#                folderjob.cpp cachedimapjob.cpp \
#                maildirjob.cpp mboxjob.cpp imapjob.cpp \
#                subscriptiondialog.cpp kmailicalifaceimpl.cpp aboutdata.cpp \
#                folderIface.cpp folderIface.skel mailserviceimpl.cpp \
#                attachmentlistview.cpp kmedit.cpp kmlineeditspell.cpp \
#                kmatmlistview.cpp composer.cpp \
#                isubject.cpp bodyvisitor.cpp antispamwizard.cpp \
#                urlhandlermanager.cpp dictionarycombobox.cpp \
#                secondarywindow.cpp filterlog.cpp filterlogdlg.cpp \
#                korganizeriface.stub messagecomposer.cpp \
#                keyresolver.cpp globalsettings.cpp globalsettings_base.kcfgc \
#                regexplineedit.cpp rulewidgethandlermanager.cpp \
#                headerlistquicksearch.cpp acljobs.cpp folderdiaacltab.cpp \
#                partnodebodypart.cpp \
#                expirejob.cpp compactionjob.cpp jobscheduler.cpp callback.cpp \
#                searchjob.cpp renamejob.cpp \
#                composercryptoconfiguration.ui \
#                warningconfiguration.ui smimeconfiguration.ui annotationjobs.cpp \
#                accountcombobox.cpp redirectdialog.cpp foldershortcutdialog.cpp \
#                folderrequester.cpp \
#                spamheaderanalyzer.cpp antispamconfig.cpp \
#                replyphrases.kcfgc custommimeheader.kcfgc \
#                recipientseditor.cpp \
#                recipientspicker.cpp kwindowpositioner.cpp \
#                distributionlistdialog.cpp expirypropertiesdialog.cpp \
#                mailinglistpropertiesdialog.cpp newfolderdialog.cpp \
#                accountwizard.cpp textsource.cpp \
#                managesievescriptsdialog.cpp chiasmuskeyselector.cpp \
#                util.cpp
#
#kmail_SOURCES = main.cpp
#
#libkmailpart_la_SOURCES =  kmailpartIface.skel  kmail_part.cpp
#
#check_PROGRAMS = dcoptest recipienteditortest
#
#METASOURCES = AUTO
#
#dcoptest_SOURCES = dcoptest.cpp kmailIface.skel kmailIface.stub mailcomposerIface.skel mailcomposerIface.stub
#dcoptest_LDADD = $(LIB_KIO)
#dcoptest_LDFLAGS = $(all_libraries)
#
#recipienteditortest_SOURCES = recipientseditortest.cpp
#recipienteditortest_LDADD = libkmailprivate.la ../libkdepim/libkdepim.la $(LIB_KIO) $(LIB_KABC)
#recipienteditortest_LDFLAGS = $(all_libraries)
#
#korganizeriface_DIR = $(top_srcdir)/korganizer
#
#include_HEADERS = kmailIface.h kmailpartIface.h kmailicalIface.h
#
#xdg_apps_DATA = KMail.desktop kmail_view.desktop
#
## why?
#EXTRA_DIST = KMail.desktop $(link_DATA)
#
#KDE_ICON = AUTO
#
#rcdir = $(kde_datadir)/kmail
#rc_DATA = kmcomposerui.rc kmmainwin.rc kmreadermainwin.rc eventsrc kmail_part.rc
#
#updatedir = $(kde_datadir)/kconf_update
#update_DATA = kmail.upd
#update_SCRIPTS = upgrade-transport.pl kmail-pgpidentity.pl \
#                upgrade-signature.pl kmail-upd-identities.pl \
#                kmail-3.1-use-UOID-for-identities.pl \
#                kmail-3.1-update-new-mail-notification-settings.pl \
#                kmail-3.1.4-dont-use-UOID-0-for-any-identity.pl \
#                kmail-3.2-update-loop-on-goto-unread-settings.sh \
#                kmail-3.2-misc.sh \
#                kmail-3.3-use-ID-for-accounts.pl \
#                kmail-3.3-move-identities.pl \
#                kmail-3.3-aegypten.pl \
#                kmail-3.3-split-sign-encr-keys.sh \
#                kmail-3.3-misc.pl \
#                kmail-3.3b1-misc.pl \
#                kmail-3.4-misc.pl \
#                kmail-3.4.1-update-status-filters.pl
#
#confdir = $(kde_confdir)
#conf_DATA = kmail.antispamrc kmail.antivirusrc
#
#tipdir = $(kde_datadir)/kmail
#tip_DATA = tips
#
#servicetypedir = $(kde_servicetypesdir)
#servicetype_DATA = dcopmail.desktop dcopimap.desktop
#
#kde_services_DATA = kmail_config_misc.desktop kmail_config_appearance.desktop \
#   kmail_config_identity.desktop kmail_config_accounts.desktop kmail_config_composer.desktop \
#   kmail_config_security.desktop
#
#messages: rc.cpp
#	rm -f tips.cpp
#	$(PREPARETIPS) > tips.cpp
#	$(XGETTEXT) -ktranslate *.cpp *.h -o $(podir)/kmail.pot
#	rm -f tips.cpp
#
#kde_kcfg_DATA = kmail.kcfg replyphrases.kcfg custommimeheader.kcfg
#
#DOXYGEN_REFERENCES = kdeui
#include $(top_srcdir)/admin/Doxyfile.am
