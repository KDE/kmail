set(kmail-refresh-settings_SRCS
    main.cpp
    refreshsettingsassistant.cpp
    )

ecm_qt_declare_logging_category(kmail-refresh-settings_SRCS HEADER kmail-refresh-settings_debug.h IDENTIFIER KMAIL_REFRESH_SETTINGS_LOG CATEGORY_NAME org.kde.pim.kmail_refresh_settings)

add_executable(kmail-refresh-settings ${kmail-refresh-settings_SRCS})
target_link_libraries(kmail-refresh-settings KF5::I18n KF5::AkonadiWidgets KF5::DBusAddons KF5::Crash KF5::XmlGui)


install(TARGETS kmail-refresh-settings  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
#install(PROGRAMS data/org.kde.akonadiimportwizard.desktop DESTINATION ${KDE_INSTALL_APPDIR})
