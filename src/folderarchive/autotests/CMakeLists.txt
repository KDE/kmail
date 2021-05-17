
# Convenience macro to add unit tests.
macro( folderarchive_kmail _source )
  set( _test ${_source} )
  get_filename_component( _name ${_source} NAME_WE )
  add_executable( ${_name} ${_test} )
  add_test(NAME ${_name} COMMAND ${_name} )
  ecm_mark_as_test(folderararchive-${_name})

  target_link_libraries( ${_name} Qt::Test Qt::Core  KF5::AkonadiCore KF5::ConfigCore kmailprivate)
endmacro()

folderarchive_kmail(folderarchiveaccountinfotest.cpp)
