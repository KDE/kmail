#! /bin/sh
# SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>
# SPDX-License-Identifier: BSD-3-Clause
$EXTRACTRC ui/*.ui >> rc.cpp
$XGETTEXT `find . -name '*.h' -o -name '*.cpp' | grep -v '/tests/' | grep -v '/autotests/' ` -o $podir/akonadi_mailmerge_agent.pot
rm -f rc.cpp
