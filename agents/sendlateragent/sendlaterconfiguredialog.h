/*
   SPDX-FileCopyrightText: 2013-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SENDLATERCONFIGUREDIALOG_H
#define SENDLATERCONFIGUREDIALOG_H

#include <QDialog>
#include <AkonadiCore/Item>
class SendLaterWidget;
class SendLaterConfigureDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SendLaterConfigureDialog(QWidget *parent = nullptr);
    ~SendLaterConfigureDialog();

    Q_REQUIRED_RESULT QVector<Akonadi::Item::Id> messagesToRemove() const;

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

#endif // SENDLATERCONFIGUREDIALOG_H
