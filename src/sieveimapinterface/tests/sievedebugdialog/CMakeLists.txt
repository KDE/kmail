# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(sievedebugdialog sievedebugdialog.cpp)

target_link_libraries(sievedebugdialog
  KPim6::KSieve
  KPim6::KSieveUi
  KPim6::AkonadiCore
  KF6::SyntaxHighlighting
  kmailprivate
)

target_link_libraries(sievedebugdialog qt6keychain)
