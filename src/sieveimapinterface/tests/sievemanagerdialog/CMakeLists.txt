# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(sievemanagerdialog sievemanagerdialog.cpp )

target_link_libraries(sievemanagerdialog
  KF5::KSieve
  KF5::KSieveUi
  KF5::AkonadiCore
  kmailprivate
)

if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(sievemanagerdialog qt6keychain)
else()
    target_link_libraries(sievemanagerdialog qt5keychain)
endif()
