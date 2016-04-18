#! /bin/sh
$EXTRACTRC ui/*.ui >> rc.cpp
$XGETTEXT `find . -name '*.h' -o -name '*.cpp' | grep -v '/tests/' | grep -v '/autotests/' ` -o $podir/akonadi_sendlater_agent.pot
rm -f rc.cpp
