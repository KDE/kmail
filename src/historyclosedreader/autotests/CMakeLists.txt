# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
# Convenience macro to add unit tests.
macro(historyclosedreader_kmail _source)
    set(_test ${_source})
    get_filename_component(_name ${_source} NAME_WE)
    add_executable(
        ${_name}
        ${_test}
        ${_name}.h
    )
    add_test(NAME ${_name} COMMAND ${_name})
    ecm_mark_as_test(historyclosedreader-${_name})

    target_link_libraries(
        ${_name}
        Qt::Test
        KPim6::AkonadiCore
        KF6::ConfigCore
        kmailprivate
    )
endmacro()

historyclosedreader_kmail(historyclosedreaderinfotest.cpp)
historyclosedreader_kmail(historyclosedreadermanagertest.cpp)
