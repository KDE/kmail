# SPDX-FileCopyrightText: Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause
# Convenience macro to add unit tests.
macro( mailmerge_agent _source )
    set( _test ${_source})
    get_filename_component( _name ${_source} NAME_WE )
    ecm_add_test(${_test} ${_name}.h
        TEST_NAME ${_name}
        NAME_PREFIX "mailmerge-"
        LINK_LIBRARIES mailmergeagent Qt::Test KPim${KF_MAJOR_VERSION}::AkonadiCore Qt::Widgets KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::XmlGui KF${KF_MAJOR_VERSION}::Service
        )
endmacro()
