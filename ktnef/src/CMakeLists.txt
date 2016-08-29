configure_file(ktnef-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/ktnef-version.h @ONLY)

add_subdirectory(pics)

set(ktnef_SRCS
    attachpropertydialog.cpp
    ktnefmain.cpp
    ktnefview.cpp
    main.cpp
    messagepropertydialog.cpp
    qwmf.cpp
    )
qt5_add_resources(ktnef_SRCS ktnef.qrc)
ecm_qt_declare_logging_category(ktnef_SRCS HEADER ktnef_debug.h IDENTIFIER KTNEFAPPS_LOG CATEGORY_NAME log_ktnefapps)

file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/pics/hicolor/*-apps-ktnef.png")
ecm_add_app_icon(ktnef_SRCS ICONS ${ICONS_SRCS})

ki18n_wrap_ui(ktnef_SRCS ui/attachpropertywidgetbase.ui)

add_executable(ktnef ${ktnef_SRCS})
target_link_libraries(ktnef KF5::Tnef KF5::DBusAddons KF5::Crash)


install(TARGETS ktnef ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS org.kde.ktnef.desktop DESTINATION ${KDE_INSTALL_APPDIR})

