#! /bin/sh
$PREPARETIPS > tips.cpp
$EXTRACTRC `find . -name '*.ui' -or -name '*.rc' -or -name '*.kcfg' -or -name '*.kcfg.cmake'` >> rc.cpp || exit 11
$XGETTEXT -ktranslate `find -name '*.cpp' -o -name '*.h' | grep -v '/autotests/'` -o $podir/kmail.pot
rm -f tips.cpp
rm -f rc.cpp
