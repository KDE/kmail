# SPDX-FileCopyrightText: 2024 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

macro(add_kmail_activities_test _source)
    set(_test ${_source})
    get_filename_component(_name ${_source} NAME_WE )
    add_executable( ${_name} ${_test} ${ARGN} ${_name}.h)
    add_test(NAME ${_name} COMMAND ${_name} )
    ecm_mark_as_test(${_name})
    target_link_libraries( ${_name} Qt::Test Plasma::Activities kmailprivate)
    set_target_properties(${_name} PROPERTIES
        DISABLE_PRECOMPILE_HEADERS ON
    )

endmacro()
add_kmail_activities_test(activitiesmanagertest.cpp)
