# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(sievemanagerdialog sievemanagerdialog.cpp )

target_link_libraries(sievemanagerdialog
  KPim6::KSieve
  KPim6::KSieveUi
  KPim6::AkonadiCore
  kmailprivate
)

target_link_libraries(sievemanagerdialog qt${QT_MAJOR_VERSION}keychain)
