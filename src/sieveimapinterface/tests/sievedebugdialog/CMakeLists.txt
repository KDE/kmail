# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(sievedebugdialog sievedebugdialog.cpp)

target_link_libraries(sievedebugdialog
  KPim${KF_MAJOR_VERSION}::KSieve
  KPim${KF_MAJOR_VERSION}::KSieveUi
  KPim${KF_MAJOR_VERSION}::AkonadiCore
  KF${KF_MAJOR_VERSION}::SyntaxHighlighting
  kmailprivate
)

target_link_libraries(sievedebugdialog qt${QT_MAJOR_VERSION}keychain)
