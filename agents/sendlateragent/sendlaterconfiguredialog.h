/*
   SPDX-FileCopyrightText: 2013-2023 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <QDialog>
class SendLaterWidget;
class SendLaterConfigureDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SendLaterConfigureDialog(QWidget *parent = nullptr);
    ~SendLaterConfigureDialog() override;

    Q_REQUIRED_RESULT QList<Akonadi::Item::Id> messagesToRemove() const;

public Q_SLOTS:
    void slotNeedToReloadConfig();

Q_SIGNALS:
    void sendNow(Akonadi::Item::Id);

private:
    void slotSave();
    void readConfig();
    void writeConfig();
    SendLaterWidget *const mWidget;
};
