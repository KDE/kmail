#!/bin/sh

kickoffrcname=`qtpaths --locate-file  GenericConfigLocation kickoffrc`
if [ -f "$kickoffrcname" ]; then
   sed -i "s/\/KMail2.desktop/\/org.kde.kmail.desktop/" $kickoffrcname
fi
