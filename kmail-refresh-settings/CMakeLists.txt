set(kmail-refresh-settings_srcs
    main.cpp
    refreshsettingsassistant.cpp
    )

add_executable(kmail-refresh-settings ${kmail-refresh-settings_srcs})
target_link_libraries(kmail-refresh-settings KF5::I18n KF5::AkonadiWidgets KF5::DBusAddons KF5::Crash)
