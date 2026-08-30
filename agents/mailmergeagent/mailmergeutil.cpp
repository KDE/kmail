/*
   SPDX-FileCopyrightText: 2021-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mailmergeutil.h"
#include "mailmergeagentsettings.h"
using namespace Qt::Literals::StringLiterals;

void MailMergeUtil::forceReparseConfiguration()
{
    MailMergeAgentSettings::self()->save();
    MailMergeAgentSettings::self()->config()->reparseConfiguration();
}

bool MailMergeUtil::mailMergeAgentEnabled()
{
    return MailMergeAgentSettings::self()->enabled();
}

QString MailMergeUtil::mailMergePattern()
{
    return u"MailMergeItem %1"_s;
}
