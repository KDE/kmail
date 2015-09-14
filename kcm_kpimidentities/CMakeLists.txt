### Identity KCM, eventually to be moved out of KMail ###

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../identity )
add_definitions( -DKCM_KPIMIDENTITIES_STANDALONE )
set(kcm_identities_srcs
  kcm_kpimidentities.cpp
  ../kmail_debug.cpp
  ../job/addressvalidationjob.cpp
  ../identity/identitypage.cpp
  ../identity/identitydialog.cpp
  ../identity/identitylistview.cpp
  ../identity/newidentitydialog.cpp
  ../identity/identityeditvcarddialog.cpp
  ../identity/identityaddvcarddialog.cpp
  ../identity/xfaceconfigurator.cpp
)

ki18n_wrap_ui(kcm_identities_srcs ../ui/identitypage.ui)
add_library(kcm_kpimidentities MODULE ${kcm_identities_srcs})
target_link_libraries(kcm_kpimidentities
  KF5::AkonadiCore
  KF5::IdentityManagement
  KF5::MailTransport
  KF5::KCMUtils
  KF5::KIOCore
  KF5::KIOWidgets
  KF5::IconThemes  
  KF5::MailCommon
  messagecomposer
  KF5::MessageViewer
  KF5::TemplateParser
  KF5::PimCommon
  kleo
  KF5::Libkdepim
)

install(FILES kcm_kpimidentities.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(TARGETS kcm_kpimidentities DESTINATION ${KDE_INSTALL_PLUGINDIR})
