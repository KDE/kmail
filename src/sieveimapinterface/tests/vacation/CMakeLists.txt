# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
add_executable(vacationmultiservertest main.cpp)
target_link_libraries(vacationmultiservertest  KF5::KSieveUi KF5::KSieve KF5::I18n KF5::AkonadiCore kmailprivate)


if (QT_MAJOR_VERSION STREQUAL "6")
    target_link_libraries(vacationmultiservertest qt6keychain)
else()
    target_link_libraries(vacationmultiservertest qt5keychain)
endif()
