include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

# Convenience macro to add unit tests.
macro( archivemail_agent _source)
    set( _test ${_source} ../archivemailinfo.cpp ../archivemaildialog.cpp ../archivemailagentutil.cpp ../addarchivemaildialog.cpp  ../widgets/formatcombobox.cpp ../widgets/unitcombobox.cpp ../archivemailwidget.cpp)
    ki18n_wrap_ui(_test ../ui/archivemailwidget.ui )
    ecm_qt_declare_logging_category(_test HEADER archivemailagent_debug.h IDENTIFIER ARCHIVEMAILAGENT_LOG CATEGORY_NAME log_archivemailagent)
    get_filename_component( _name ${_source} NAME_WE )
    add_executable( ${_name} ${_test} )
    add_test( ${_name} ${_name} )
    ecm_mark_as_test(archivemailagent-${_name})
    target_link_libraries( ${_name} Qt5::Test Qt5::Core KF5::MailCommon KF5::AkonadiCore KF5::XmlGui KF5::KIOWidgets KF5::Libkdepim KF5::I18n)
endmacro()

archivemail_agent(archivemailinfotest.cpp )
archivemail_agent(archivemaildialogtest.cpp)
archivemail_agent(formatcomboboxtest.cpp)
archivemail_agent(unitcomboboxtest.cpp)
