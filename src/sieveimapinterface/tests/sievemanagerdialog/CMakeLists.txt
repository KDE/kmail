# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(sievemanagerdialog sievemanagerdialog.cpp )

target_link_libraries(sievemanagerdialog
  KF5::KSieve
  KF5::KSieveUi
  KF5::AkonadiCore
  kmailprivate
)

target_link_libraries(sievemanagerdialog qt${QT_MAJOR_VERSION}keychain)
