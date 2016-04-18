# Convenience macro to add unit tests.
macro(add_sendlater_agent_test _source )
    set(_test ${_source} ../sendlaterconfiguredialog.cpp ../sendlaterconfigurewidget.cpp)
    ki18n_wrap_ui(_test ../ui/sendlaterconfigurewidget.ui)
    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(${_test}
        TEST_NAME ${_name}
        NAME_PREFIX "sendlateragent-"
        LINK_LIBRARIES Qt5::Test KF5::XmlGui KF5::AkonadiCore KF5::SendLater KF5::PimCommon KF5::I18n
        )
endmacro()

add_sendlater_agent_test(sendlaterinfotest.cpp)
add_sendlater_agent_test(sendlaterconfiguredialogtest.cpp)
add_sendlater_agent_test(sendlaterconfigtest.cpp)
add_sendlater_agent_test(sendlaterdialogtest.cpp)
