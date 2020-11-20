add_executable(sievedebugdialog sievedebugdialog.cpp)

target_link_libraries(sievedebugdialog
  KF5::KSieve
  KF5::KSieveUi
  KF5::AkonadiCore
  KF5::SyntaxHighlighting
  kmailprivate
  qt5keychain
)
