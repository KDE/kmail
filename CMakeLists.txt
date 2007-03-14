project(kmail)

check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(byteswap.h HAVE_BYTESWAP_H)
check_include_files(paths.h HAVE_PATHS_H)

configure_file(config-kmail.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kmail.h )


add_subdirectory( interfaces )
add_subdirectory( about )
add_subdirectory( pics )
add_subdirectory( profiles )
add_subdirectory( avscripts )
add_subdirectory( tests )
add_subdirectory( kconf_update )
include_directories( ${CMAKE_SOURCE_DIR}/libkpgp
  ${CMAKE_SOURCE_DIR}/libkdepim ${CMAKE_SOURCE_DIR}/libkpimidentities ${CMAKE_BINARY_DIR}/libkdepim
  ${CMAKE_SOURCE_DIR}/libksieve
  ${CMAKE_SOURCE_DIR}/mimelib ${CMAKE_SOURCE_DIR}/libkleo
  ${GPGME_INCLUDES}
   )


########### next target ###############

set(kmailprivate_LIB_SRCS
   kmmessage.cpp
   kmmainwin.cpp
   configuredialog.cpp
   configuredialog_p.cpp
   simplestringlisteditor.cpp
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
   mailserviceimpl.cpp
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
   templateparser.cpp templatesconfiguration.cpp templatesinsertcommand.cpp
   customtemplates.cpp
   quotajobs.cpp
   folderdiaquotatab.cpp
   folderdiaquotatab_p.cpp
   keditcl1.cpp keditcl2.cpp folderadaptor.cpp 
   copyfolderjob.cpp messagecopyhelper.cpp
   localsubscriptiondialog.cpp
)

qt4_add_dbus_interfaces(kmailprivate_LIB_SRCS ${CMAKE_SOURCE_DIR}/kmail/org.kde.kmail.kmail.xml)
qt4_add_dbus_adaptor( kmailprivate_LIB_SRCS ${CMAKE_SOURCE_DIR}/libkdepim/interfaces/org.kde.mailtransport.service.xml mailserviceimpl.h KMail::MailServiceImpl)  

kde4_automoc(${kmailprivate_LIB_SRCS})

kde4_add_ui3_files(kmailprivate_LIB_SRCS
   signatureconfigurationdialog.ui
   )

kde4_add_ui_files(kmailprivate_LIB_SRCS composercryptoconfiguration.ui warningconfiguration.ui smimeconfiguration.ui encryptionconfigurationdialog.ui templatesconfiguration_base.ui customtemplates_base.ui )

#kde4_add_dcop_skels(kmailprivate_LIB_SRCS
#   kmailIface.h
#   kmailicalIface.h
#   folderIface.h )
qt4_add_dbus_adaptor( kmailprivate_LIB_SRCS org.kde.kmail.mailcomposer.xml kmcomposewin.h KMComposeWin )
qt4_add_dbus_adaptor( kmailprivate_LIB_SRCS org.kde.kmail.kmail.xml kmkernel.h  KMKernel)

#kde4_add_dcop_stubs(kmailprivate_LIB_SRCS ${CMAKE_SOURCE_DIR}/korganizer/korganizeriface.h )

kde4_add_kcfg_files(kmailprivate_LIB_SRCS globalsettings_base.kcfgc replyphrases.kcfgc custommimeheader.kcfgc templatesconfiguration_kfg.kcfgc customtemplates_kfg.kcfgc )

kde4_add_library(kmailprivate SHARED ${kmailprivate_LIB_SRCS})

target_link_libraries(kmailprivate  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KHTML_LIBS} ${KDE4_KMIME_LIBS} kpgp kdepim
  kpimidentities mimelib ksieve kleopatra ${KDE4_KCAL_LIBS} phononcore ${KDE4_KNOTIFYCONFIG_LIBS} ${KDE4_KTNEF_LIBS}
  threadweaver)

set_target_properties(kmailprivate PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install(TARGETS kmailprivate  DESTINATION ${LIB_INSTALL_DIR})


########### next target ###############

set(kcm_kmail_PART_SRCS kcm_kmail.cpp )

kde4_automoc(${kcm_kmail_PART_SRCS})

kde4_add_plugin(kcm_kmail ${kcm_kmail_PART_SRCS})



target_link_libraries(kcm_kmail  ${KDE4_KDECORE_LIBS} kmailprivate )

install(TARGETS kcm_kmail  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kmailpart_PART_SRCS kmail_part.cpp )

kde4_automoc(${kmailpart_PART_SRCS})
qt4_add_dbus_adaptor( kmailpart_PART_SRCS org.kde.kmail.mailcomposer.xml kmcomposewin.h KMComposeWin )
qt4_add_dbus_adaptor( kmailpart_PART_SRCS org.kde.kmail.kmailpart.xml kmail_part.h  KMailPart)

kde4_add_plugin(kmailpart WITH_PREFIX ${kmailpart_PART_SRCS})



target_link_libraries(kmailpart  ${KDE4_KDECORE_LIBS} kmailprivate )

install(TARGETS kmailpart  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kmail_bodypartformatter_application_octetstream_PART_SRCS app_octetstream.cpp )

kde4_automoc(${kmail_bodypartformatter_application_octetstream_PART_SRCS})

kde4_add_plugin(kmail_bodypartformatter_application_octetstream WITH_PREFIX ${kmail_bodypartformatter_application_octetstream_PART_SRCS})



target_link_libraries(kmail_bodypartformatter_application_octetstream  ${KDE4_KDECORE_LIBS} )

install(TARGETS kmail_bodypartformatter_application_octetstream  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kmail_SRCS main.cpp )

kde4_automoc(${kmail_SRCS})

kde4_add_executable(kmail ${kmail_SRCS})

target_link_libraries(kmail  ${KDE4_KDECORE_LIBS} kmailprivate )

install(TARGETS kmail  DESTINATION ${BIN_INSTALL_DIR} )


########### next target ###############
if(KDE4_BUILD_TESTS)
set(dcoptest_SRCS dcoptest.cpp )

kde4_automoc(${dcoptest_SRCS})
qt4_add_dbus_adaptor( dcoptest_SRCS org.kde.kmail.mailcomposer.xml kmcomposewin.h KMComposeWin )
qt4_add_dbus_interfaces( dcoptest_SRCS org.kde.kmail.mailcomposer.xml )

#kde4_add_dcop_skels(dcoptest_SRCS kmailIface.h )

#kde4_add_dcop_stubs(dcoptest_SRCS kmailIface.h )

#kde4_add_executable(dcoptest ${dcoptest_SRCS})

#target_link_libraries(dcoptest  ${KDE4_KIO_LIBS} )


########### next target ###############

set(recipienteditortest_SRCS recipientseditortest.cpp )

kde4_automoc(${recipienteditortest_SRCS})


kde4_add_executable(recipienteditortest ${recipienteditortest_SRCS})

target_link_libraries(recipienteditortest  ${KDE4_KIO_LIBS} kmailprivate kdepim )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install( FILES KMail.desktop kmail_view.desktop  DESTINATION ${XDG_APPS_DIR})
install( FILES kmail.kcfg replyphrases.kcfg custommimeheader.kcfg customtemplates_kfg.kcfg templatesconfiguration_kfg.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kmail.antispamrc kmail.antivirusrc  DESTINATION ${CONFIG_INSTALL_DIR})
install( FILES tips  DESTINATION ${DATA_INSTALL_DIR}/kmail)
install( FILES kmail_config_misc.desktop kmail_config_appearance.desktop     kmail_config_identity.desktop kmail_config_accounts.desktop kmail_config_composer.desktop     kmail_config_security.desktop  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES kmcomposerui.rc kmmainwin.rc kmreadermainwin.rc kmail.notifyrc kmail_part.rc  DESTINATION ${DATA_INSTALL_DIR}/kmail)
install( FILES dbusmail.desktop dbusimap.desktop  DESTINATION ${SERVICETYPES_INSTALL_DIR})
install( FILES application_octetstream.desktop  DESTINATION ${DATA_INSTALL_DIR}/kmail/plugins/bodypartformatter)

install(FILES org.kde.kmail.kmailpart.xml org.kde.kmail.mailcomposer.xml DESTINATION ${DBUS_INTERFACES_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR} )

