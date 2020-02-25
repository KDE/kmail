set(followupreminderagent_test_SRCS)
qt5_add_dbus_interface(followupreminderagent_test_SRCS ../org.freedesktop.Notifications.xml notifications_interface)
qt5_add_dbus_interface(followupreminderagent_test_SRCS ../org.freedesktop.DBus.Properties.xml dbusproperties)

set(autotest_categories_followupreminderagent_SRCS)
if (ECM_VERSION VERSION_LESS "5.68.0")
    ecm_qt_declare_logging_category(autotest_categories_followupreminderagent_SRCS HEADER followupreminderagent_debug.h IDENTIFIER FOLLOWUPREMINDERAGENT_LOG CATEGORY_NAME org.kde.pim.followupreminderagent)
else()
    ecm_qt_declare_logging_category(autotest_categories_followupreminderagent_SRCS HEADER followupreminderagent_debug.h IDENTIFIER FOLLOWUPREMINDERAGENT_LOG CATEGORY_NAME org.kde.pim.followupreminderagent
        DESCRIPTION "kmail (followupreminderagent)"
        OLD_CATEGORY_NAMES log_followupreminderagent
        EXPORT KMAIL
    )
endif()

# Convenience macro to add unit tests.
macro( followupreminder_agent _source )
    set( _test ${_source} ../followupreminderinfowidget.cpp ../jobs/followupremindershowmessagejob.cpp ../followupremindernoanswerdialog.cpp ${followupreminderagent_test_SRCS} ${autotest_categories_followupreminderagent_SRCS})
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
