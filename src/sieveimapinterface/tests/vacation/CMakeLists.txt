# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(vacationmultiservertest main.cpp)
target_link_libraries(vacationmultiservertest  KF5::KSieveUi KF5::KSieve KF${KF_MAJOR_VERSION}::I18n KF5::AkonadiCore kmailprivate)


target_link_libraries(vacationmultiservertest qt${QT_MAJOR_VERSION}keychain)
