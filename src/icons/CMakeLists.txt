
########### install files ###############

set(icons
    48-apps-kmail.svg
)

file(GLOB icons_svg "*-emblems-gpg-key-trust-level-*.svg")
file(GLOB dark_svg "dark/*.svg")

set(all_icons ${icons} ${icons_svg})

ecm_install_icons(ICONS ${all_icons}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
ecm_install_icons(ICONS ${dark_svg}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME breeze-dark
)
