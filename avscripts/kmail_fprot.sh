#!/bin/sh
#
#    This file is part of KMail.
#    Copyright (c) 2004 Fred Emmott <fred87@users.sf.net>
#
#    KMail is free software; you can redistribute it and/or modify it
#    under the terms of the GNU General Public License, version 2, as
#    published by the Free Software Foundation.
#
#    KMail is distributed in the hope that it will be useful, but
#    WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#    General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
#    In addition, as a special exception, the copyright holders give
#    permission to link the code of this program with any edition of
#    the Qt library by Trolltech AS, Norway (or with modified versions
#    of Qt that use the same license as Qt), and distribute linked
#    combinations including the two.  You must obey the GNU General
#    Public License in all respects for all of the code used other than
#    Qt.  If you modify this file, you may extend this exception to
#    your version of the file, but you are not obligated to do so.  If
#    you do not wish to do so, delete this exception statement from
#    your version.
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
