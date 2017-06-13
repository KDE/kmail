add_executable(sievedebugdialog sievedebugdialog.cpp ../../../sieveimapinterface/kmailsieveimapinstanceinterface.cpp ../../../sieveimapinterface/kmsieveimappasswordprovider.cpp)

target_link_libraries(sievedebugdialog
  KF5::KSieve
  KF5::KSieveUi
  KF5::AkonadiCore
  KF5::SyntaxHighlighting
  KF5::Wallet
)
