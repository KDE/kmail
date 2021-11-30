# SPDX-FileCopyrightText: Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause
# Convenience macro to add unit tests.
macro( mailmerge_agent _source )
    set( _test ${_source})
    get_filename_component( _name ${_source} NAME_WE )
    ecm_add_test(${_test}
        TEST_NAME ${_name}
        NAME_PREFIX "mailmerge-"
        LINK_LIBRARIES mailmergeagent Qt::Test KF5::AkonadiCore Qt::Widgets KF5::I18n KF5::XmlGui KF5::Service
        )
endmacro()
