# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(PIM_VERSION "6.2.1")

project(kmail VERSION ${PIM_VERSION})

include(CheckIncludeFiles)


set(KF_MIN_VERSION "6.3.0")
set(QT_REQUIRED_VERSION "6.6.0")
set(CMAKE_CXX_STANDARD 20)
find_package(ECM ${KF_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMAddTests)

include(GenerateExportHeader)
include(ECMGenerateHeaders)

include(FeatureSummary)
include(KDEGitCommitHooks)
include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
include(CheckFunctionExists)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddAppIcon)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)
include(ECMFeatureSummary)

# Do NOT add quote
set(KDEPIM_DEV_VERSION )
set(RELEASE_SERVICE_VERSION "24.08.1")

# add an extra space
if(DEFINED KDEPIM_DEV_VERSION)
    set(KDEPIM_DEV_VERSION " ${KDEPIM_DEV_VERSION}")
endif()

set(KDEPIM_VERSION "${PIM_VERSION}${KDEPIM_DEV_VERSION} (${RELEASE_SERVICE_VERSION})")


set(AKONADI_MIMELIB_VERSION "6.2.0")
set(AKONADI_CONTACT_VERSION "6.2.0")
set(CALENDARUTILS_LIB_VERSION "6.2.0")
set(IDENTITYMANAGEMENT_LIB_VERSION "6.2.0")
set(KLDAP_LIB_VERSION "6.2.0")
set(KMAILTRANSPORT_LIB_VERSION "6.2.0")
set(KONTACTINTERFACE_LIB_VERSION "6.2.0")
set(KMIME_LIB_VERSION "6.2.0")
set(KPIMTEXTEDIT_LIB_VERSION "6.2.0")
set(AKONADI_VERSION "6.2.0")
set(KTNEF_LIB_VERSION "6.2.0")

set(KDEPIM_LIB_VERSION "${PIM_VERSION}")
set(KDEPIM_LIB_SOVERSION "6")

set(KTEXTADDONS_MIN_VERSION "1.5.4")
option(KDEPIM_ENTERPRISE_BUILD "Enable features specific to the enterprise branch, which are normally disabled. Also, it disables many components not needed for Kontact such as the Kolab client." FALSE)

find_package(Qt6 ${QT_REQUIRED_VERSION} CONFIG REQUIRED DBus Network Test Widgets WebEngineWidgets)
set(LIBGRAVATAR_VERSION "6.2.0")
set(MAILCOMMON_LIB_VERSION "6.2.0")
set(MESSAGELIB_LIB_VERSION "6.2.0")
set(LIBKLEO_LIB_VERSION "6.2.0")
set(PIMCOMMON_LIB_VERSION "6.2.0")
set(LIBKDEPIM_LIB_VERSION "6.2.0")
set(LIBKSIEVE_LIB_VERSION "6.2.0")
set(AKONADI_SEARCH_VERSION "6.2.0")
set(QT6KEYCHAIN_LIB_VERSION "0.14.2")

find_package(KPim6WebEngineViewer ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)

find_package(KPim6AkonadiSearch ${AKONADI_SEARCH_VERSION} CONFIG REQUIRED)
set_package_properties(KPim6AkonadiSearch PROPERTIES DESCRIPTION "The Akonadi Search libraries" URL "https://invent.kde.org/pim/akonadi-search" TYPE REQUIRED PURPOSE "Provides search capabilities in KMail and Akonadi")

set(GPGMEPP_LIB_VERSION "1.20.0")
find_package(Gpgmepp ${GPGMEPP_LIB_VERSION} CONFIG REQUIRED)

# Find KF6 package
find_package(KF6Bookmarks ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Config ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6ConfigWidgets ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Crash ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6DBusAddons ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6DocTools ${KF_MIN_VERSION})
set_package_properties(KF6DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)
find_package(KF6GuiAddons ${KF_MIN_VERSION} REQUIRED)
find_package(KF6I18n ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6ItemViews ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6JobWidgets ${KF_MIN_VERSION} REQUIRED)
find_package(KF6KIO ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6KCMUtils ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Notifications ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6NotifyConfig ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Parts ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Service ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Sonnet ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6TextWidgets ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6WidgetsAddons ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6WindowSystem ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6XmlGui ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6IconThemes ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6Contacts ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6CalendarCore ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6StatusNotifierItem ${KF_MIN_VERSION} CONFIG REQUIRED)

find_package(KPim6Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KPim6AkonadiContactWidgets ${AKONADI_CONTACT_VERSION} CONFIG REQUIRED)
find_package(KPim6AkonadiMime ${AKONADI_MIMELIB_VERSION} CONFIG REQUIRED)
find_package(KPim6CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6IdentityManagementCore ${IDENTITYMANAGEMENT_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6LdapWidgets ${KLDAP_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6MailTransport ${KMAILTRANSPORT_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6TextEdit ${KPIMTEXTEDIT_LIB_VERSION} CONFIG REQUIRED)

find_package(KPim6KontactInterface ${KONTACTINTERFACE_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)

find_package(KPim6Gravatar ${LIBGRAVATAR_VERSION} CONFIG REQUIRED)
find_package(KPim6Libkdepim ${LIBKDEPIM_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6Libkleo ${LIBKLEO_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6KSieveUi ${LIBKSIEVE_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6MailCommon ${MAILCOMMON_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6MessageCore ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6MessageComposer ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6MessageList ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6MessageViewer ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6PimCommonAkonadi ${PIMCOMMON_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6TemplateParser ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6Tnef ${KTNEF_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim6MailTransportDBusService CONFIG REQUIRED)

find_package(KF6TextEditTextToSpeech ${KTEXTADDONS_MIN_VERSION} CONFIG)
set_package_properties(KF6TextEditTextToSpeech PROPERTIES DESCRIPTION
    "Add support for text to speech"
    TYPE OPTIONAL
)
if(TARGET KF6::TextEditTextToSpeech)
    add_definitions(-DHAVE_TEXT_TO_SPEECH_SUPPORT)
endif()
find_package(KF6TextAutoCorrectionWidgets ${KTEXTADDONS_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6TextUtils ${KTEXTADDONS_MIN_VERSION} CONFIG REQUIRED)
find_package(KF6TextCustomEditor ${KTEXTADDONS_MIN_VERSION} CONFIG REQUIRED)
find_package(Qt6Keychain ${QT6KEYCHAIN_LIB_VERSION} CONFIG)
set_package_properties(Qt6Keychain PROPERTIES
    DESCRIPTION "Provides support for secure credentials storage"
    URL "https://github.com/frankosterfeld/qtkeychain"
    TYPE REQUIRED)

configure_file(config-enterprise.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-enterprise.h)

include_directories(${kmail_SOURCE_DIR} ${kmail_BINARY_DIR})
configure_file(kmail-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/kmail-version.h @ONLY)
if(BUILD_TESTING)
    add_definitions(-DBUILD_TESTING)
endif()
add_definitions(-DQT_NO_CONTEXTLESS_CONNECT)
ecm_set_disabled_deprecation_versions(QT 6.7.2
    KF 6.4.0
)
find_package(KF6UserFeedback ${KF_MIN_VERSION} CONFIG)
set_package_properties(KF6UserFeedback PROPERTIES DESCRIPTION "User Feedback lib" TYPE OPTIONAL PURPOSE "Allow to send Telemetry Information (optional).")

option(USE_UNITY_CMAKE_SUPPORT "Use UNITY cmake support (speedup compile time)" OFF)

set(COMPILE_WITH_UNITY_CMAKE_SUPPORT OFF)
if(USE_UNITY_CMAKE_SUPPORT)
    set(COMPILE_WITH_UNITY_CMAKE_SUPPORT ON)
endif()


option(USE_PRECOMPILED_HEADERS "Use precompiled headers" OFF) # Set to OFF when using clazy and such
set(COMPILE_WITH_CMAKE_PCH_SUPPORT OFF)
macro(pim_target_precompile_headers)
    if(USE_PRECOMPILED_HEADERS)
        set(COMPILE_WITH_CMAKE_PCH_SUPPORT ON)
        if(COMPILE_WITH_CMAKE_PCH_SUPPORT)
            target_precompile_headers(${ARGV})
        endif()
    endif()
endmacro()

option(OPTION_USE_PLASMA_ACTIVITIES "Activate plasma activities (experimental)" OFF)
if(OPTION_USE_PLASMA_ACTIVITIES)
    if(NOT WIN32 AND NOT APPLE)
       set(OPTION_USE_PLASMA_ACTIVITIES true)
    else()
       set(OPTION_USE_PLASMA_ACTIVITIES false)
    endif()
endif()
if(OPTION_USE_PLASMA_ACTIVITIES)
    if(NOT WIN32 AND NOT APPLE)
       find_package(PlasmaActivities "6.1.0")
    endif()
endif()

if(TARGET Plasma::Activities)
    set(HAVE_ACTIVITY_SUPPORT TRUE)
endif()

configure_file(config-kmail.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-kmail.h)

add_subdirectory(src)
add_subdirectory(agents)
add_subdirectory(ktnef)
add_subdirectory(kmail-refresh-settings)
ecm_qt_install_logging_categories(
    EXPORT KMAIL
    FILE kmail.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
ki18n_install(po)
if(KF6DocTools_FOUND)
    kdoctools_install(po)
    add_subdirectory(doc)
endif()
ecm_feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

