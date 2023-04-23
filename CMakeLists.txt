# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(PIM_VERSION "5.23.40")

project(kmail VERSION ${PIM_VERSION})

include(CheckIncludeFiles)


set(KF_MIN_VERSION "5.105.0")

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

# Do NOT add quote
set(KDEPIM_DEV_VERSION alpha)
set(RELEASE_SERVICE_VERSION "23.07.40")

# add an extra space
if(DEFINED KDEPIM_DEV_VERSION)
    set(KDEPIM_DEV_VERSION " ${KDEPIM_DEV_VERSION}")
endif()

set(KDEPIM_VERSION "${PIM_VERSION}${KDEPIM_DEV_VERSION} (${RELEASE_SERVICE_VERSION})")


set(AKONADI_MIMELIB_VERSION "5.23.41")
set(AKONADI_CONTACT_VERSION "5.23.41")
set(CALENDARUTILS_LIB_VERSION "5.23.40")
set(IDENTITYMANAGEMENT_LIB_VERSION "5.23.40")
set(KLDAP_LIB_VERSION "5.23.40")
set(KMAILTRANSPORT_LIB_VERSION "5.23.40")
set(KONTACTINTERFACE_LIB_VERSION "5.23.40")
set(KMIME_LIB_VERSION "5.23.40")
set(KPIMTEXTEDIT_LIB_VERSION "5.23.40")
set(AKONADI_VERSION "5.23.40")
set(KTNEF_LIB_VERSION "5.23.40")
set(GRANTLEETHEME_LIB_VERSION "5.23.40")

set(KDEPIM_LIB_VERSION "${PIM_VERSION}")
set(KDEPIM_LIB_SOVERSION "5")

set(QT_REQUIRED_VERSION "5.15.2")
if (QT_MAJOR_VERSION STREQUAL "6")
    set(QT_REQUIRED_VERSION "6.4.0")
    set(KF_MIN_VERSION "5.240.0")
    set(KF_MAJOR_VERSION "6")
    set(KTEXTADDONS_MIN_VERSION "1.1.0")
else()
    set(KF_MAJOR_VERSION "5")
    set(KTEXTADDONS_MIN_VERSION "1.0.0")
endif()
option(KDEPIM_ENTERPRISE_BUILD "Enable features specific to the enterprise branch, which are normally disabled. Also, it disables many components not needed for Kontact such as the Kolab client." FALSE)

find_package(Qt${QT_MAJOR_VERSION} ${QT_REQUIRED_VERSION} CONFIG REQUIRED DBus Network Test Widgets WebEngineWidgets)
set(LIBGRAVATAR_VERSION "5.23.40")
set(MAILCOMMON_LIB_VERSION "5.23.40")
set(MESSAGELIB_LIB_VERSION "5.23.40")
set(LIBKLEO_LIB_VERSION "5.23.40")
set(PIMCOMMON_LIB_VERSION "5.23.40")
set(LIBKDEPIM_LIB_VERSION "5.23.40")
set(LIBKSIEVE_LIB_VERSION "5.23.40")
set(AKONADI_SEARCH_VERSION "5.23.40")
if (QT_MAJOR_VERSION STREQUAL "6")
    find_package(Qt6Core5Compat)
endif()

find_package(KPim${KF_MAJOR_VERSION}WebEngineViewer ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)

find_package(KPim${KF_MAJOR_VERSION}AkonadiSearch ${AKONADI_SEARCH_VERSION} CONFIG REQUIRED)
set_package_properties(KPim${KF_MAJOR_VERSION}AkonadiSearch PROPERTIES DESCRIPTION "The Akonadi Search libraries" URL "https://invent.kde.org/pim/akonadi-search" TYPE REQUIRED PURPOSE "Provides search capabilities in KMail and Akonadi")

set(GPGMEPP_LIB_VERSION "1.16.0")
find_package(Gpgmepp ${GPGMEPP_LIB_VERSION} CONFIG REQUIRED)

# Find KF5 package
find_package(KF${KF_MAJOR_VERSION}Bookmarks ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}Config ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}ConfigWidgets ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}Crash ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}DBusAddons ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}DocTools ${KF_MIN_VERSION})
set_package_properties(KF${KF_MAJOR_VERSION}DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)
find_package(KF${KF_MAJOR_VERSION}GuiAddons ${KF_MIN_VERSION} REQUIRED)
find_package(KF${KF_MAJOR_VERSION}I18n ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}ItemViews ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}JobWidgets ${KF_MIN_VERSION} REQUIRED)
find_package(KF${KF_MAJOR_VERSION}KIO ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}KCMUtils ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}Notifications ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}NotifyConfig ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}Parts ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}Service ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}Sonnet ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}TextWidgets ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}WidgetsAddons ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}WindowSystem ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}XmlGui ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}IconThemes ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}Contacts ${KF_MIN_VERSION} CONFIG REQUIRED)
find_package(KF${KF_MAJOR_VERSION}CalendarCore ${KF_MIN_VERSION} CONFIG REQUIRED)

find_package(KPim${KF_MAJOR_VERSION}Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}AkonadiContact ${AKONADI_CONTACT_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}ContactEditor ${AKONADI_CONTACT_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}AkonadiMime ${AKONADI_MIMELIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}CalendarUtils ${CALENDARUTILS_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}IdentityManagement ${IDENTITYMANAGEMENT_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}Ldap ${KLDAP_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}MailTransport ${KMAILTRANSPORT_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}TextEdit ${KPIMTEXTEDIT_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}GrantleeTheme ${GRANTLEETHEME_LIB_VERSION} CONFIG REQUIRED)

find_package(KPim${KF_MAJOR_VERSION}KontactInterface ${KONTACTINTERFACE_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)

find_package(KPim${KF_MAJOR_VERSION}Gravatar ${LIBGRAVATAR_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}Libkdepim ${LIBKDEPIM_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}Libkleo ${LIBKLEO_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}LibKSieve ${LIBKSIEVE_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}MailCommon ${MAILCOMMON_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}MessageCore ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}MessageComposer ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}MessageList ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}MessageViewer ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}PimCommonAkonadi ${PIMCOMMON_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}TemplateParser ${MESSAGELIB_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}Tnef ${KTNEF_LIB_VERSION} CONFIG REQUIRED)
find_package(KPim${KF_MAJOR_VERSION}MailTransportDBusService CONFIG REQUIRED)

find_package(KF${KF_MAJOR_VERSION}TextEditTextToSpeech ${KTEXTADDONS_MIN_VERSION} CONFIG)
set_package_properties(KF${KF_MAJOR_VERSION}TextEditTextToSpeech PROPERTIES DESCRIPTION
    "Add support for text to speech"
    TYPE OPTIONAL
)
if (TARGET KF${KF_MAJOR_VERSION}::TextEditTextToSpeech)
    add_definitions(-DHAVE_TEXT_TO_SPEECH_SUPPORT)
endif()
find_package(KF${KF_MAJOR_VERSION}TextAutoCorrectionWidgets ${KTEXTADDONS_MIN_VERSION} CONFIG)
set_package_properties(KF${KF_MAJOR_VERSION}TextAutoCorrectionWidgets PROPERTIES DESCRIPTION
    "Add support for text auto correction"
    TYPE OPTIONAL
)
if (TARGET KF${KF_MAJOR_VERSION}::TextAutoCorrectionWidgets)
    set(HAVE_TEXT_AUTOCORRECTION_WIDGETS TRUE)
else()
    find_package(KF${KF_MAJOR_VERSION}TextAutoCorrection ${KTEXTADDONS_MIN_VERSION} CONFIG REQUIRED)
endif()
configure_file(config-kmail.h.in ${CMAKE_CURRENT_BINARY_DIR}/config-kmail.h)

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
     KF 5.105.0
    )
set(KUSERFEEDBACK_SUFFIX)
if (QT_MAJOR_VERSION STREQUAL "6")
    set(KUSERFEEDBACK_SUFFIX "Qt6")
    find_package(KUserFeedback${KUSERFEEDBACK_SUFFIX} 1.2.1 CONFIG)
    add_definitions(-DUSE_KUSERFEEDBACK_QT6)
else()
    find_package(KUserFeedback${KUSERFEEDBACK_SUFFIX} 1.2.0 CONFIG)
endif()
set_package_properties(KUserFeedback${KUSERFEEDBACK_SUFFIX} PROPERTIES DESCRIPTION "User Feedback lib" TYPE OPTIONAL PURPOSE "Allow to send Telemetry Information (optional).")

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
if(KF${KF_MAJOR_VERSION}DocTools_FOUND)
    kdoctools_install(po)
    add_subdirectory(doc)
endif()
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

