
# Convenience macro to add unit tests.
macro( folderarchive_kmail _source )
  set( _test ${_source} ../folderarchiveaccountinfo.cpp )
  get_filename_component( _name ${_source} NAME_WE )
  kde4_add_unit_test( ${_name} TESTNAME folderarchive-${_name} ${_test} )
  target_link_libraries( ${_name} ${QT_QTTEST_LIBRARY} ${QT_QTCORE_LIBRARY} ${KDE4_KDEUI_LIBS} ${KDEPIMLIBS_AKONADI_LIBS})
endmacro()

folderarchive_kmail(folderarchiveaccountinfotest.cpp)
