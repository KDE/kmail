project(kmail)

include(CheckIncludeFiles)

if(GRANTLEE_GREATER_0_2)
  add_definitions(-DGRANTLEE_GREATER_0_2)
endif()

if (NOT WINCE)

  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${KDE4_DATA_DIR}/cmake/modules)

  qt4_generate_dbus_interface(kmkernel.h org.kde.kmail.kmail.xml OPTIONS -a)
  qt4_generate_dbus_interface(kmcomposewin.h org.kde.kmail.mailcomposer.xml OPTIONS -a)

  add_custom_target(kmail_xml ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmail.kmail.xml
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmail.mailcomposer.xml
    COMMENT "Helper target for XML stuff. The Kontact plugin, KOrganizer and others depend on it."
  )

endif (NOT WINCE)

include_directories(
  ${CMAKE_SOURCE_DIR}/libkdepim
  ${CMAKE_SOURCE_DIR}/libksieve
  ${CMAKE_SOURCE_DIR}/libkleo
  ${CMAKE_SOURCE_DIR}/messagelist
  ${CMAKE_SOURCE_DIR}/messageviewer
  ${CMAKE_SOURCE_DIR}/templateparser
  ${CMAKE_BINARY_DIR}/templateparser
  ${CMAKE_SOURCE_DIR}/messagecomposer
  ${CMAKE_BINARY_DIR}/messagecomposer
  ${CMAKE_SOURCE_DIR}/messagecore
  ${CMAKE_SOURCE_DIR}/mailcommon
  ${CMAKE_BINARY_DIR}/mailcommon
  ${CMAKE_SOURCE_DIR}/pimcommon
  ${CMAKE_BINARY_DIR}/pimcommon
  ${CMAKE_SOURCE_DIR}/korganizer
  ${GPGME_INCLUDES}
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../ontologies
  ${NEPOMUK_CORE_INCLUDE_DIR}
  ${SOPRANO_INCLUDE_DIR}
 )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")


if (NOT WINCE AND KDEPIM_BUILD_DESKTOP)
  add_subdirectory(about)
  add_subdirectory(pics)
  add_subdirectory(icons)
  add_subdirectory(avscripts)
  add_subdirectory(tests)
  add_subdirectory(kconf_update)
endif(NOT WINCE AND KDEPIM_BUILD_DESKTOP)
add_subdirectory(kcm_kpimidentities)

########### kmailprivate ###############

if (NOT WINCE AND KDEPIM_BUILD_DESKTOP)

  set(kmailprivate_LIB_SRCS
     collectionpane.cpp
     collectiontemplatespage.cpp
     collectionmaintenancepage.cpp
     collectionviewpage.cpp
     collectionquotapage.cpp
     collectionquotapage_p.cpp
     colorlistbox.cpp
     kmmainwin.cpp
     configagentdelegate.cpp
     configuredialoglistview.cpp
     configuredialog.cpp
     configuredialog_p.cpp
     simplestringlisteditor.cpp
     identitylistview.cpp
     identitydialog.cpp
     globalsettings.cpp
     snippetwidget.cpp
     kmreaderwin.cpp
     kmsystemtray.cpp
     xfaceconfigurator.cpp
     undostack.cpp
     kmkernel.cpp
     searchwindow.cpp
     kmcommands.cpp
     kmreadermainwin.cpp
     kmstartup.cpp
     kmmainwidget.cpp
     aboutdata.cpp
     mailserviceimpl.cpp
     kmcomposereditor.cpp
     composer.cpp
     antispamwizard.cpp
     secondarywindow.cpp
     codecaction.cpp
     codecmanager.cpp
     foldershortcutdialog.cpp
     mailinglistpropertiesdialog.cpp
     util.cpp
     messageactions.cpp
     statusbarlabel.cpp
     kmcomposewin.cpp
     attachmentcontroller.cpp
     attachmentview.cpp
     archivefolderdialog.cpp
     searchdescriptionattribute.cpp
     tag.cpp
     tagactionmanager.cpp
     foldershortcutactionmanager.cpp
     addressvalidationjob.cpp
     identitypage.cpp
     newidentitydialog.cpp
     kmsearchmessagemodel.cpp
     kmsearchfilterproxymodel.cpp
     kmknotify.cpp
     tagselectdialog.cpp
     accountconfigorderdialog.cpp
     identityeditvcarddialog.cpp
     identityaddvcarddialog.cpp
     attachmentmissingwarning.cpp
  )

  soprano_add_ontology(kmailprivate_LIB_SRCS
    ${CMAKE_SOURCE_DIR}/messagecore/messagetag.trig
    "MessageTag"
    "Vocabulary"
    "trig"
  )

  # TODO PORT
  qt4_generate_dbus_interface(kmcomposewin.h org.kde.kmail.mailcomposer.xml OPTIONS -a)
  qt4_add_dbus_adaptor(kmailprivate_LIB_SRCS
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.mailcomposer.xml kmcomposewin.h KMComposeWin
  )

  qt4_add_dbus_adaptor(kmailprivate_LIB_SRCS
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml kmkernel.h KMKernel
  )
  qt4_add_dbus_adaptor(kmailprivate_LIB_SRCS
    ${CMAKE_SOURCE_DIR}/libkdepim/interfaces/org.kde.mailtransport.service.xml mailserviceimpl.h
    KMail::MailServiceImpl
  )

  qt4_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml
  )
  qt4_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ${CMAKE_SOURCE_DIR}/archivemailagent/org.freedesktop.Akonadi.ArchiveMailAgent.xml
  )


  kde4_add_ui_files(kmailprivate_LIB_SRCS
    ui/composercryptoconfiguration.ui
    ui/warningconfiguration.ui
    ui/smimeconfiguration.ui
    ui/miscpagemaintab.ui
    ui/securitypagegeneraltab.ui
    ui/identitypage.ui
    ui/accountspagereceivingtab.ui
  )

  # KCFG files. The main kmail.kcfg is configured by CMake and put
  # in the build directory.

  if(KDEPIM_ENTERPRISE_BUILD)
    set(WARN_TOOMANY_RECIPIENTS_DEFAULT true)
  else(KDEPIM_ENTERPRISE_BUILD)
    set(WARN_TOOMANY_RECIPIENTS_DEFAULT false)
  endif(KDEPIM_ENTERPRISE_BUILD)

  configure_file(kmail.kcfg.cmake ${CMAKE_CURRENT_BINARY_DIR}/kmail.kcfg)

  kde4_add_kcfg_files(kmailprivate_LIB_SRCS
    globalsettings_base.kcfgc
    custommimeheader.kcfgc
  )


  kde4_add_library(kmailprivate ${LIBRARY_TYPE} ${kmailprivate_LIB_SRCS})

  target_link_libraries(kmailprivate
    ${KDEPIMLIBS_KMBOX_LIBS}
    ${KDEPIMLIBS_KMIME_LIBS}
    ${KDEPIMLIBS_AKONADI_LIBS}
    ${KDEPIMLIBS_AKONADI_KMIME_LIBS}
    messagecore
    messagecomposer
    messagelist
    pimcommon
    mailcommon
    templateparser
    ${KDEPIMLIBS_KPIMIDENTITIES_LIBS}
    ${KDEPIMLIBS_KONTACTINTERFACE_LIBS}
    ${KDE4_PHONON_LIBRARY}
    ${KDE4_KNOTIFYCONFIG_LIBRARY}
    ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
    ${KDEPIMLIBS_KIMAP_LIBS}
    ${KDE4_KPARTS_LIBRARY}
    ${KDEPIMLIBS_KRESOURCES_LIBS}
    ${KDE4_KCMUTILS_LIBS}
    kleo
    ${QGPGME_LIBRARIES}
    ksieve
    ksieveui
    kpgp
    kdepim
    ${KDE4_KIO_LIBS}
    ${KDE4_SOLID_LIBS}
    ${KDEPIMLIBS_KABC_LIBS}
    ${KDEPIMLIBS_KPIMUTILS_LIBS}
    ${KDEPIMLIBS_KPIMTEXTEDIT_LIBS}
    messageviewer
    akonadi_next
    ${SOPRANO_LIBRARIES}
    niefast_apps
    ${QT_QTWEBKIT_LIBRARY}
    ${QT_QTSCRIPT_LIBRARY}
    ${NEPOMUK_LIBRARIES}
    ${NEPOMUK_QUERY_LIBRARIES}
    ${KDE4_KPRINTUTILS_LIBS}
    ${NEPOMUK_CORE_LIBRARY}
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
    mailcommon
    ${KDE4_KPARTS_LIBS}
    ${KDE4_KCMUTILS_LIBS}
  )

  ########### KMail executable ###############

  set(kmail_SRCS main.cpp)
  kde4_add_app_icon(kmail_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/hi*-apps-kmail.png")
  kde4_add_executable(kmail ${kmail_SRCS})
  target_link_libraries(kmail
    ${KDE4_KDEUI_LIBS}
    ${KDE4_KDECORE_LIBS}
    ${KDEPIMLIBS_KONTACTINTERFACE_LIBS}
    kmailprivate
    kdepim
    ${QT_QTSCRIPT_LIBRARY}
  )


  ########### install files ###############

  install(TARGETS kmailprivate ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

  install(TARGETS kmail ${INSTALL_TARGETS_DEFAULT_ARGS})

  install(PROGRAMS KMail2.desktop kmail_view.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kmail.kcfg
    DESTINATION ${KCFG_INSTALL_DIR}
  )
  install(FILES kmail.antispamrc kmail.antivirusrc  DESTINATION ${CONFIG_INSTALL_DIR})
  install(FILES tips  DESTINATION ${DATA_INSTALL_DIR}/kmail2)
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
    kmail2.notifyrc
    kmail_part.rc
    DESTINATION ${DATA_INSTALL_DIR}/kmail2
  )
  install(FILES dbusmail.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
  install(TARGETS
    kmailpart
    kcm_kmail
    DESTINATION ${PLUGIN_INSTALL_DIR}
  )
  install(FILES
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmailpart.xml
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml
    DESTINATION ${DBUS_INTERFACES_INSTALL_DIR}
  )

  install( FILES kmail_addattachmentservicemenu.desktop DESTINATION
    ${SERVICES_INSTALL_DIR}/ServiceMenus)

endif (NOT WINCE AND KDEPIM_BUILD_DESKTOP)
  
