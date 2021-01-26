/*
   SPDX-FileCopyrightText: 2014-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MANAGESHOWCOLLECTIONPROPERTIES_H
#define MANAGESHOWCOLLECTIONPROPERTIES_H

#include <MailCommon/FolderSettings>
#include <QObject>
#include <QPointer>

#include <Libkdepim/ProgressManager>

namespace Akonadi
{
class CollectionPropertiesDialog;
}

class KJob;
class KMMainWidget;
class ManageShowCollectionProperties : public QObject
{
    Q_OBJECT
public:
    explicit ManageShowCollectionProperties(KMMainWidget *mainWidget, QObject *parent = nullptr);
    ~ManageShowCollectionProperties() override;

public Q_SLOTS:
    void slotFolderMailingListProperties();
    void slotShowFolderShortcutDialog();
    void slotShowExpiryProperties();
    void slotCollectionProperties();

private:
    Q_DISABLE_COPY(ManageShowCollectionProperties)
    void slotCollectionPropertiesContinued(KJob *job);
    void slotCollectionPropertiesFinished(KJob *job);
    void showCollectionProperties(const QString &pageToShow);
    void showCollectionPropertiesContinued(const QString &pageToShow, QPointer<KPIM::ProgressItem> progressItem);

    QHash<Akonadi::Collection::Id, QPointer<Akonadi::CollectionPropertiesDialog>> mHashDialogBox;
    KMMainWidget *const mMainWidget;
    const QStringList mPages;
};

#endif // MANAGESHOWCOLLECTIONPROPERTIES_H
