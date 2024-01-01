/*
   SPDX-FileCopyrightText: 2021-2024 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <QObject>

class MailMergeManager : public QObject
{
    Q_OBJECT
public:
    explicit MailMergeManager(QObject *parent = nullptr);
    ~MailMergeManager() override;
    [[nodiscard]] QString printDebugInfo() const;
    void load(bool state = false);
    void stopAll();
    [[nodiscard]] bool itemRemoved(Akonadi::Item::Id id);

Q_SIGNALS:
    void needUpdateConfigDialogBox();
};
