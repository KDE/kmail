# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

add_executable( kmail_incorrectidentityfolderwarningtest incorrectidentityfolderwarningtest.cpp incorrectidentityfolderwarningtest.h)
add_test(NAME kmail_incorrectidentityfolderwarningtest COMMAND kmail_incorrectidentityfolderwarningtest)
ecm_mark_as_test(kmail_incorrectidentityfolderwarningtest)
target_link_libraries( kmail_incorrectidentityfolderwarningtest Qt::Test KF5::PimCommon KF${KF_MAJOR_VERSION}::I18n kmailprivate)


add_executable( kmail_externaleditorwarningtest externaleditorwarningtest.cpp externaleditorwarningtest.h)
add_test(NAME kmail_externaleditorwarningtest COMMAND kmail_externaleditorwarningtest)
ecm_mark_as_test(kmail_externaleditorwarningtest)
target_link_libraries( kmail_externaleditorwarningtest Qt::Test KF5::PimCommon KF${KF_MAJOR_VERSION}::I18n kmailprivate)
