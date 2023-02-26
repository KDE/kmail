/*
   SPDX-FileCopyrightText: 2019-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "undosendmanager.h"
#include "kmail_debug.h"
#include "undosendcreatejob.h"
#include <KLocalizedString>

UndoSendManager::UndoSendManager(QObject *parent)
    : QObject(parent)
{
}

UndoSendManager::~UndoSendManager() = default;

UndoSendManager *UndoSendManager::self()
{
    static UndoSendManager s_self;
    return &s_self;
}

void UndoSendManager::addItem(const UndoSendManagerInfo &info)
{
    if (info.isValid()) {
        auto job = new UndoSendCreateJob(this);
        job->setAkonadiIndex(info.index);
        job->setMessageInfoText(info.generateMessageInfoText());
        job->setDelay(info.delay);
        if (!job->start()) {
            qCWarning(KMAIL_LOG) << " Impossible to create job";
        }
    }
}

QString UndoSendManager::UndoSendManagerInfo::generateMessageInfoText() const
{
    QString str = QStringLiteral("<qt>");
    if (!to.isEmpty()) {
        str += i18n("<b>To:</b> %1", to);
    }
    if (!subject.isEmpty()) {
        if (!str.isEmpty()) {
            str += QLatin1Char('\n');
        }
        str += i18n("<b>Subject:</b> %1", subject);
    }
    str += QStringLiteral("</qt>");
    return str;
}

bool UndoSendManager::UndoSendManagerInfo::isValid() const
{
    return index != -1 && delay != -1;
}
