add_executable(sievemanagerdialog sievemanagerdialog.cpp ../../../sieveimapinterface/kmailsieveimapinstanceinterface.cpp)

target_link_libraries(sievemanagerdialog
  KF5::KSieve
  KF5::KSieveUi
  KF5::AkonadiCore
)
