set(vacation_multi_server_SRCS
  main.cpp
  ../../../sieveimapinterface/kmailsieveimapinstanceinterface.cpp
)

add_executable(vacationmultiservertest ${vacation_multi_server_SRCS} )
target_link_libraries(vacationmultiservertest  KF5::KSieveUi KF5::KSieve KF5::I18n KF5::AkonadiCore)

