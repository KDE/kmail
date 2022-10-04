#!/bin/sh
#
#    This file is part of KMail.
#    SPDX-FileCopyrightText: 2004 Hendrik Muhs <Hendrik.Muhs@web.de>
#
#    SPDX-License-Identifier: GPL-2.0-only
#
TEMPFILE=`mktemp` 
if [ $? != 0 ] ; then 
    TEMPFILE=`mktemp /tmp/kmail.XXXXXX` 
fi 
export TEMPFILE
cat > $TEMPFILE
if antivir --scan-in-archive --scan-in-mbox $TEMPFILE | grep -q ALERT; then
echo "X-Virus-Flag: yes"
else
echo "X-Virus-Flag: no"
fi
cat $TEMPFILE
rm $TEMPFILE
