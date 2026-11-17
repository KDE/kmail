# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(sievemanagerdialog sievemanagerdialog.cpp)

target_link_libraries(
    sievemanagerdialog
    KPim6::KSieveUi
    KPim6::AkonadiCore
    kmailprivate
    qt6keychain
)
