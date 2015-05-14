
# Convenience macro to add unit tests.
macro( folderarchive_kmail _source )
  set( _test ${_source} ../folderarchiveaccountinfo.cpp )
  get_filename_component( _name ${_source} NAME_WE )
  add_executable( ${_name} ${_test} )
  add_test( ${_name} ${_name} )
  ecm_mark_as_test(folderararchive-${_name})

  target_link_libraries( ${_name} Qt5::Test Qt5::Core  KF5::AkonadiCore KF5::ConfigCore)
endmacro()

folderarchive_kmail(folderarchiveaccountinfotest.cpp)
