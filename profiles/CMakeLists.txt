set(profileelement default html high-contrast purist secure)
foreach ( profilecurrentelement ${profileelement})
  set(nameprofile "profile-${profilecurrentelement}-rc.desktop")
  install(FILES ${nameprofile} DESTINATION  ${DATA_INSTALL_DIR}/kmail/ RENAME profile-${profilecurrentelement}-rc )
endforeach(profilecurrentelement ${profileelement})

