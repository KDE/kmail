### Identity KCM, eventually to be moved out of KMail ###

if (KDEPIM_MOBILE_UI)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/.. )
add_definitions( -DKCM_KPIMIDENTITIES_STANDALONE )
set(kcm_identities_srcs
  kcm_kpimidentities.cpp
  ../addressvalidationjob.cpp
  ../identitypage.cpp
  ../identitydialog.cpp
  ../identitylistview.cpp
  ../newidentitydialog.cpp
  ../xfaceconfigurator.cpp
)
kde4_add_ui_files(kcm_identities_srcs ../ui/identitypage.ui)
kde4_add_plugin(kcm_kpimidentities ${kcm_identities_srcs})
target_link_libraries(kcm_kpimidentities
  ${KDEPIMLIBS_GPGMEPP_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KPIMIDENTITIES_LIBS}
  ${KDEPIMLIBS_MAILTRANSPORT_LIBS}
  ${KDE4_KCMUTILS_LIBS}
  ${KDE4_KIO_LIBS}
  akonadi_next
  messagecomposer
  messageviewer
  templateparser
  kleo
  kdepim
)

install(FILES kcm_kpimidentities.desktop DESTINATION ${SERVICES_INSTALL_DIR})
install(TARGETS kcm_kpimidentities DESTINATION ${PLUGIN_INSTALL_DIR})

endif (KDEPIM_MOBILE_UI)
