#!/bin/sh
#
#    This file is part of KMail.
#    SPDX-FileCopyrightText: 2004 Fred Emmott <fred87@users.sf.net>
#
#    SPDX-License-Identifier: GPL-2.0-only
#
TEMPFILE=`mktemp` 
if [ $? != 0 ] ; then 
    TEMPFILE=`mktemp /tmp/kmail.XXXXXX` 
fi 
export TEMPFILE
cat > $TEMPFILE
f-prot -archive 3 $TEMPFILE > /dev/null
RC=$?
if [ $RC -eq 0 ] ; then
    echo "X-Virus-Flag: no"
else
    case $RC in
        1 ) DESC="no - Unrecoverable error" ;;
        2 ) DESC="no - Selftest failed" ;;
        3 ) DESC="yes - Virus-infected object found" ;;
        4 ) DESC="no - Reserved" ;;
        5 ) DESC="no - Abnormal termination" ;;
        6 ) DESC="no - Virus was removed" ;;
        7 ) DESC="no - Error, out of memory" ;;
        8 ) DESC="yes - Something suspicious found" ;;
    esac
    echo "X-Virus-Flag: $DESC"
fi

cat $TEMPFILE
rm $TEMPFILE
