project(ktnef)

if(${CMAKE_SOURCE_DIR} STREQUAL ${ktnef_SOURCE_DIR})
    cmake_minimum_required(VERSION 3.1)
    set(KF5_VERSION "5.25.0")
    find_package(ECM ${KF5_VERSION} REQUIRED NO_MODULE)
    set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
    include(ECMInstallIcons)

    include(ECMOptionalAddSubdirectory)
    include(ECMSetupVersion)
    include(ECMAddTests)
    include(ECMMarkNonGuiExecutable)
    include(GenerateExportHeader)
    include(ECMGenerateHeaders)
    include(CMakePackageConfigHelpers)
    include(FeatureSummary)
    include(CheckFunctionExists)
    include(ECMGeneratePriFile)

    include(KDEInstallDirs)
    include(KDECMakeSettings)
    include(KDECompilerSettings NO_POLICY_SCOPE)
    include(ECMAddAppIcon)
    include(ECMQtDeclareLoggingCategory)

    set(QT_REQUIRED_VERSION "5.6.0")
    find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Gui Widgets)

    set(KTNEF_LIB_VERSION "5.3.40")
    find_package(KF5Tnef ${KTNEF_LIB_VERSION} CONFIG REQUIRED)
    find_package(KF5DBusAddons ${KF5_VERSION} CONFIG REQUIRED)
    find_package(KF5CoreAddons ${KF5_VERSION} CONFIG REQUIRED)
    find_package(KF5Crash ${KF5_VERSION} REQUIRED)

    include_directories(${ktnef_SOURCE_DIR} ${ktnef_BINARY_DIR} ${ktnef_BINARY_DIR}/src)
endif()

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
add_definitions(-DQT_USE_QSTRINGBUILDER)
include(TestBigEndian)
test_big_endian(CMAKE_WORDS_BIGENDIAN)
configure_file(src/config-ktnef.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/src/config-ktnef.h)
add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )

add_subdirectory(src)

install( FILES ktnef.categories DESTINATION ${KDE_INSTALL_CONFDIR} )

if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
    add_subdirectory(doc)
    feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
endif()
