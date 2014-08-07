include_directories(
  ${CMAKE_SOURCE_DIR}/kmail/mailmerge
)
# convenience macro to add qtest unit tests
macro(add_kmail_unittest _source)
  set(_test ${_source} ../mailmergewidget.cpp ../attachmentlistwidget.cpp)
  get_filename_component(_name ${_source} NAME_WE)
  add_executable( ${_name} ${_test} )
  add_test( ${_name} ${_name} )
  ecm_mark_as_test(kmail-${_name})
  target_link_libraries( ${_name}
    pimcommon
    Qt5::Test
    KF5::KIOCore
  )
endmacro ()

add_kmail_unittest( mailmergewidgettest.cpp )

