#! /bin/sh
$PREPARETIPS > tips.cpp
$XGETTEXT -ktranslate `find -name '*.cpp' -o -name '*.h'` -o $podir/kmail.pot
rm -f tips.cpp
