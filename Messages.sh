#! /bin/sh
$PREPARETIPS > tips.cpp
$XGETTEXT -ktranslate *.cpp *.h -o $podir/kmail.pot
rm -f tips.cpp
