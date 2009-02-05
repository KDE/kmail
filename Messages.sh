#! /bin/sh
$PREPARETIPS > tips.cpp
$EXTRACTRC `find . -name '*.ui' -or -name '*.rc' -or -name '*.kcfg'` >> rc.cpp || exit 11
$XGETTEXT -ktranslate `find -name '*.cpp' -o -name '*.h'` -o $podir/kmail.pot
rm -f tips.cpp
rm -f rc.cpp
