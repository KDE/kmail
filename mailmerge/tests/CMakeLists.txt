include_directories(
  ${CMAKE_SOURCE_DIR}/kmail/mailmerge
)
# convenience macro to add qtest unit tests
macro(add_kmail_unittest _source)
  set(_test ${_source} ../mailmergewidget.cpp ../attachmentlistwidget.cpp)
  get_filename_component(_name ${_source} NAME_WE)
  kde4_add_unit_test(${_name} TESTNAME kmail-${_name} ${_test})
  target_link_libraries( ${_name}
    pimcommon
    Qt5::Test
    
    
    KF5::KIOCore
  )
endmacro ()

add_kmail_unittest( mailmergewidgettest.cpp )


set(mailmergewidget_gui_SRCS mailmergewidgettest_gui.cpp ../mailmergewidget.cpp ../attachmentlistwidget.cpp)
add_executable(mailmergewidget_gui ${mailmergewidget_gui_SRCS})
target_link_libraries(mailmergewidget_gui
  
  
  
  KF5::KIOCore
  pimcommon
)

