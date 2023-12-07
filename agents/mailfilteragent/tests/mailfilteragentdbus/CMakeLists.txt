# SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause


set(dbusobjectmanager_xml ../../org.freedesktop.Akonadi.MailFilterAgent.xml)
set_source_files_properties(${dbusobjectmanager_xml} PROPERTIES INCLUDE "../../dbusoperators.h")
qt_add_dbus_interfaces(mailagentdbus_gui_SRCS ${dbusobjectmanager_xml})


add_executable(mailagentdbus_gui mailagentdbus_gui.cpp
    mailagentdbuswidget.h
    mailagentdbuswidget.cpp
    ${mailagentdbus_gui_SRCS}
)

target_link_libraries(mailagentdbus_gui
    mailfilteragentprivate
    KPim6::MailCommon
    )
