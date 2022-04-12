# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(PIM_VERSION "5.20.40")

project(kmail VERSION ${PIM_VERSION})

include(CheckIncludeFiles)


set(KF5_MIN_VERSION "5.93.0")

find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
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

# Do NOT add quote
set(KDEPIM_DEV_VERSION alpha)
set(RELEASE_SERVICE_VERSION "22.07.40")

# add an extra space
if(DEFINED KDEPIM_DEV_VERSION)
    set(KDEPIM_DEV_VERSION " ${KDEPIM_DEV_VERSION}")
endif()

set(KDEPIM_VERSION "${PIM_VERSION}${KDEPIM_DEV_VERSION} (${RELEASE_SERVICE_VERSION})")


set(AKONADI_MIMELIB_VERSION "5.20.40")
set(AKONADI_CONTACT_VERSION "5.20.40")
set(CALENDARUTILS_LIB_VERSION "5.20.40")
set(IDENTITYMANAGEMENT_LIB_VERSION "5.20.40")
set(KLDAP_LIB_VERSION "5.20.40")
set(KMAILTRANSPORT_LIB_VERSION "5.20.40")
set(KONTACTINTERFACE_LIB_VERSION "5.20.40")
set(KMIME_LIB_VERSION "5.20.40")
set(KPIMTEXTEDIT_LIB_VERSION "5.20.40")
set(AKONADI_VERSION "5.20.40")
set(KTNEF_LIB_VERSION "5.20.40")
set(GRANTLEETHEME_LIB_VERSION "5.20.40")

set(KDEPIM_LIB_VERSION "${PIM_VERSION}")
set(KDEPIM_LIB_SOVERSION "5")

set(QT_REQUIRED_VERSION "5.15.2")
option(KDEPIM_ENTERPRISE_BUILD "Enable features specific to the enterprise branch, which are normally disabled. Also, it disables many components not needed for Kontact such as the Kolab client." FALSE)


find_package(Qt${QT_MAJOR_VERSION} ${QT_REQUIRED_VERSION} CONFIG REQUIRED DBus Network Test Widgets WebEngineWidgets)
set(LIBGRAVATAR_VERSION "5.20.40")
set(MAILCOMMON_LIB_VERSION "5.20.40")
set(MESSAGELIB_LIB_VERSION "5.20.40")
set(LIBKLEO_LIB_VERSION "5.20.40")
set(PIMCOMMON_LIB_VERSION "5.20.41")
set(LIBKDEPIM_LIB_VERSION "5.20.40")
set(LIBKSIEVE_LIB_VERSION "5.20.40")
set(AKONADI_SEARCH_VERSION "5.20.40")

find_package(KF5WebEngineViewer ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)

find_package(KF5AkonadiSearch ${AKONADI_SEARCH_VERSION} CONFIG REQUIRED)
set_package_properties(KF5AkonadiSearch PROPERTIES DESCRIPTION "The Akonadi Search libraries" URL "https://invent.kde.org/pim/akonadi-search" TYPE REQUIRED PURPOSE "Provides search capabilities in KMail and Akonadi")

set(GPGMEPP_LIB_VERSION "1.16.0")
find_package(Gpgmepp ${GPGMEPP_LIB_VERSION} CONFIG REQUIRED)

# Find KF5 package
find_package(KF5Bookmarks ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Config ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5ConfigWidgets ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Crash ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5DBusAddons ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5DocTools ${KF5_MIN_VERSION})
set_package_properties(KF5DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)
find_package(KF5GuiAddons ${KF5_MIN_VERSION} REQUIRED)
find_package(KF5I18n ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5ItemViews ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5JobWidgets ${KF5_MIN_VERSION} REQUIRED)
find_package(KF5KIO ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5KCMUtils ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Notifications ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5NotifyConfig ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Parts ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Service ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Sonnet ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5TextWidgets ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5WidgetsAddons ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5WindowSystem ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5XmlGui ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5IconThemes ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5Contacts ${KF5_MIN_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarCore ${KF5_MIN_VERSION} CONFIG REQUIRED)

find_package(KF5Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiContact ${AKONADI_CONTACT_VERSION} CONFIG REQUIRED)
find_package(KF5AkonadiMime ${AKONADI_MIMELIB_VERSION} CONFIG REQUIRED)
find_package(KF5CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5IdentityManagement ${IDENTITYMANAGEMENT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Ldap ${KLDAP_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5MailTransportAkonadi ${KMAILTRANSPORT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5PimTextEdit ${KPIMTEXTEDIT_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5GrantleeTheme ${GRANTLEETHEME_LIB_VERSION} CONFIG REQUIRED)

find_package(KF5KontactInterface ${KONTACTINTERFACE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)

find_package(KF5Gravatar ${LIBGRAVATAR_VERSION} CONFIG REQUIRED)
find_package(KF5Libkdepim ${LIBKDEPIM_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Libkleo ${LIBKLEO_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5LibKSieve ${LIBKSIEVE_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5MailCommon ${MAILCOMMON_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5MessageCore ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5MessageComposer ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5MessageList ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5MessageViewer ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5PimCommonAkonadi ${PIMCOMMON_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5TemplateParser ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KF5Tnef ${KTNEF_LIB_VERSION} CONFIG REQUIRED)
find_package(MailTransportDBusService CONFIG REQUIRED)

find_package(Qt${QT_MAJOR_VERSION}Keychain CONFIG)
set_package_properties(Qt${QT_MAJOR_VERSION}Keychain PROPERTIES
                                   DESCRIPTION "Provides support for secure credentials storage"
                                   URL "https://github.com/frankosterfeld/qtkeychain"
				   TYPE REQUIRED)


configure_file(config-enterprise.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-enterprise.h )

include_directories(${kmail_SOURCE_DIR} ${kmail_BINARY_DIR})
configure_file(kmail-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/kmail-version.h @ONLY)
if(BUILD_TESTING)
   add_definitions(-DBUILD_TESTING)
endif()
ecm_set_disabled_deprecation_versions(QT 5.15.2
    KF 5.93.0
    KCOREADDONS 5.88.0 #KPluginFactory::registerPlugin
)

find_package(KUserFeedback 1.2.0 CONFIG)
set_package_properties(KUserFeedback PROPERTIES DESCRIPTION "User Feedback lib" TYPE OPTIONAL PURPOSE "Allow to send Telemetry Information (optional).")

option(USE_UNITY_CMAKE_SUPPORT "Use UNITY cmake support (speedup compile time)" OFF)

set(COMPILE_WITH_UNITY_CMAKE_SUPPORT OFF)
if (USE_UNITY_CMAKE_SUPPORT)
    set(COMPILE_WITH_UNITY_CMAKE_SUPPORT ON)
endif()


option(USE_PRECOMPILED_HEADERS "Use precompiled headers" OFF) # Set to OFF when using clazy and such
set(COMPILE_WITH_CMAKE_PCH_SUPPORT OFF)
macro(pim_target_precompile_headers)
    if (USE_PRECOMPILED_HEADERS)
        set(COMPILE_WITH_CMAKE_PCH_SUPPORT ON)

        if (COMPILE_WITH_CMAKE_PCH_SUPPORT)
            target_precompile_headers(${ARGV})
        endif()
    endif()
endmacro()


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
if(KF5DocTools_FOUND)
    kdoctools_install(po)
    add_subdirectory(doc)
endif()
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

