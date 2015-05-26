project(kmail)

include(CheckIncludeFiles)

add_definitions(-DTRANSLATION_DOMAIN=\"kmail\")
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

qt5_generate_dbus_interface(kmkernel.h org.kde.kmail.kmail.xml OPTIONS -a)
qt5_generate_dbus_interface(editor/kmcomposewin.h org.kde.kmail.mailcomposer.xml OPTIONS -a)

add_custom_target(kmail_xml ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmail.kmail.xml
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.kmail.mailcomposer.xml
  COMMENT "Helper target for XML stuff. The Kontact plugin, KOrganizer and others depend on it."
)

include_directories(
  ${CMAKE_SOURCE_DIR}/libksieve/ksieveui/
  ${CMAKE_SOURCE_DIR}/mailcommon/folder
  ${CMAKE_SOURCE_DIR}/mailcommon/tag
  ${CMAKE_SOURCE_DIR}/mailcommon/kernel
  ${CMAKE_SOURCE_DIR}/pimcommon/util/
  ${CMAKE_SOURCE_DIR}/messagecomposer/utils/
  ${CMAKE_CURRENT_BINARY_DIR}
  ${GPGME_INCLUDES}
  ${Boost_INCLUDE_DIR}
  ${AKONADI_INCLUDE_DIR}
 )




if (KDEPIM_BUILD_DESKTOP)
  add_subdirectory(about)
  add_subdirectory(pics)
  add_subdirectory(icons)
  add_subdirectory(avscripts)
  if (BUILD_TESTING)
     add_subdirectory(autotests)
  endif()
  add_subdirectory(kconf_update)
endif()
add_subdirectory(kcm_kpimidentities)

########### kmailprivate ###############

if (KDEPIM_BUILD_DESKTOP)

  set(kmailprivate_attributes_LIB_SRCS
     attributes/taskattribute.cpp
  )

  set(kmailprivate_folderarchive_LIB_SRCS
     folderarchive/folderarchiveaccountinfo.cpp
     folderarchive/folderarchiveutil.cpp
     folderarchive/folderarchivecache.cpp
     folderarchive/folderarchiveagentcheckcollection.cpp
     folderarchive/folderarchivemanager.cpp
     folderarchive/folderarchiveagentjob.cpp
  )
  set(kmailprivate_collectionpage_LIB_SRCS
     collectionpage/collectiontemplatespage.cpp
     collectionpage/collectionmaintenancepage.cpp
     collectionpage/collectionviewpage.cpp
     collectionpage/collectionquotapage.cpp
     collectionpage/collectionquotawidget.cpp
     collectionpage/collectionmailinglistpage.cpp
     collectionpage/collectionshortcutpage.cpp
  )
  set(kmailprivate_configuredialog_LIB_SRCS
     configuredialog/configagentdelegate.cpp
     configuredialog/configuredialoglistview.cpp
     configuredialog/configuredialog.cpp
     configuredialog/configuredialog_p.cpp
     configuredialog/configuremiscpage.cpp
     configuredialog/configuresecuritypage.cpp
     configuredialog/configurecomposerpage.cpp
     configuredialog/configureappearancepage.cpp
     configuredialog/configureaccountpage.cpp
     configuredialog/colorlistbox.cpp
     configuredialog/configureagentswidget.cpp
     configuredialog/configurestorageservicewidget.cpp
     configuredialog/configureagents/configureagentlistmodel.cpp
     configuredialog/configureagents/configureagentlistdelegate.cpp
     configuredialog/configureagents/configureagentlistview.cpp
     configuredialog/configureagents/configureagentitem.cpp
  )

  set(kmailprivate_searchdialog_LIB_SRCS
     searchdialog/kmsearchmessagemodel.cpp
     searchdialog/kmsearchfilterproxymodel.cpp
     searchdialog/searchpatternwarning.cpp
     searchdialog/kmailsearchpatternedit.cpp
     searchdialog/searchwindow.cpp
     searchdialog/searchdescriptionattribute.cpp
  )
  set(kmailprivate_storageservice_LIB_SRCS
     storageservice/storageservicesettingsjob.cpp
  )
  set(kmailprivate_identity_LIB_SRCS
     identity/identitylistview.cpp
     identity/identitydialog.cpp
     identity/xfaceconfigurator.cpp
     identity/identitypage.cpp
     identity/newidentitydialog.cpp
     identity/identityeditvcarddialog.cpp
     identity/identityaddvcarddialog.cpp
  )
  set(kmailprivate_editor_LIB_SRCS
     editor/widgets/snippetwidget.cpp
     editor/kmcomposereditor.cpp
     editor/composer.cpp
     editor/codec/codecaction.cpp
     editor/codec/codecmanager.cpp
     editor/kmcomposewin.cpp
     editor/attachment/attachmentcontroller.cpp
     editor/attachment/attachmentview.cpp
     editor/widgets/cryptostateindicatorwidget.cpp
     editor/storageservice/kmstorageservice.cpp
     editor/validatesendmailshortcut.cpp
     editor/kmcomposerglobalaction.cpp
  )
  set(kmailprivate_warningwidgets_LIB_SRCS
     warningwidgets/externaleditorwarning.cpp
     warningwidgets/attachmentmissingwarning.cpp
  )
  set(kmailprivate_editor_potentialphishingemail_SRCS
     editor/potentialphishingemail/potentialphishingemailwarning.cpp
     editor/potentialphishingemail/potentialphishingemailjob.cpp
     editor/potentialphishingemail/potentialphishingdetaildialog.cpp
     editor/potentialphishingemail/potentialphishingdetailwidget.cpp
  )
  set(kmailprivate_dialogs_LIB_SRCS
     dialog/archivefolderdialog.cpp
     dialog/addemailtoexistingcontactdialog.cpp
     dialog/kmknotify.cpp
  )
  set(kmailprivate_mailmerge_LIB_SRCS
     mailmerge/widgets/mailmergewidget.cpp
     mailmerge/widgets/attachmentlistwidget.cpp
     mailmerge/widgets/addressbookwidget.cpp
     mailmerge/mailmergedialog.cpp
     mailmerge/widgets/csvwidget.cpp
     mailmerge/widgets/selectattachmentdialog.cpp
     mailmerge/job/mailmergejob.cpp
  )

  set(kmailprivate_job_LIB_SRCS
     job/addressvalidationjob.cpp
     job/createnewcontactjob.cpp
     job/addemailtoexistingcontactjob.cpp
     job/createtaskjob.cpp
     job/savedraftjob.cpp
     job/removeduplicatemailjob.cpp
     job/createfollowupreminderonexistingmessagejob.cpp
     job/manageserversidesubscriptionjob.cpp
     job/manageserversidesubscriptionjob.cpp
     job/removecollectionjob.cpp
     job/saveasfilejob.cpp
  )
 
  set(kmailprivate_widgets_LIB_SRCS
     widgets/collectionpane.cpp
     widgets/vacationscriptindicatorwidget.cpp
     widgets/displaymessageformatactionmenu.cpp
     widgets/statusbarlabeltoggledstate.cpp
     widgets/kactionmenutransport.cpp
     widgets/kactionmenuaccount.cpp
  )
  set(kmailprivate_tag_LIB_SRCS
     tag/tagactionmanager.cpp
     tag/tagselectdialog.cpp
  )
  set(kmailprivate_LIB_SRCS
     kmail_debug.cpp
     kmmainwin.cpp
     settings/globalsettings.cpp
     kmreaderwin.cpp
     kmsystemtray.cpp
     undostack.cpp
     kmkernel.cpp
     kmcommands.cpp
     kmreadermainwin.cpp
     kmstartup.cpp
     kmmainwidget.cpp
     aboutdata.cpp
     mailserviceimpl.cpp
     antispam-virus/antispamwizard.cpp
     secondarywindow.cpp
     util.cpp
     messageactions.cpp
     foldershortcutactionmanager.cpp
     kmlaunchexternalcomponent.cpp
     manageshowcollectionproperties.cpp
     migration/kmmigratekmail4config.cpp
     ${kmailprivate_attributes_LIB_SRCS}
     ${kmailprivate_tag_LIB_SRCS}
     ${kmailprivate_job_LIB_SRCS}
     ${kmailprivate_widgets_LIB_SRCS}
     ${kmailprivate_mailmerge_LIB_SRCS}
     ${kmailprivate_dialogs_LIB_SRCS}
     ${kmailprivate_warningwidgets_LIB_SRCS}
     ${kmailprivate_folderarchive_LIB_SRCS}
     ${kmailprivate_collectionpage_LIB_SRCS}
     ${kmailprivate_configuredialog_LIB_SRCS}
     ${kmailprivate_searchdialog_LIB_SRCS}
     ${kmailprivate_storageservice_LIB_SRCS}
     ${kmailprivate_identity_LIB_SRCS}
     ${kmailprivate_editor_LIB_SRCS}
     ${kmailprivate_editor_potentialphishingemail_SRCS}
  )

  # TODO PORT
  qt5_generate_dbus_interface(editor/kmcomposewin.h org.kde.kmail.mailcomposer.xml OPTIONS -a)
  qt5_add_dbus_adaptor(kmailprivate_LIB_SRCS
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.mailcomposer.xml editor/kmcomposewin.h KMComposeWin
  )

  qt5_add_dbus_adaptor(kmailprivate_LIB_SRCS
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml kmkernel.h KMKernel
  )
  qt5_add_dbus_adaptor(kmailprivate_LIB_SRCS
    ${CMAKE_SOURCE_DIR}/libkdepim/interfaces/org.kde.mailtransport.service.xml mailserviceimpl.h
    KMail::MailServiceImpl
  )

  qt5_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml
  )
  qt5_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ${CMAKE_SOURCE_DIR}/agents/archivemailagent/org.freedesktop.Akonadi.ArchiveMailAgent.xml
  )
  qt5_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ${CMAKE_SOURCE_DIR}/agents/sendlateragent/org.freedesktop.Akonadi.SendLaterAgent.xml
  )

  qt5_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ${CMAKE_SOURCE_DIR}/kmail/dbusinterface/org.freedesktop.Akonadi.NewMailNotifier.xml
  )

  qt5_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ${CMAKE_SOURCE_DIR}/agents/followupreminderagent/org.freedesktop.Akonadi.FollowUpReminder.xml
  )


  ki18n_wrap_ui(kmailprivate_LIB_SRCS
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

  kconfig_add_kcfg_files(kmailprivate_LIB_SRCS
    settings/globalsettings_base.kcfgc
    editor/custommimeheader/custommimeheader.kcfgc
  )


  add_library(kmailprivate ${kmailprivate_LIB_SRCS})
generate_export_header(kmailprivate BASE_NAME kmail)

  target_link_libraries(kmailprivate
    KF5::Mime
    KF5::AkonadiCore
    KF5::AkonadiPrivate
    KF5::AkonadiMime
    messagecore
    messagecomposer
    messagelist
    pimcommon
    mailcommon
    templateparser
    KF5::IdentityManagement
    KF5::KDELibs4Support
    KF5::NotifyConfig
    KF5::MailTransport
    KF5::KCMUtils
    kleo
    ksieveui
    kdepim
    KF5::KIOCore
    KF5::Contacts
    KF5::PimTextEdit
    messageviewer
    KF5::DBusAddons
    sendlater
    followupreminder
    grantleetheme
  )

  set_target_properties(kmailprivate
    PROPERTIES VERSION ${KDEPIM_LIB_VERSION} SOVERSION ${KDEPIM_LIB_SOVERSION}
  )

  ########### kcm_kmail ###############

  set(kcm_kmail_PART_SRCS kcm_kmail.cpp )
  add_library(kcm_kmail MODULE ${kcm_kmail_PART_SRCS})
  target_link_libraries(kcm_kmail  kmailprivate )

  ########### kmailpart ###############

  set(kmailpart_PART_SRCS kmail_part.cpp kmail_debug.cpp)
  qt5_generate_dbus_interface(kmail_part.h org.kde.kmail.kmailpart.xml OPTIONS -a)
  qt5_add_dbus_adaptor(kmailpart_PART_SRCS
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmailpart.xml kmail_part.h KMailPart
  )
  qt5_add_dbus_interfaces(kmailpart_PART_SRCS ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmailpart.xml)
  add_library(kmailpart MODULE ${kmailpart_PART_SRCS})

  target_link_libraries(kmailpart
    
    KF5::AkonadiCore
    kmailprivate
    mailcommon
    KF5::KCMUtils
  )

  ########### KMail executable ###############

  set(kmail_SRCS main.cpp kmail_debug.cpp)
  file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*-apps-kmail.png")
  ecm_add_app_icon(kmail_SRCS ICONS ${ICONS_SRCS})
  add_executable(kmail ${kmail_SRCS})
  target_link_libraries(kmail
    KF5::KontactInterface
    kmailprivate
    kdepim
  )


  add_subdirectory(mailmerge/tests/)
  if (BUILD_TESTING)
     add_subdirectory(mailmerge/autotests/)
     add_subdirectory(folderarchive/autotests/)
     add_subdirectory(editor/potentialphishingemail/autotests)
  endif()
  ########### install files ###############

  install(TARGETS kmailprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

  install(TARGETS kmail ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

  install(PROGRAMS data/org.kde.kmail.desktop data/kmail_view.desktop DESTINATION ${KDE_INSTALL_APPDIR})
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kmail.kcfg
    DESTINATION ${KDE_INSTALL_KCFGDIR}
  )
  install(FILES data/org.kde.kmail.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
  install(FILES antispam-virus/kmail.antispamrc antispam-virus/kmail.antivirusrc  DESTINATION ${KDE_INSTALL_CONFDIR})
  install(FILES tips  DESTINATION ${KDE_INSTALL_DATADIR}/kmail2)
  install(FILES
    data/kmail_config_misc.desktop
    data/kmail_config_appearance.desktop
    data/kmail_config_identity.desktop
    data/kmail_config_accounts.desktop
    data/kmail_config_composer.desktop
    data/kmail_config_security.desktop
    DESTINATION ${KDE_INSTALL_KSERVICES5DIR}
  )
  install(FILES
    kmcomposerui.rc
    kmmainwin.rc
    kmreadermainwin.rc
    kmail_part.rc
    DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/kmail2
  )
  install( FILES data/kmail2.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR} )
  install(FILES data/dbusmail.desktop DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})
  install(TARGETS
    kmailpart
    kcm_kmail
    DESTINATION ${KDE_INSTALL_PLUGINDIR}
  )
  install(FILES
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmailpart.xml
    ${CMAKE_BINARY_DIR}/kmail/org.kde.kmail.kmail.xml
    DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR}
  )

  install( FILES data/kmail_addattachmentservicemenu.desktop DESTINATION
    ${KDE_INSTALL_KSERVICES5DIR}/ServiceMenus)

endif ()
 
add_subdirectory(kontactplugin) 
