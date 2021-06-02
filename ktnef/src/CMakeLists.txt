configure_file(ktnef-version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/ktnef-version.h @ONLY)

add_subdirectory(pics)
add_executable(ktnef)

target_sources(ktnef PRIVATE
    attachpropertydialog.cpp
    ktnefmain.cpp
    ktnefview.cpp
    main.cpp
    messagepropertydialog.cpp
    qwmf.cpp
    ktnef.qrc
    )

ecm_qt_declare_logging_category(ktnef HEADER ktnef_debug.h IDENTIFIER KTNEFAPPS_LOG CATEGORY_NAME org.kde.pim.ktnefapps
        DESCRIPTION "kmail-refresh-settings"
        EXPORT KMAIL
    )


file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/pics/hicolor/*-apps-ktnef.png")
ecm_add_app_icon(ktnef ICONS ${ICONS_SRCS})

ki18n_wrap_ui(ktnef ui/attachpropertywidgetbase.ui)

if (COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(ktnef PROPERTIES UNITY_BUILD ON)
endif()

target_link_libraries(ktnef
    Qt::Widgets
    KF5::Tnef
    KF5::DBusAddons
    KF5::Crash
    KF5::I18n
    KF5::XmlGui
    KF5::WidgetsAddons
    KF5::Service
    KF5::KIOWidgets
)


install(TARGETS ktnef ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(PROGRAMS org.kde.ktnef.desktop DESTINATION ${KDE_INSTALL_APPDIR})

