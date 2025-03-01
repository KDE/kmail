# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(vacationmultiservertest main.cpp)
target_link_libraries(vacationmultiservertest  KPim6::KSieveUi KF6::I18n KPim6::AkonadiCore kmailprivate)


target_link_libraries(vacationmultiservertest qt6keychain)
