# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause

set(filterlogdialog_gui_SRCS filterlogdialog_gui.cpp )
add_executable(filterlogdialog_gui ${filterlogdialog_gui_SRCS})
target_link_libraries(filterlogdialog_gui
    mailfilteragentprivate
    KPim6::MailCommon
    )
