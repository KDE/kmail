# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
macro(add_kmail_historyswitchfolder_unittest _source)
    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(${_source}
        TEST_NAME ${_name}
        LINK_LIBRARIES kmailprivate Qt::Test Qt::Widgets
    )
endmacro ()


add_kmail_historyswitchfolder_unittest(collectionswitchertreeviewtest.cpp collectionswitchertreeviewtest.h)
add_kmail_historyswitchfolder_unittest(collectionswitchertreeviewmanagertest.cpp collectionswitchertreeviewmanagertest.h)
