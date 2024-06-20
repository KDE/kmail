# SPDX-FileCopyrightText: 2019-2024 Laurent Montel <montel.org>
# SPDX-License-Identifier: BSD-3-Clause
add_executable(kmail-refresh-settings)
target_sources(kmail-refresh-settings PRIVATE
    main.cpp
    refreshsettingsassistant.cpp
    refreshsettringsfinishpage.cpp
    refreshsettingscleanuppage.cpp
    refreshsettingsfirstpage.cpp
    refreshsettingsassistant.h
    refreshsettringsfinishpage.h
    refreshsettingscleanuppage.h
    refreshsettingsfirstpage.h
)

ecm_qt_declare_logging_category(kmail-refresh-settings HEADER kmail-refresh-settings_debug.h IDENTIFIER KMAIL_REFRESH_SETTINGS_LOG CATEGORY_NAME org.kde.pim.kmail_refresh_settings
        DESCRIPTION "kmail-refresh-settings"
        EXPORT KMAIL
)


if(COMPILE_WITH_UNITY_CMAKE_SUPPORT)
    set_target_properties(kmail-refresh-settings PROPERTIES UNITY_BUILD ON)
endif()
target_link_libraries(kmail-refresh-settings KF6::I18n KPim6::AkonadiWidgets KF6::DBusAddons KF6::Crash KF6::XmlGui)


install(TARGETS kmail-refresh-settings  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.kmail-refresh-settings.desktop DESTINATION ${KDE_INSTALL_APPDIR})

