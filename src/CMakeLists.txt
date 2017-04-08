add_definitions(-DTRANSLATION_DOMAIN=\"kmail\")

qt5_generate_dbus_interface(kmkernel.h org.kde.kmail.kmail.xml OPTIONS -a)
qt5_generate_dbus_interface(editor/kmcomposerwin.h org.kde.kmail.mailcomposer.xml OPTIONS -a)

add_custom_target(kmail_xml ALL
    DEPENDS ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml
    DEPENDS ${kmail_BINARY_DIR}/src/org.kde.kmail.mailcomposer.xml
    COMMENT "Helper target for XML stuff. The Kontact plugin, KOrganizer and others depend on it."
    )

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GPGME_INCLUDES}
    ${AKONADI_INCLUDE_DIR}
    )




add_subdirectory(about)
add_subdirectory(pics)
add_subdirectory(icons)
add_subdirectory(kcm_kpimidentities)
add_subdirectory(kconf_update)
add_subdirectory(kontactplugin)

########### kmailprivate ###############


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
    configuredialog/configurepluginpage.cpp
    configuredialog/colorlistbox.cpp
    )

set(kmailprivate_configureplugins_LIB_SRCS
    configuredialog/configureplugins/configurepluginslistwidget.cpp
    )

set(kmailprivate_searchdialog_LIB_SRCS
    searchdialog/kmsearchmessagemodel.cpp
    searchdialog/kmsearchfilterproxymodel.cpp
    searchdialog/searchpatternwarning.cpp
    searchdialog/kmailsearchpatternedit.cpp
    searchdialog/searchwindow.cpp
    searchdialog/searchdescriptionattribute.cpp
    searchdialog/incompleteindexdialog.cpp
    )
set(kmailprivate_identity_LIB_SRCS
    identity/identitylistview.cpp
    identity/identitydialog.cpp
    identity/xfaceconfigurator.cpp
    identity/identitypage.cpp
    identity/newidentitydialog.cpp
    identity/identityeditvcarddialog.cpp
    identity/identityaddvcarddialog.cpp
    identity/identityinvalidfolder.cpp
    identity/identityfolderrequester.cpp
    )
set(kmailprivate_editor_LIB_SRCS
    editor/widgets/snippetwidget.cpp
    editor/kmcomposereditorng.cpp
    editor/composer.cpp
    editor/codec/codecaction.cpp
    editor/codec/codecmanager.cpp
    editor/kmcomposerwin.cpp
    editor/attachment/attachmentcontroller.cpp
    editor/attachment/attachmentview.cpp
    editor/widgets/cryptostateindicatorwidget.cpp
    editor/validatesendmailshortcut.cpp
    editor/kmcomposerglobalaction.cpp
    editor/kmcomposerupdatetemplatejob.cpp
    editor/kmcomposercreatenewcomposerjob.cpp
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
set(kmailprivate_job_LIB_SRCS
    job/addressvalidationjob.cpp
    job/createnewcontactjob.cpp
    job/addemailtoexistingcontactjob.cpp
    job/createtaskjob.cpp
    job/savedraftjob.cpp
    job/removeduplicatemailjob.cpp
    job/createfollowupreminderonexistingmessagejob.cpp
    job/removecollectionjob.cpp
    job/saveasfilejob.cpp
    job/markallmessagesasreadinfolderandsubfolderjob.cpp
    job/removeduplicatemessageinfolderandsubfolderjob.cpp
    job/handleclickedurljob.cpp
    job/composenewmessagejob.cpp
    job/opencomposerjob.cpp
    job/newmessagejob.cpp
    job/opencomposerhiddenjob.cpp
    job/fillcomposerjob.cpp
    job/createreplymessagejob.cpp
    job/createforwardmessagejob.cpp
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


set(kmailprivate_plugininterface_LIB_SRCS
    plugininterface/kmailplugininterface.cpp
    )

set(kmailprivate_editor_plugininterface_LIB_SRCS
    editor/plugininterface/kmailplugineditormanagerinterface.cpp
    editor/plugininterface/kmailplugineditorcheckbeforesendmanagerinterface.cpp
    )

set(kmailprivate_checkindexing_LIB_SRCS
    search/checkindexingmanager.cpp
    search/checkindexingjob.cpp
    )
    
set(kmailprivate_sieveimapinstanceinterface_LIB_SRCS
    sieveimapinterface/kmailsieveimapinstanceinterface.cpp
    )

set(kmail_common_SRCS)
ecm_qt_declare_logging_category(kmail_common_SRCS HEADER kmail_debug.h IDENTIFIER KMAIL_LOG CATEGORY_NAME org.kde.pim.kmail)
set(kmailprivate_LIB_SRCS
    ${kmail_common_SRCS}
    kmmainwin.cpp
    settings/kmailsettings.cpp
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
    secondarywindow.cpp
    util.cpp
    messageactions.cpp
    foldershortcutactionmanager.cpp
    kmlaunchexternalcomponent.cpp
    manageshowcollectionproperties.cpp
    kmmigrateapplication.cpp
    ${kmailprivate_configureplugins_LIB_SRCS}
    ${kmailprivate_attributes_LIB_SRCS}
    ${kmailprivate_tag_LIB_SRCS}
    ${kmailprivate_job_LIB_SRCS}
    ${kmailprivate_widgets_LIB_SRCS}
    ${kmailprivate_dialogs_LIB_SRCS}
    ${kmailprivate_warningwidgets_LIB_SRCS}
    ${kmailprivate_folderarchive_LIB_SRCS}
    ${kmailprivate_collectionpage_LIB_SRCS}
    ${kmailprivate_configuredialog_LIB_SRCS}
    ${kmailprivate_searchdialog_LIB_SRCS}
    ${kmailprivate_identity_LIB_SRCS}
    ${kmailprivate_editor_LIB_SRCS}
    ${kmailprivate_editor_potentialphishingemail_SRCS}
    ${kmailprivate_plugininterface_LIB_SRCS}
    ${kmailprivate_editor_plugininterface_LIB_SRCS}
    ${kmailprivate_checkindexing_LIB_SRCS}
    ${kmailprivate_sieveimapinstanceinterface_LIB_SRCS}
    )

qt5_generate_dbus_interface(editor/kmcomposerwin.h org.kde.kmail.mailcomposer.xml OPTIONS -a)
qt5_add_dbus_adaptor(kmailprivate_LIB_SRCS
    ${kmail_BINARY_DIR}/src/org.kde.kmail.mailcomposer.xml editor/kmcomposerwin.h KMComposerWin
    )

qt5_add_dbus_adaptor(kmailprivate_LIB_SRCS
    ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml kmkernel.h KMKernel
    )
qt5_add_dbus_adaptor(kmailprivate_LIB_SRCS
    ${MAILTRANSPORT_DBUS_SERVICE} mailserviceimpl.h
    KMail::MailServiceImpl
    )

qt5_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml
    )
qt5_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ../agents/archivemailagent/org.freedesktop.Akonadi.ArchiveMailAgent.xml
    )
qt5_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ../agents/sendlateragent/org.freedesktop.Akonadi.SendLaterAgent.xml
    )

qt5_add_dbus_interfaces(kmailprivate_LIB_SRCS
    dbusinterface/org.freedesktop.Akonadi.NewMailNotifier.xml
    )

qt5_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ../agents/followupreminderagent/org.freedesktop.Akonadi.FollowUpReminder.xml
    )

qt5_add_dbus_interfaces(kmailprivate_LIB_SRCS
    ../agents/mailfilteragent/org.freedesktop.Akonadi.MailFilterAgent.xml
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
    ui/incompleteindexdialog.ui
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
    settings/globalsettings_kmail.kcfgc
    editor/custommimeheader/custommimeheader.kcfgc
    )


add_library(kmailprivate ${kmailprivate_LIB_SRCS})
generate_export_header(kmailprivate BASE_NAME kmail)
target_link_libraries(kmailprivate
    PRIVATE
    KF5::TextWidgets
    KF5::I18n
    KF5::Gravatar
    KF5::Mime
    KF5::AkonadiCore
    KF5::AkonadiMime
    KF5::MessageCore
    KF5::MessageList
    KF5::MessageComposer
    KF5::PimCommon
    KF5::MailCommon
    KF5::TemplateParser
    KF5::IdentityManagement
    KF5::WindowSystem
    KF5::Notifications
    KF5::GuiAddons
    KF5::Crash
    KF5::Bookmarks
    KF5::KIOFileWidgets
    KF5::NotifyConfig
    KF5::MailTransportAkonadi
    KF5::KCMUtils
    KF5::Libkleo
    KF5::KSieveUi
    KF5::LibkdepimAkonadi
    KF5::KIOCore
    KF5::Contacts
    KF5::PimTextEdit
    KF5::MessageViewer
    KF5::SendLater
    KF5::FollowupReminder
    KF5::IconThemes
    KF5::XmlGui
    KF5::Completion
    KF5::Ldap
    KF5::AkonadiSearchDebug
    KF5::AkonadiSearchPIM
    KF5::WebEngineViewer
    KF5::SyntaxHighlighting
    )
target_include_directories(kmailprivate PUBLIC $<TARGET_PROPERTY:KF5::MessageList,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(kmailprivate PUBLIC $<TARGET_PROPERTY:KF5::MessageViewer,INTERFACE_INCLUDE_DIRECTORIES>)
target_include_directories(kmailprivate PUBLIC $<TARGET_PROPERTY:KF5::MailCommon,INTERFACE_INCLUDE_DIRECTORIES>)

set_target_properties(kmailprivate
    PROPERTIES VERSION ${KDEPIM_LIB_VERSION} SOVERSION ${KDEPIM_LIB_SOVERSION}
    )

########### kcm_kmail ###############
set(kcm_kmail_PART_SRCS kcm_kmail.cpp )
add_library(kcm_kmail MODULE ${kcm_kmail_PART_SRCS})
target_link_libraries(kcm_kmail  kmailprivate KF5::KCMUtils KF5::Completion KF5::I18n KF5::TextWidgets)

########### kmailpart ###############

set(kmailpart_PART_SRCS kmail_part.cpp ${kmail_common_SRCS})
qt5_generate_dbus_interface(kmail_part.h org.kde.kmail.kmailpart.xml OPTIONS -a)
qt5_add_dbus_adaptor(kmailpart_PART_SRCS
    ${kmail_BINARY_DIR}/src/org.kde.kmail.kmailpart.xml kmail_part.h KMailPart
    )
qt5_add_dbus_interfaces(kmailpart_PART_SRCS ${kmail_BINARY_DIR}/src/org.kde.kmail.kmailpart.xml)
add_library(kmailpart MODULE ${kmailpart_PART_SRCS})

target_link_libraries(kmailpart
    kmailprivate
    KF5::PimCommon
    KF5::LibkdepimAkonadi
    KF5::KCMUtils
    KF5::Parts
    KF5::IconThemes
    KF5::TemplateParser
    )

########### KMail executable ###############

set(kmail_SRCS main.cpp ${kmail_common_SRCS})
file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*-apps-kmail.png")
ecm_add_app_icon(kmail_SRCS ICONS ${ICONS_SRCS})
add_executable(kmail ${kmail_SRCS})
target_link_libraries(kmail
    KF5::KontactInterface
    kmailprivate
    KF5::PimCommon
    KF5::I18n
    KF5::TemplateParser
    KF5::Libkdepim
    KF5::Crash
    )

if(BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(tests)
    add_subdirectory(folderarchive/autotests/)
    add_subdirectory(editor/potentialphishingemail/autotests)
    add_subdirectory(sieveimapinterface/tests/)
endif()
########### install files ###############

install(TARGETS kmailprivate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)

install(TARGETS kmail ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS data/org.kde.kmail2.desktop data/kmail_view.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/kmail.kcfg
    DESTINATION ${KDE_INSTALL_KCFGDIR}
    )
install(FILES data/org.kde.kmail2.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES
    data/kmail_config_misc.desktop
    data/kmail_config_appearance.desktop
    data/kmail_config_identity.desktop
    data/kmail_config_accounts.desktop
    data/kmail_config_composer.desktop
    data/kmail_config_security.desktop
    data/kmail_config_plugins.desktop
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
    ${kmail_BINARY_DIR}/src/org.kde.kmail.kmailpart.xml
    ${kmail_BINARY_DIR}/src/org.kde.kmail.kmail.xml
    DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR}
    )

install( FILES data/kmail_addattachmentservicemenu.desktop DESTINATION
    ${KDE_INSTALL_KSERVICES5DIR}/ServiceMenus)
