project(kmail)

include(CheckIncludeFiles)

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )


if (NOT WINCE)

  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${KDE4_DATA_DIR}/cmake/modules)

  qt4_generate_dbus_interface(kmkernel.h org.kde.kmail.kmail.xml OPTIONS -a)
  qt4_generate_dbus_interface(editor/kmcomposewin.h org.kde.kmail.mailcomposer.xml OPTIONS -a)

  add_custom_target(kmail_xml ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmail.kmail.xml
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmail.mailcomposer.xml
    COMMENT "Helper target for XML stuff. The Kontact plugin, KOrganizer and others depend on it."
  )

endif ()

include_directories(
  ${CMAKE_SOURCE_DIR}/libkdepim
  ${CMAKE_SOURCE_DIR}/libksieve
  ${CMAKE_SOURCE_DIR}/libksieve/ksieveui/
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
  ${CMAKE_SOURCE_DIR}/mailcommon/folder
  ${CMAKE_SOURCE_DIR}/mailcommon/tag
  ${CMAKE_SOURCE_DIR}/mailcommon/kernel
  ${CMAKE_SOURCE_DIR}/pimcommon
  ${CMAKE_BINARY_DIR}/pimcommon
  ${CMAKE_SOURCE_DIR}/pimcommon/util/
  ${CMAKE_SOURCE_DIR}/korganizer
  ${CMAKE_SOURCE_DIR}/messagecomposer/utils/
  ${GPGME_INCLUDES}
  ${Boost_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/../ontologies
  ${NEPOMUK_CORE_INCLUDE_DIR}
  ${SOPRANO_INCLUDE_DIR}
  ${AKONADI_INCLUDE_DIR}
 )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}")


if (NOT WINCE AND KDEPIM_BUILD_DESKTOP)
  add_subdirectory(about)
  add_subdirectory(pics)
  add_subdirectory(icons)
  add_subdirectory(avscripts)
  add_subdirectory(tests)
  add_subdirectory(kconf_update)
endif()
add_subdirectory(kcm_kpimidentities)

########### kmailprivate ###############

if (NOT WINCE AND KDEPIM_BUILD_DESKTOP)

  set(kmailprivate_LIB_SRCS
     widgets/collectionpane.cpp
     collectionpage/collectiontemplatespage.cpp
     collectionpage/collectionmaintenancepage.cpp
     collectionpage/collectionviewpage.cpp
     collectionpage/collectionquotapage.cpp
     collectionpage/collectionquotapage_p.cpp
     collectionpage/collectionmailinglistpage.cpp
     collectionpage/collectionshortcutpage.cpp
     configuredialog/colorlistbox.cpp
     kmmainwin.cpp
     configuredialog/configagentdelegate.cpp
     configuredialog/configuredialoglistview.cpp
     configuredialog/configuredialog.cpp
     configuredialog/configuredialog_p.cpp
     identity/identitylistview.cpp
     identity/identitydialog.cpp
     settings/globalsettings.cpp
     editor/snippetwidget.cpp
     kmreaderwin.cpp
     kmsystemtray.cpp
     identity/xfaceconfigurator.cpp
     undostack.cpp
     kmkernel.cpp
     searchdialog/searchwindow.cpp
     kmcommands.cpp
     kmreadermainwin.cpp
     kmstartup.cpp
     kmmainwidget.cpp
     aboutdata.cpp
     mailserviceimpl.cpp
     editor/kmcomposereditor.cpp
     editor/composer.cpp
     antispam-virus/antispamwizard.cpp
     secondarywindow.cpp
     editor/codecaction.cpp
     editor/codecmanager.cpp
     util.cpp
     messageactions.cpp
     widgets/statusbarlabel.cpp
     editor/kmcomposewin.cpp
     editor/attachmentcontroller.cpp
     editor/attachmentview.cpp
     dialog/archivefolderdialog.cpp
     searchdialog/searchdescriptionattribute.cpp
     tag/tagactionmanager.cpp
     foldershortcutactionmanager.cpp
     job/addressvalidationjob.cpp
     identity/identitypage.cpp
     identity/newidentitydialog.cpp
     searchdialog/kmsearchmessagemodel.cpp
     searchdialog/kmsearchfilterproxymodel.cpp
     dialog/kmknotify.cpp
     tag/tagselectdialog.cpp
     configuredialog/accountconfigorderdialog.cpp
     identity/identityeditvcarddialog.cpp
     identity/identityaddvcarddialog.cpp
     warningwidgets/attachmentmissingwarning.cpp
     job/createnewcontactjob.cpp
     warningwidgets/externaleditorwarning.cpp
     configuredialog/configureagentswidget.cpp
  )

  # TODO PORT
  qt4_generate_dbus_interface(editor/kmcomposewin.h org.kde.kmail.mailcomposer.xml OPTIONS -a)
  qt4_add_dbus_adaptor(kmailprivate_LIB_SRCS
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.mailcomposer.xml editor/kmcomposewin.h KMComposeWin
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
    ${CMAKE_SOURCE_DIR}/agents/archivemailagent/org.freedesktop.Akonadi.ArchiveMailAgent.xml
  )
  qt4_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ${CMAKE_SOURCE_DIR}/agents/sendlateragent/org.freedesktop.Akonadi.SendLaterAgent.xml
  )

  qt4_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ${CMAKE_SOURCE_DIR}/kmail/dbusinterface/org.freedesktop.Akonadi.NewMailNotifier.xml
  )


  set_source_files_properties(${CMAKE_SOURCE_DIR}/agents/folderarchiveagent/org.freedesktop.Akonadi.FolderArchiveAgent.xml PROPERTIES INCLUDE "../mailcommon/dbusoperators.h")

  qt4_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ${CMAKE_SOURCE_DIR}/agents/folderarchiveagent/org.freedesktop.Akonadi.FolderArchiveAgent.xml
  )



  kde4_add_ui_files(kmailprivate_LIB_SRCS
    ui/composercryptoconfiguration.ui
    ui/warningconfiguration.ui
    ui/smimeconfiguration.ui
    ui/miscpagemaintab.ui
    ui/securitypagegeneraltab.ui
    ui/securitypagemdntab.ui
    ui/identitypage.ui
    ui/accountspagereceivingtab.ui
    ui/searchwindow.ui
  )

  # KCFG files. The main kmail.kcfg is configured by CMake and put
  # in the build directory.

  if(KDEPIM_ENTERPRISE_BUILD)
    set(WARN_TOOMANY_RECIPIENTS_DEFAULT true)
  else()
    set(WARN_TOOMANY_RECIPIENTS_DEFAULT false)
  endif()

  configure_file(settings/kmail.kcfg.cmake ${CMAKE_CURRENT_BINARY_DIR}/kmail.kcfg)

  kde4_add_kcfg_files(kmailprivate_LIB_SRCS
    settings/globalsettings_base.kcfgc
    editor/custommimeheader.kcfgc
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
    ${NEPOMUK_QUERY_LIBRARIES}
    ${KDE4_KPRINTUTILS_LIBS}
    ${NEPOMUK_CORE_LIBRARY}
    sendlater
    folderarchive
    ${AKONADI_COMMON_LIBRARIES}
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

  install(PROGRAMS data/KMail2.desktop data/kmail_view.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kmail.kcfg
    DESTINATION ${KCFG_INSTALL_DIR}
  )
  install(FILES antispam-virus/kmail.antispamrc antispam-virus/kmail.antivirusrc  DESTINATION ${CONFIG_INSTALL_DIR})
  install(FILES tips  DESTINATION ${DATA_INSTALL_DIR}/kmail2)
  install(FILES
    data/kmail_config_misc.desktop
    data/kmail_config_appearance.desktop
    data/kmail_config_identity.desktop
    data/kmail_config_accounts.desktop
    data/kmail_config_composer.desktop
    data/kmail_config_security.desktop
    DESTINATION ${SERVICES_INSTALL_DIR}
  )
  install(FILES
    kmcomposerui.rc
    kmmainwin.rc
    kmreadermainwin.rc
    data/kmail2.notifyrc
    kmail_part.rc
    DESTINATION ${DATA_INSTALL_DIR}/kmail2
  )
  install(FILES data/dbusmail.desktop DESTINATION ${SERVICETYPES_INSTALL_DIR})
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

  install( FILES data/kmail_addattachmentservicemenu.desktop DESTINATION
    ${SERVICES_INSTALL_DIR}/ServiceMenus)

endif ()
  
