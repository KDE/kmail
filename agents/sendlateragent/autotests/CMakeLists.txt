# Convenience macro to add unit tests.
macro(add_sendlater_agent_test _source )
    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(${_source}
        TEST_NAME ${_name}
        NAME_PREFIX "sendlateragent-"
        LINK_LIBRARIES sendlateragent Qt::Test
        )
endmacro()

add_sendlater_agent_test(sendlaterutiltest.cpp)
add_sendlater_agent_test(sendlaterconfiguredialogtest.cpp)
add_sendlater_agent_test(sendlaterconfigtest.cpp)
