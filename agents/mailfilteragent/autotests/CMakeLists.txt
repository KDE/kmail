# Convenience macro to add unit tests.
macro(add_mailfilter_agent_test _source _extrasource)
    set(_test ${_source} ${_extrasource})
    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(${_test}
        TEST_NAME ${_name}
        NAME_PREFIX "mailfilteragent-"
        LINK_LIBRARIES Qt5::Test Qt5::Widgets KF5::I18n KF5::ConfigCore
        )
endmacro()

add_mailfilter_agent_test(configurewidgettest.cpp "../config/configurewidget.cpp")
