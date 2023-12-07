# SPDX-FileCopyrightText: 2023 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause


#set_source_files_properties(../../org.freedesktop.Akonadi.MailFilterAgent.xml PROPERTIES INCLUDE "dbusoperators.h")

qt_add_dbus_interfaces(mailagentdbus_gui_SRCS
  ../../org.freedesktop.Akonadi.MailFilterAgent.xml
)


add_executable(mailagentdbus_gui mailagentdbus_gui.cpp
    mailagentdbuswidget.h
    mailagentdbuswidget.cpp
    ${mailagentdbus_gui_SRCS}
)

target_link_libraries(mailagentdbus_gui
    mailfilteragentprivate
    KPim6::MailCommon
    )
