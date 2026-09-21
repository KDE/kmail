# SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause
# Convenience macro to add unit tests.
macro(add_snooze_agent_test _source)
    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(${_source} ${_name}.h
        TEST_NAME ${_name}
        NAME_PREFIX "snoozeagent-"
        LINK_LIBRARIES snoozeagent Qt::Test
    )
endmacro()

add_snooze_agent_test(snoozeinfotest.cpp)
add_snooze_agent_test(snoozeattributetest.cpp)
