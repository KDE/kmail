# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(sievemanagerdialog sievemanagerdialog.cpp )

target_link_libraries(sievemanagerdialog
  KPim${KF_MAJOR_VERSION}::KSieve
  KPim${KF_MAJOR_VERSION}::KSieveUi
  KF5::AkonadiCore
  kmailprivate
)

target_link_libraries(sievemanagerdialog qt${QT_MAJOR_VERSION}keychain)
