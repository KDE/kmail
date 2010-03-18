project(kmail)

include(CheckIncludeFiles)

add_definitions(-DQT3_SUPPORT_WARNINGS)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${KDE4_DATA_DIR}/cmake/modules)

qt4_generate_dbus_interface(kmkernel.h org.kde.kmail.kmail.xml OPTIONS -a)

add_custom_target(kmail_xml ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmail.kmail.xml
  COMMENT "Helper target for XML stuff. The Kontact plugin, KOrganizer and others depend on it."
)

include_directories(
  ${CMAKE_SOURCE_DIR}/libkdepim
  ${CMAKE_BINARY_DIR}/libkdepim
  ${CMAKE_SOURCE_DIR}/libksieve
  ${CMAKE_SOURCE_DIR}/libkleo
  ${CMAKE_SOURCE_DIR}/messagelist
  ${CMAKE_SOURCE_DIR}/messageviewer
  ${CMAKE_SOURCE_DIR}/korganizer
  ${GPGME_INCLUDES}
  ${Boost_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/akonadi/
  ${CMAKE_CURRENT_BINARY_DIR}/../ontologies
  ${NEPOMUK_INCLUDES}
  ${SOPRANO_INCLUDE_DIR} 
 )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")


add_subdirectory(about)
add_subdirectory(pics)
add_subdirectory(icons)
add_subdirectory(avscripts)
add_subdirectory(tests)
add_subdirectory(kconf_update)

########### kmailprivate ###############

set(kmailprivate_LIB_SRCS
   kmagentmanager.cpp
   kmagentinstance.cpp
   foldercollectionmonitor.cpp
   readablecollectionproxymodel.cpp
   folderselectiontreeview.cpp
   folderselectiontreeviewdialog.cpp
   foldertreeview.cpp
   collectionannotationsattribute.cpp
   collectiontemplatespage.cpp
   collectionmaintenancepage.cpp
   collectiongeneralpage.cpp
   collectionviewpage.cpp
   collectionquotapage.cpp
   collectionquotapage_p.cpp
   collectionaclpage.cpp
   colorlistbox.cpp
   imapaclattribute.cpp
   messagehelper.cpp
   messageinfo.cpp
   foldercollection.cpp
   kmmainwin.cpp
   configuredialoglistview.cpp
   configuredialog.cpp
   configuredialog_p.cpp
   simplestringlisteditor.cpp
   identitylistview.cpp
   identitydialog.cpp
   globalsettings.cpp
   snippetdlg.cpp
   snippetwidget.cpp
   snippetitem.cpp
   kmreaderwin.cpp
   actionscheduler.cpp
   messageproperty.cpp
   kmsystemtray.cpp
   akonadisender.cpp
   kmfiltermgr.cpp
   filterimporterexporter.cpp
   kmsearchpatternedit.cpp
   kmfilteraction.cpp
   kmsearchpattern.cpp
   kmfilter.cpp
   kmfilterdlg.cpp
   xfaceconfigurator.cpp
   kmfawidgets.cpp
   undostack.cpp
   kmkernel.cpp
   searchwindow.cpp
   vacationdialog.cpp
   vacation.cpp
   sievedebugdialog.cpp
   sievejob.cpp
   mailinglist-magic.cpp
   attachmentcollector.cpp
   kmcommands.cpp
   kmreadermainwin.cpp
   kmstartup.cpp
   kmmainwidget.cpp
   folderjob.cpp
   aboutdata.cpp
   mailserviceimpl.cpp
   kmcomposereditor.cpp
   kmlineeditspell.cpp
   composer.cpp
   isubject.cpp
   antispamwizard.cpp
   secondarywindow.cpp
   filterlog.cpp
   filterlogdlg.cpp
   codecaction.cpp
   codecmanager.cpp
   keyresolver.cpp
   regexplineedit.cpp
   rulewidgethandlermanager.cpp
   expirejob.cpp
   compactionjob.cpp
   jobscheduler.cpp
   redirectdialog.cpp
   foldershortcutdialog.cpp
   folderrequester.cpp
   recipientseditor.cpp
   recipientspicker.cpp
   kwindowpositioner.cpp
   distributionlistdialog.cpp
   expirypropertiesdialog.cpp
   mailinglistpropertiesdialog.cpp
   managesievescriptsdialog.cpp
   util.cpp
   templateparser.cpp
   templatesconfiguration.cpp
   templatesinsertcommand.cpp
   customtemplates.cpp
   customtemplatesmenu.cpp
   korghelper.cpp
   messageactions.cpp
   statusbarlabel.cpp
   stringutil.cpp
   kmcomposewin.cpp
   attachmentcontroller.cpp
   attachmentmodel.cpp
   attachmentview.cpp
   attachmentfrompublickeyjob.cpp
   mdnadvicedialog.cpp
   backupjob.cpp
   archivefolderdialog.cpp
   tagging.cpp
)

soprano_add_ontology(kmailprivate_LIB_SRCS
  ${CMAKE_SOURCE_DIR}/messagecore/messagetag.trig
  "MessageTag"
  "Vocabulary"
  "trig"
)

macro(add_resource_iface _kcfgFile _ifaceName _className)
  kcfg_generate_dbus_interface(${CMAKE_CURRENT_SOURCE_DIR}/${_kcfgFile} ${_ifaceName})
  string(TOLOWER ${_className} _codeFile)
  set_source_files_properties( ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml PROPERTIES INCLUDE "metatype.h")
  qt4_add_dbus_interface(kmailprivate_LIB_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/${_ifaceName}.xml ${_codeFile} ${_className}
  )
endmacro(add_resource_iface)

add_resource_iface( imapresource.kcfg org.kde.Akonadi.Imap.Settings ImapSettings )

# TODO PORT
# qt4_generate_dbus_interface(kmcomposewin.h org.kde.kmail.mailcomposer.xml OPTIONS -a)
# qt4_add_dbus_adaptor(kmailprivate_LIB_SRCS
#   ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.mailcomposer.xml kmcomposewin.h KMComposeWin
# )

qt4_add_dbus_adaptor(kmailprivate_LIB_SRCS
  ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml kmkernel.h KMKernel
)
qt4_add_dbus_adaptor(kmailprivate_LIB_SRCS
  ${CMAKE_SOURCE_DIR}/libkdepim/interfaces/org.kde.mailtransport.service.xml mailserviceimpl.h
  KMail::MailServiceImpl
)

qt4_add_dbus_interfaces(kmailprivate_LIB_SRCS
  ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml
  ${CMAKE_SOURCE_DIR}/korganizer/org.kde.Korganizer.Calendar.xml
)

kde4_add_ui_files(kmailprivate_LIB_SRCS
  ui/composercryptoconfiguration.ui
  ui/warningconfiguration.ui
  ui/smimeconfiguration.ui
  ui/templatesconfiguration_base.ui
  ui/customtemplates_base.ui
  ui/snippetdlgbase.ui
  ui/miscpagemaintab.ui
  ui/miscpageinvitetab.ui
  ui/securitypagegeneraltab.ui
  ui/identitypage.ui
  ui/accountspagereceivingtab.ui
)

# KCFG files. The main kmail.kcfg is configured by CMake and put
# in the build directory.

if(KDEPIM_ENTERPRISE_BUILD)
  set(WARN_TOOMANY_RECIPIENTS_DEFAULT true)
  set(DELETE_INVITATIONS_AFTER_REPLY_DEFAULT true)
  set(ALLOW_SEMICOLON_AS_ADDRESS_SEPARATOR_DEFAULT true)
else(KDEPIM_ENTERPRISE_BUILD)
  set(WARN_TOOMANY_RECIPIENTS_DEFAULT false)
  set(DELETE_INVITATIONS_AFTER_REPLY_DEFAULT false)
  set(ALLOW_SEMICOLON_AS_ADDRESS_SEPARATOR_DEFAULT false)
endif(KDEPIM_ENTERPRISE_BUILD)

configure_file(kmail.kcfg.cmake ${CMAKE_CURRENT_BINARY_DIR}/kmail.kcfg)

kde4_add_kcfg_files(kmailprivate_LIB_SRCS
  globalsettings_base.kcfgc
  custommimeheader.kcfgc
  templatesconfiguration_kfg.kcfgc
  customtemplates_kfg.kcfgc
)


kde4_add_library(kmailprivate SHARED ${kmailprivate_LIB_SRCS})
target_link_libraries(kmailprivate
  ${KDE4_THREADWEAVER_LIBRARY}
  ${KDEPIMLIBS_KMIME_LIBRARY}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
  messagecore
  messagecomposer
  messagelist
  ${KDEPIMLIBS_KPIMIDENTITIES_LIBRARY}
  ${KDEPIMLIBS_KONTACTINTERFACE_LIBS}
  ${KDE4_PHONON_LIBRARY}
  ${KDE4_KNOTIFYCONFIG_LIBRARY}
  ${KDEPIMLIBS_KTNEF_LIBRARY}
  ${KDEPIMLIBS_MAILTRANSPORT_LIBRARY}
  ${KDEPIMLIBS_KIMAP_LIBRARY}
  ${KDE4_KPARTS_LIBRARY}
  ${KDE4_KRESOURCES_LIBRARY}
  ${KDE4_KUTILS_LIBS}
  kleo
  ${QGPGME_LIBRARIES}
  ksieve
  kpgp
  kdepim
  ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_KABC_LIBS}
  ${KDEPIMLIBS_KPIMUTILS_LIBS}
  ${KDEPIMLIBS_KPIMTEXTEDIT_LIBS}
  messageviewer
  akonadi_next
  ${SOPRANO_LIBRARIES}
  niefast_apps
  ${QT_QTWEBKIT_LIBRARY}
  ${NEPOMUK_LIBRARIES}
  ${NEPOMUK_QUERY_LIBRARIES}
)

set_target_properties(kmailprivate
  PROPERTIES VERSION ${GENERIC_LIB_VERSION} SOVERSION ${GENERIC_LIB_SOVERSION}
)

########### kcm_kmail ###############

set(kcm_kmail_PART_SRCS kcm_kmail.cpp )
kde4_add_plugin(kcm_kmail ${kcm_kmail_PART_SRCS})
target_link_libraries(kcm_kmail ${KDE4_KDEUI_LIBS} kmailprivate )

########### kmailpart ###############

set(kmailpart_PART_SRCS kmail_part.cpp )
qt4_generate_dbus_interface(kmail_part.h org.kde.kmail.kmailpart.xml OPTIONS -a)
qt4_add_dbus_adaptor(kmailpart_PART_SRCS
  ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmailpart.xml kmail_part.h KMailPart
)
qt4_add_dbus_interfaces(kmailpart_PART_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmailpart.xml)
kde4_add_plugin(kmailpart ${kmailpart_PART_SRCS})
target_link_libraries(kmailpart
  ${KDE4_KDECORE_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  kmailprivate
  ${KDE4_KPARTS_LIBS}
  ${KDE4_KUTILS_LIBS}
)


########### bodypartformatter_application_octetstream ###############

set(kmail_bodypartformatter_application_octetstream_PART_SRCS app_octetstream.cpp )
kde4_add_plugin(kmail_bodypartformatter_application_octetstream
  ${kmail_bodypartformatter_application_octetstream_PART_SRCS}
)
target_link_libraries(kmail_bodypartformatter_application_octetstream ${KDE4_KDECORE_LIBS})


########### KMail executable ###############

set(kmail_SRCS main.cpp)
kde4_add_app_icon(kmail_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/hi*-apps-kmail.png")
kde4_add_executable(kmail ${kmail_SRCS})
target_link_libraries(kmail
  ${KDE4_KDEUI_LIBS}
  ${KDE4_KDECORE_LIBS}
  ${KDEPIMLIBS_AKONADI_KCAL_LIBS}
  ${KDEPIMLIBS_KONTACTINTERFACE_LIBS}
  kmailprivate
  kdepim
)

########### install files ###############

install(TARGETS kmailprivate ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
install(TARGETS kmail ${INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES KMail.desktop kmail_view.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/kmail.kcfg  
  custommimeheader.kcfg
  customtemplates_kfg.kcfg
  templatesconfiguration_kfg.kcfg
  DESTINATION ${KCFG_INSTALL_DIR}
)
install(FILES kmail.antispamrc kmail.antivirusrc  DESTINATION ${CONFIG_INSTALL_DIR})
install(FILES tips  DESTINATION ${DATA_INSTALL_DIR}/kmail)
install(FILES
  kmail_config_misc.desktop
  kmail_config_appearance.desktop
  kmail_config_identity.desktop
  kmail_config_accounts.desktop
  kmail_config_composer.desktop
  kmail_config_security.desktop
  DESTINATION ${SERVICES_INSTALL_DIR}
)
install(FILES
  kmcomposerui.rc
  kmmainwin.rc
  kmreadermainwin.rc
  kmail.notifyrc
  kmail_part.rc
  DESTINATION ${DATA_INSTALL_DIR}/kmail
)
install(FILES dbusmail.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
install(FILES
  application_octetstream.desktop
  DESTINATION ${DATA_INSTALL_DIR}/kmail/plugins/bodypartformatter
)
install(TARGETS
  kmailpart
  kmail_bodypartformatter_application_octetstream
  kcm_kmail
  DESTINATION ${PLUGIN_INSTALL_DIR}
)
install(FILES
  ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmailpart.xml
  ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml
  DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}
)
