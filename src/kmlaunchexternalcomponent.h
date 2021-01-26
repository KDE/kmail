/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef KMLAUNCHEXTERNALCOMPONENT_H
#define KMLAUNCHEXTERNALCOMPONENT_H

#include <QObject>

class KMLaunchExternalComponent : public QObject
{
    Q_OBJECT
public:
    explicit KMLaunchExternalComponent(QWidget *parentWidget, QObject *parent = nullptr);
    ~KMLaunchExternalComponent() override;

public Q_SLOTS:
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

private:
    Q_DISABLE_COPY(KMLaunchExternalComponent)
    QWidget *const mParentWidget;
};

#endif // KMLAUNCHEXTERNALCOMPONENT_H
