# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
macro(add_kmail_undosend_unittest _source)
    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(${_source} ${_name}.h
        TEST_NAME ${_name}
        LINK_LIBRARIES kmailprivate Qt::Test Qt::Widgets
    )
endmacro()

add_kmail_undosend_unittest(undosendcomboboxtest.cpp)
add_kmail_undosend_unittest(undosendcreatejobtest.cpp)
