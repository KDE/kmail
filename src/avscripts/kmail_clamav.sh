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

# check for a running daemon
if [ "`ps -axo comm|grep clamd`" = "clamd" ]; then
    chmod a+r $TEMPFILE
    CLAMCOMANDO="clamdscan --stdout --no-summary "
else
    CLAMCOMANDO="clamscan --stdout --no-summary"
fi

# analyze the message
if $CLAMCOMANDO $TEMPFILE | grep -q FOUND; then
    echo "X-Virus-Flag: yes"
else
    echo "X-Virus-Flag: no"
fi

cat $TEMPFILE
rm $TEMPFILE
