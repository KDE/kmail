# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(sievedebugdialog sievedebugdialog.cpp)

target_link_libraries(sievedebugdialog
  KF5::KSieve
  KF5::KSieveUi
  KF5::AkonadiCore
  KF5::SyntaxHighlighting
  kmailprivate
)

if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(sievedebugdialog qt6keychain)
else()
    target_link_libraries(sievedebugdialog qt5keychain)
endif()
