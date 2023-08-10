include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../.. )

ecm_qt_declare_logging_category(autotest_categories_SRCS HEADER archivemailagent_debug.h IDENTIFIER ARCHIVEMAILAGENT_LOG CATEGORY_NAME org.kde.pim.archivemailagent)


# Convenience macro to add unit tests.
macro( archivemail_agent _source)
    set( _test ${_source} ../hourcombobox.cpp ../archivemailrangewidget.cpp ../archivemailwidget.cpp ../addarchivemaildialog.cpp ../widgets/formatcombobox.cpp ../widgets/unitcombobox.cpp ${autotest_categories_SRCS})
    ki18n_wrap_ui(_test ../ui/archivemailwidget.ui )
    get_filename_component( _name ${_source} NAME_WE )
    add_executable( ${_name} ${_test} ${_name}.h)
    add_test(NAME ${_name} COMMAND ${_name} )
    ecm_mark_as_test(archivemailagent-${_name})
    target_link_libraries( ${_name} archivemailagent Qt::Test Qt::Core KPim6::MailCommon KPim6::AkonadiCore KF6::XmlGui KF6::KIOWidgets KF6::I18n)
endmacro()

archivemail_agent(archivemailinfotest.cpp )
archivemail_agent(archivemailwidgettest.cpp)
archivemail_agent(formatcomboboxtest.cpp)
archivemail_agent(unitcomboboxtest.cpp)
archivemail_agent(archivemailrangewidgettest.cpp)
