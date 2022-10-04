#!/bin/sh
#
#    This file is part of KMail.
#    SPDX-FileCopyrightText: 2004 Fred Emmott <fred87@users.sf.net>
#
#    SPDX-License-Identifier: GPL-2.0-only
TEMPFILE=`mktemp` 
if [ $? != 0 ] ; then 
    TEMPFILE=`mktemp /tmp/kmail.XXXXXX` 
fi 
export TEMPFILE
cat > $TEMPFILE
if sweep -ss -mime $TEMPFILE | grep -q found; then
echo "X-Virus-Flag: yes"
else
echo "X-Virus-Flag: no"
fi
cat $TEMPFILE
rm $TEMPFILE
