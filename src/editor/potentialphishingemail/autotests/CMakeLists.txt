# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

add_executable(
    kmail_potentialphishingemailjobtest
    potentialphishingemailjobtest.cpp
    potentialphishingemailjobtest.h
)
add_test(NAME kmail_potentialphishingemailjobtest COMMAND kmail_potentialphishingemailjobtest)
ecm_mark_as_test(kmail_potentialphishingemailjobtest)
target_link_libraries(
    kmail_potentialphishingemailjobtest
    Qt::Test
    KPim6::PimCommon
    kmailprivate
)

add_executable(
    kmail_potentialphishingdetaildialogtest
    potentialphishingdetaildialogtest.cpp
    potentialphishingdetaildialogtest.h
)
add_test(NAME kmail_potentialphishingdetaildialogtest COMMAND kmail_potentialphishingdetaildialogtest)
ecm_mark_as_test(kmail_potentialphishingdetaildialogtest)
target_link_libraries(
    kmail_potentialphishingdetaildialogtest
    Qt::Test
    Qt::Widgets
    KF6::ConfigCore
    KF6::I18n
    KF6::WidgetsAddons
    kmailprivate
)

add_executable(
    kmail_potentialphishingdetailwidgettest
    potentialphishingdetailwidgettest.cpp
    potentialphishingdetailwidgettest.h
)
add_test(NAME kmail_potentialphishingdetailwidgettest COMMAND kmail_potentialphishingdetailwidgettest)
ecm_mark_as_test(kmail_potentialphishingdetailwidgettest)
target_link_libraries(
    kmail_potentialphishingdetailwidgettest
    Qt::Test
    Qt::Widgets
    KF6::ConfigCore
    KF6::I18n
    KF6::WidgetsAddons
    KPim6::PimCommon
    kmailprivate
)

add_executable(
    kmail_potentialphishingemailwarningtest
    potentialphishingemailwarningtest.cpp
    potentialphishingemailwarningtest.h
)
add_test(NAME kmail_potentialphishingemailwarningtest COMMAND kmail_potentialphishingemailwarningtest)
ecm_mark_as_test(kmail_potentialphishingemailwarningtest)
target_link_libraries(
    kmail_potentialphishingemailwarningtest
    Qt::Test
    Qt::Widgets
    KF6::ConfigCore
    KF6::I18n
    KF6::WidgetsAddons
    kmailprivate
)
