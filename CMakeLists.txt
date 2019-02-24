cmake_minimum_required(VERSION 3.1)
set(KDEPIM_VERSION_NUMBER "5.10.3")

project(kmail VERSION ${KDEPIM_VERSION_NUMBER})

include(CheckIncludeFiles)


if (POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()
set(KF5_VERSION "5.51.0")

find_package(ECM ${KF5_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMAddTests)

include(GenerateExportHeader)
include(ECMGenerateHeaders)

include(FeatureSummary)
include(CheckFunctionExists)
include(ECMGeneratePriFile)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)

# Do NOT add quote
set(KDEPIM_DEV_VERSION)

# add an extra space
if(DEFINED KDEPIM_DEV_VERSION)
    set(KDEPIM_DEV_VERSION " ${KDEPIM_DEV_VERSION}")
endif()

set(KDEPIM_VERSION "${KDEPIM_VERSION_NUMBER}${KDEPIM_DEV_VERSION}")


set(AKONADI_MIMELIB_VERSION "5.10.3")
set(AKONADI_CONTACT_VERSION "5.10.3")
set(KCONTACTS_LIB_VERSION "5.10.3")
set(KCALENDARCORE_LIB_VERSION "5.10.3")
set(CALENDARUTILS_LIB_VERSION "5.10.3")
set(IDENTITYMANAGEMENT_LIB_VERSION "5.10.3")
set(KLDAP_LIB_VERSION "5.10.3")
set(KMAILTRANSPORT_LIB_VERSION "5.10.3")
set(KONTACTINTERFACE_LIB_VERSION "5.10.3")
set(KMIME_LIB_VERSION "5.10.3")
set(KPIMTEXTEDIT_LIB_VERSION "5.10.3")
set(AKONADI_VERSION "5.10.3")
set(KTNEF_LIB_VERSION "5.10.3")

set(KDEPIM_LIB_VERSION "${KDEPIM_VERSION_NUMBER}")
set(KDEPIM_LIB_SOVERSION "5")

set(QT_REQUIRED_VERSION "5.9.0")
option(KDEPIM_ENTERPRISE_BUILD "Enable features specific to the enterprise branch, which are normally disabled. Also, it disables many components not needed for Kontact such as the Kolab client." FALSE)

find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED DBus Network Test Widgets WebEngine WebEngineWidgets)
set(LIBGRAVATAR_VERSION_LIB "5.10.3")
set(MAILCOMMON_LIB_VERSION_LIB "5.10.3")
set(KDEPIM_APPS_LIB_VERSION_LIB "5.10.3")
set(MESSAGELIB_LIB_VERSION_LIB "5.10.3")
set(LIBKLEO_LIB_VERSION_LIB "5.10.3")
set(PIMCOMMON_LIB_VERSION_LIB "5.10.3")
set(LIBKDEPIM_LIB_VERSION_LIB "5.10.3")
set(LIBKSIEVE_LIB_VERSION_LIB "5.10.3")

find_package(KF5WebEngineViewer ${MESSAGELIB_LIB_VERSION_LIB} CONFIG REQUIRED)

find_package(KF5AkonadiSearch "5.10.3" CONFIG REQUIRED)
set_package_properties(KF5AkonadiSearch PROPERTIES DESCRIPTION "The Akonadi Search libraries" URL "http://www.kde.org" TYPE REQUIRED PURPOSE "Provides search capabilities in KMail and Akonadi")

set(GPGMEPP_LIB_VERSION "1.8.0")
find_package(Gpgmepp ${GPGMEPP_LIB_VERSION} CONFIG REQUIRED)

# Find KF5 package
find_package(KF5Bookmarks ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Codecs ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5ConfigWidgets ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Crash ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5DBusAddons ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5DocTools ${KF5_VERSION} REQUIRED)
find_package(KF5GuiAddons ${KF5_VERSION} REQUIRED)
find_package(KF5I18n ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5ItemViews ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5JobWidgets ${KF5_VERSION} REQUIRED)
find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5KCMUtils ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Notifications ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5NotifyConfig ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Parts ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Service ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Sonnet ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5TextWidgets ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5WidgetsAddons ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5WindowSystem ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5XmlGui ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5IconThemes ${KF5_VERSION} CONFIG REQUIRED)

# Find KdepimLibs Package
find_package(KF5Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiContact ${AKONADI_CONTACT_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${AKONADI_MIMELIB_VERSION} CONFIG REQUIRED)
find_package(KF5Contacts ${KCONTACTS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KCALENDARCORE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IdentityManagement ${IDENTITYMANAGEMENT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Ldap ${KLDAP_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5MailTransportAkonadi ${KMAILTRANSPORT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5PimTextEdit ${KPIMTEXTEDIT_LIB_VERSION} CONFIG REQUIRED)

find_package(KF5KontactInterface ${KONTACTINTERFACE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)

find_package(KF5FollowupReminder ${KDEPIM_APPS_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5Gravatar ${LIBGRAVATAR_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5KdepimDBusInterfaces ${KDEPIM_APPS_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5LibkdepimAkonadi ${LIBKDEPIM_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5Libkleo ${LIBKLEO_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5LibKSieve ${LIBKSIEVE_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5MailCommon ${MAILCOMMON_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5MessageCore ${MESSAGELIB_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5MessageComposer ${MESSAGELIB_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5MessageList ${MESSAGELIB_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5MessageViewer ${MESSAGELIB_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5PimCommonAkonadi ${PIMCOMMON_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5SendLater ${KDEPIM_APPS_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5TemplateParser ${MESSAGELIB_LIB_VERSION_LIB} CONFIG REQUIRED)
find_package(KF5Tnef ${KTNEF_LIB_VERSION} CONFIG REQUIRED)
find_package(MailTransportDBusService CONFIG REQUIRED)

configure_file(config-enterprise.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-enterprise.h )

include_directories(${kmail_SOURCE_DIR} ${kmail_BINARY_DIR})
configure_file(kmail-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kmail-version.h @ONLY)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)

add_subdirectory(src)
add_subdirectory(agents)
add_subdirectory(ktnef)

install( FILES kmail.renamecategories kmail.categories DESTINATION ${KDE_INSTALL_CONFDIR} )

add_subdirectory(doc)
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

