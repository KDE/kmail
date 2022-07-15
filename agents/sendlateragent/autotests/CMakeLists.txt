# SPDX-FileCopyrightText: Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause
# Convenience macro to add unit tests.
macro(add_sendlater_agent_test _source )
    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(${_source} ${_name}.h
        TEST_NAME ${_name}
        NAME_PREFIX "sendlateragent-"
        LINK_LIBRARIES sendlateragent Qt${QT_MAJOR_VERSION}::Test
        )
endmacro()

add_sendlater_agent_test(sendlaterutiltest.cpp)
add_sendlater_agent_test(sendlaterconfiguredialogtest.cpp)
add_sendlater_agent_test(sendlaterconfigtest.cpp)
