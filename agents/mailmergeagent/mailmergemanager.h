/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <AkonadiCore/Item>
#include <QObject>

class MailMergeManager : public QObject
{
    Q_OBJECT
public:
    explicit MailMergeManager(QObject *parent = nullptr);
    ~MailMergeManager() override;
    Q_REQUIRED_RESULT QString printDebugInfo() const;
    void load(bool state = false);
    void stopAll();
    Q_REQUIRED_RESULT bool itemRemoved(Akonadi::Item::Id id);

Q_SIGNALS:
    void needUpdateConfigDialogBox();
};
