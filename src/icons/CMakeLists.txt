
########### install files ###############

file(GLOB kmail_icons "*-apps-kmail.*")
file(GLOB icons_svg "*-emblems-gpg-key-trust-level-*.svg")
file(GLOB dark_svg "dark/*.svg")

ecm_install_icons(ICONS ${kmail_icons} ${icons_svg}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
ecm_install_icons(ICONS ${dark_svg}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME breeze-dark
)
