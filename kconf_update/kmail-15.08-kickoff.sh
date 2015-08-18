#!/bin/sh

sed -i "s/\/KMail2.desktop/\/org.kde.kmail.desktop/" `kf5-config --path config --locate kickoffrc`
