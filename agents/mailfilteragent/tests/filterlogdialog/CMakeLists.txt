# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause

add_executable(filterlogdialog_gui filterlogdialog_gui.cpp)
target_link_libraries(
    filterlogdialog_gui
    mailfilteragentprivate
    KPim6::MailCommon
)
