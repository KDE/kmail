
########### install files ###############

set(icons
    16-apps-kmail.png
    22-apps-kmail.png
    32-apps-kmail.png
    48-apps-kmail.png
    64-apps-kmail.png
    128-apps-kmail.png
    sc-apps-kmail.svgz
)

file(GLOB icons_svg "*.svg")
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
