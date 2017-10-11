#!/bin/sh

kickoffrcname=`qtpaths --locate-file  GenericConfigLocation kickoffrc`
if [ -f "$kickoffrcname" ]; then
   sed -i "s/\/org.kde.kmail.desktop/\/org.kde.kmail2.desktop/" $kickoffrcname
fi

mimeappsname=`qtpaths --locate-file GenericConfigLocation mimeapps.list`
if [ -f "$mimeappsname" ]; then
    sed -i "s/org.kde.kmail.desktop/org.kde.kmail2.desktop/" $mimeappsname
fi
