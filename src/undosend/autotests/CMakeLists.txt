macro(add_kmail_undosend_unittest _source)
    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(${_source}
        TEST_NAME ${_name}
        LINK_LIBRARIES kmailprivate Qt5::Test Qt5::Widgets QGpgme
    )
endmacro ()

add_kmail_undosend_unittest(undosendcomboboxtest.cpp)
add_kmail_undosend_unittest(undosendcreatejobtest.cpp)
