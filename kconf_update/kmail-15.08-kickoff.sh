#!/bin/sh

kickoffrcname=`kf5-config --path config --locate kickoffrc`
if [ -f "$kickoffrcname" ]; then
   sed -i "s/\/KMail2.desktop/\/org.kde.kmail.desktop/" $kickoffrcname
fi
