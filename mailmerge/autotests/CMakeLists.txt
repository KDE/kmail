include_directories(
  ${CMAKE_SOURCE_DIR}/kmail/mailmerge
)

# convenience macro to add qtest unit tests
macro(add_kmail_unittest _source _additionalsource)
  set(_test ${_source} ${_additionalsource})
  get_filename_component(_name ${_source} NAME_WE)
  add_executable( ${_name} ${_test} )
  add_test( ${_name} ${_name} )
  ecm_mark_as_test(kmail-${_name})
  target_link_libraries( ${_name}
    pimcommon
    Qt5::Test
    KF5::KIOCore
    KF5::KIOWidgets
  )
endmacro ()

add_kmail_unittest( mailmergewidgettest.cpp "../widgets/csvwidget.cpp;../widgets/addressbookwidget.cpp;../widgets/mailmergewidget.cpp;../widgets/attachmentlistwidget.cpp;../widgets/selectattachmentdialog.cpp")
add_kmail_unittest(addressbookwidgettest.cpp "../widgets/csvwidget.cpp")
add_kmail_unittest(csvwidgettest.cpp "../widgets/csvwidget.cpp")
add_kmail_unittest(mailmergedialogtest.cpp "../mailmergedialog.cpp;../widgets/attachmentlistwidget.cpp;../widgets/selectattachmentdialog.cpp;../widgets/mailmergewidget.cpp;../widgets/csvwidget.cpp;../widgets/addressbookwidget.cpp;../widgets/selectattachmentdialog.cpp" "pimcommon")
add_kmail_unittest(mailmergejobtest.cpp "../job/mailmergejob.cpp" "")
