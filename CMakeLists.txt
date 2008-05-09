project(kmail)

INCLUDE(CheckIncludeFiles)
check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(byteswap.h HAVE_BYTESWAP_H)
check_include_files(paths.h HAVE_PATHS_H)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${KDE4_DATA_DIR}/cmake/modules)

check_include_files(sys/inotify.h SYS_INOTIFY_H_FOUND)
macro_bool_to_01(SYS_INOTIFY_H_FOUND HAVE_SYS_INOTIFY_H)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")
  if ( NOT KdeSubversion_FOUND )
    find_package( KdeSubversion )
  endif ( NOT KdeSubversion_FOUND )
  if ( KdeSubversion_FOUND )
    KdeSubversion_WC_INFO( ${PROJECT_SOURCE_DIR} KMail )
    string( SUBSTRING "${KMail_WC_LAST_CHANGED_DATE}" 0 10 KMail_WC_LAST_CHANGED_DATE )
    set( kmail_svn_revision "svn-${KMail_WC_REVISION}" )
    set( kmail_svn_last_change "${KMail_WC_LAST_CHANGED_DATE}" )
  endif ( KdeSubversion_FOUND )
endif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.svn")
configure_file(config-kmail.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-kmail.h )

add_definitions ( -DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS )
add_definitions ( -DKDE_DEFAULT_DEBUG_AREA=5006 )
add_definitions ( -DStorageDebug=5120 )

qt4_generate_dbus_interface( kmkernel.h org.kde.kmail.kmail.xml OPTIONS -a )
qt4_generate_dbus_interface( kmcomposewin.h org.kde.kmail.mailcomposer.xml OPTIONS -a )

add_custom_target(kmail_xml ALL
                  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmail.kmail.xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmail.mailcomposer.xml
                  COMMENT "Helper target for XML stuff. The Kontact plugin, KOrganizer and others depend on it." )

add_subdirectory( interfaces )
add_subdirectory( about )
add_subdirectory( pics )
add_subdirectory( profiles )
add_subdirectory( avscripts )
add_subdirectory( tests )
add_subdirectory( kconf_update )
include_directories(
  ${CMAKE_SOURCE_DIR}/libkdepim ${CMAKE_BINARY_DIR}/libkdepim
  ${CMAKE_SOURCE_DIR}/libksieve
  ${CMAKE_SOURCE_DIR}/mimelib
  ${CMAKE_SOURCE_DIR}/libkleo
  ${GPGME_INCLUDES}
  ${Boost_INCLUDE_DIR}
   )
if(Nepomuk_FOUND)
  include_directories( ${NEPOMUK_INCLUDES} )
endif(Nepomuk_FOUND)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

########### next target ###############

set(kmailprivate_LIB_SRCS
   kmmessage.cpp
   kmmainwin.cpp
   configuredialog.cpp
   configuredialog_p.cpp
   simplestringlisteditor.cpp
   identitylistview.cpp
   identitydialog.cpp
   kmfolderdialog.cpp
   kmfoldertree.cpp
   kmfoldercombobox.cpp
   foldertreebase.cpp
   favoritefolderview.cpp
   kmaccount.cpp
   kmheaders.cpp
   headeritem.cpp
   listjob.cpp
   kmcomposewin.cpp
   snippetdlg.cpp
   snippetwidget.cpp
   snippetitem.cpp
   kmfolder.cpp
   kmmsgpartdlg.cpp
   kmreaderwin.cpp
   htmlstatusbar.cpp
   kmmsgdict.cpp
   kmgroupware.cpp
   folderstorage.cpp
   csshelper.cpp
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
   filterimporterexporter.cpp
   kmsearchpatternedit.cpp
   kmfilteraction.cpp
   kmsearchpattern.cpp
   folderselectiontreewidget.cpp
   folderselectiondialog.cpp
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
   mailsourceviewer.cpp
   kmcommands.cpp
   kmreadermainwin.cpp
   kmstartup.cpp
   kmmainwidget.cpp
   kmfolderindex.cpp
   kmfoldersearch.cpp
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
   kmcomposereditor.cpp
   kmlineeditspell.cpp
   kmatmlistview.cpp
   composer.cpp
   isubject.cpp
   bodyvisitor.cpp
   antispamwizard.cpp
   urlhandlermanager.cpp
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
   folderdialogacltab.cpp
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
   kmmessagetag.cpp
   templateparser.cpp
   templatesconfiguration.cpp
   templatesinsertcommand.cpp
   customtemplates.cpp
   customtemplatesmenu.cpp
   quotajobs.cpp
   folderdialogquotatab.cpp
   folderdialogquotatab_p.cpp
   folderadaptor.cpp
   copyfolderjob.cpp
   messagecopyhelper.cpp
   korghelper.cpp
   localsubscriptiondialog.cpp
   editorwatcher.cpp
   scalix.cpp
   messageactions.cpp
   statusbarlabel.cpp
   groupware_types.cpp
   kleojobexecutor.cpp
#TODO   locale.c
)

qt4_add_dbus_adaptor( kmailprivate_LIB_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml kmkernel.h KMKernel )
qt4_add_dbus_adaptor( kmailprivate_LIB_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.mailcomposer.xml kmcomposewin.h KMComposeWin )
qt4_add_dbus_adaptor( kmailprivate_LIB_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.kmail.groupware.xml kmailicalifaceimpl.h KMailICalIfaceImpl )

qt4_add_dbus_interfaces(kmailprivate_LIB_SRCS
  ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml
  ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.mailcomposer.xml
  ${CMAKE_SOURCE_DIR}/korganizer/org.kde.Korganizer.Calendar.xml
)

qt4_add_dbus_adaptor( kmailprivate_LIB_SRCS ${CMAKE_SOURCE_DIR}/libkdepim/interfaces/org.kde.mailtransport.service.xml mailserviceimpl.h KMail::MailServiceImpl)



kde4_add_ui_files(kmailprivate_LIB_SRCS composercryptoconfiguration.ui warningconfiguration.ui smimeconfiguration.ui templatesconfiguration_base.ui customtemplates_base.ui snippetdlgbase.ui )

kde4_add_kcfg_files(kmailprivate_LIB_SRCS globalsettings_base.kcfgc replyphrases.kcfgc custommimeheader.kcfgc templatesconfiguration_kfg.kcfgc customtemplates_kfg.kcfgc )

if(WIN32)
  find_package(Sqlite)
  if (NOT SQLITE_FOUND)
    MESSAGE(FATAL_ERROR "You need SQLite library for indexing")
  endif(NOT SQLITE_FOUND)
  add_definitions(-DKMAIL_SQLITE_INDEX) # to alter declaration of KMFolderIndex
  include_directories(${SQLITE_INCLUDE_DIR})
endif(WIN32)

kde4_add_library(kmailprivate SHARED ${kmailprivate_LIB_SRCS})
kdepim4_link_unique_libraries(kmailprivate ${KDE4_KHTML_LIBRARY} ${KDE4_THREADWEAVER_LIBRARY} ${KDE4_KMIME_LIBRARY} ${KDE4_KPIMIDENTITIES_LIBRARY} ${KDE4_PHONON_LIBRARY} ${KDE4_KNOTIFYCONFIG_LIBRARY} ${KDE4_KTNEF_LIBRARY} ${KDE4_KUTILS_LIBRARY} ${KDE4_MAILTRANSPORT_LIBRARY} ${KDE4_KIMAP_LIBRARY} ${KDE4_KPARTS_LIBRARY} kleo ${QGPGME_LIBRARIES} mimelib ksieve kpgp kdepim ${QT_QT3SUPPORT_LIBRARY})
if(WIN32)
  kdepim4_link_unique_libraries(kmailprivate ${SQLITE_LIBRARIES})
endif(WIN32)

if(Nepomuk_FOUND)
   kdepim4_link_unique_libraries(kmailprivate ${NEPOMUK_LIBRARIES} )
endif(Nepomuk_FOUND)

set_target_properties(kmailprivate PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION} )
install(TARGETS kmailprivate  DESTINATION ${LIB_INSTALL_DIR})


########### next target ###############

set(kcm_kmail_PART_SRCS kcm_kmail.cpp )


kde4_add_plugin(kcm_kmail ${kcm_kmail_PART_SRCS})



kdepim4_link_unique_libraries(kcm_kmail  ${KDE4_KDECORE_LIBS} kmailprivate )

install(TARGETS kcm_kmail  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kmailpart_PART_SRCS kmail_part.cpp )

qt4_generate_dbus_interface( kmail_part.h org.kde.kmail.kmailpart.xml OPTIONS -a )
qt4_add_dbus_adaptor( kmailpart_PART_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmailpart.xml kmail_part.h KMailPart )
qt4_add_dbus_interfaces( kmailpart_PART_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmailpart.xml )

kde4_add_plugin(kmailpart ${kmailpart_PART_SRCS})

kdepim4_link_unique_libraries(kmailpart  ${KDE4_KDECORE_LIBS} kmailprivate )

install(TARGETS kmailpart  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kmail_bodypartformatter_application_octetstream_PART_SRCS app_octetstream.cpp )


kde4_add_plugin(kmail_bodypartformatter_application_octetstream ${kmail_bodypartformatter_application_octetstream_PART_SRCS})



kdepim4_link_unique_libraries(kmail_bodypartformatter_application_octetstream  ${KDE4_KDECORE_LIBS} )

install(TARGETS kmail_bodypartformatter_application_octetstream  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kmail_SRCS main.cpp )

# todo: use KMail icon
kde4_add_app_icon(kmail_SRCS "${KDE4_ICON_DIR}/oxygen/*/apps/internet-mail.png")

kde4_add_executable(kmail ${kmail_SRCS})

kdepim4_link_unique_libraries(kmail  ${KDE4_KDECORE_LIBS} kmailprivate )

install(TARGETS kmail  DESTINATION ${BIN_INSTALL_DIR} )



########### install files ###############

install( FILES KMail.desktop kmail_view.desktop  DESTINATION ${XDG_APPS_INSTALL_DIR})
install( FILES kmail.kcfg replyphrases.kcfg custommimeheader.kcfg customtemplates_kfg.kcfg templatesconfiguration_kfg.kcfg DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kmail.antispamrc kmail.antivirusrc  DESTINATION ${CONFIG_INSTALL_DIR})
install( FILES tips  DESTINATION ${DATA_INSTALL_DIR}/kmail)
install( FILES kmail_config_misc.desktop kmail_config_appearance.desktop     kmail_config_identity.desktop kmail_config_accounts.desktop kmail_config_composer.desktop     kmail_config_security.desktop  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES kmcomposerui.rc kmmainwin.rc kmreadermainwin.rc kmail.notifyrc kmail_part.rc  DESTINATION ${DATA_INSTALL_DIR}/kmail)
install( FILES dbusmail.desktop dbusimap.desktop  DESTINATION ${SERVICETYPES_INSTALL_DIR})
install( FILES application_octetstream.desktop  DESTINATION ${DATA_INSTALL_DIR}/kmail/plugins/bodypartformatter)

# Install the D-Bus interface XML files
install( FILES
         ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmailpart.xml
         ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml
         ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.mailcomposer.xml
         DESTINATION
         ${KDE4_DBUS_INTERFACES_DIR} )

kde4_install_icons( ${ICON_INSTALL_DIR} )

