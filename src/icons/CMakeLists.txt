
########### install files ###############

set(icons
    16-apps-kmail.png
    22-apps-kmail.png
    32-apps-kmail.png
    48-apps-kmail.png
    64-apps-kmail.png
    128-apps-kmail.png
    hisc-apps-kmail.svgz
)

file(GLOB icons "*.svg")


ecm_install_icons(ICONS ${icons}
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
