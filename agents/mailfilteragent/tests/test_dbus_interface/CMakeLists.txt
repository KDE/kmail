# SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

#Main application

set(dbusservermanager_xml org.freedesktop.test.dbus.xml)
#set_source_files_properties(${dbusservermanager_xml} PROPERTIES INCLUDE "dbusoperators.h")
qt_add_dbus_adaptor(serverdbuswidget_SRCS ${dbusservermanager_xml} serverdbuswidget.h ServerDbusWidget)

add_executable(serverdbus_gui serverdbus_gui.cpp
    serverdbuswidget.h
    serverdbuswidget.cpp
    ${serverdbuswidget_SRCS}
)

target_link_libraries(serverdbus_gui
    Qt::Gui
    Qt::Widgets
    Qt::DBus
    )


#client
set(dbusclientmanager_xml org.freedesktop.test.dbus.xml)
qt_add_dbus_interfaces(clientdbus_gui_SRCS ${dbusclientmanager_xml})

add_executable(clientdbus_gui clientdbus_gui.cpp
    clientdbuswidget.h
    clientdbuswidget.cpp
    ${clientdbus_gui_SRCS}
)

target_link_libraries(clientdbus_gui
    Qt::Gui
    Qt::Widgets
    Qt::DBus
    )
