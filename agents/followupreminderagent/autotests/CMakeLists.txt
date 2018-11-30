
# Convenience macro to add unit tests.
macro( followupreminder_agent _source )
    set( _test ${_source} ../followupreminderinfowidget.cpp ../jobs/followupremindershowmessagejob.cpp ../followupremindernoanswerdialog.cpp)
    ecm_qt_declare_logging_category(_test HEADER followupreminderagent_debug.h IDENTIFIER FOLLOWUPREMINDERAGENT_LOG CATEGORY_NAME org.kde.pim.followupreminderagent)
    get_filename_component( _name ${_source} NAME_WE )
    ecm_add_test(${_test}
        TEST_NAME ${_name}
        NAME_PREFIX "followupreminder-"
        LINK_LIBRARIES Qt5::Test KF5::AkonadiCore KF5::FollowupReminder Qt5::Widgets KF5::I18n KF5::XmlGui KF5::Service
        )
endmacro()

followupreminder_agent(followupreminderinfotest.cpp)
followupreminder_agent(followupremindernoanswerdialogtest.cpp)
followupreminder_agent(followupreminderconfigtest.cpp)
