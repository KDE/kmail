/*
   SPDX-FileCopyrightText: 2014-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QObject>

class KMLaunchExternalComponent : public QObject
{
    Q_OBJECT
public:
    explicit KMLaunchExternalComponent(QWidget *parentWidget, QObject *parent = nullptr);
    ~KMLaunchExternalComponent() override;

    void slotConfigureMailMerge();
    void slotConfigureSendLater();
    void slotConfigureAutomaticArchiving();
    void slotConfigureFollowupReminder();
    void slotStartCertManager();
    void slotImportWizard();
    void slotExportData();
    void slotRunAddressBook();
    void slotImport();
    void slotAccountWizard();
    void slotFilterLogViewer();

    void slotShowNotificationHistory();

private:
    QWidget *const mParentWidget;
};
