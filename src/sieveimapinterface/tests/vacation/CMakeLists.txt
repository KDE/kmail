# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(vacationmultiservertest main.cpp)
target_link_libraries(vacationmultiservertest  KPim${KF_MAJOR_VERSION}::KSieveUi KPim${KF_MAJOR_VERSION}::KSieve KF${KF_MAJOR_VERSION}::I18n KPim${KF_MAJOR_VERSION}::AkonadiCore kmailprivate)


target_link_libraries(vacationmultiservertest qt${QT_MAJOR_VERSION}keychain)
