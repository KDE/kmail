set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
include_directories(
  BEFORE
  ${CMAKE_SOURCE_DIR}/kmail
  ${CMAKE_BINARY_DIR}/kmail
)

set( kmail_migratekmail4configtest_source kmmigratekmail4configtest.cpp ../kmmigratekmail4config.cpp ../migrateinfo.cpp)
add_executable(migratekmail4configtest ${kmail_migratekmail4configtest_source})
ecm_mark_as_test(kmail-migratekmail4configtest)
target_link_libraries( migratekmail4configtest Qt5::Test KF5::CoreAddons)



set( kmail_migrateinfotest_source migrateinfotest.cpp ../migrateinfo.cpp)
add_executable(migrateinfotest ${kmail_migrateinfotest_source})
ecm_mark_as_test(kmail-migrateinfotest)
target_link_libraries( migrateinfotest Qt5::Test )

