/*
   SPDX-FileCopyrightText: 2014-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Collection>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <QStringList>

namespace Akonadi
{
class CollectionPropertiesDialog;
}
namespace KPIM
{
class ProgressItem;
}

class KJob;
class KMMainWidget;
class ManageShowCollectionProperties : public QObject
{
    Q_OBJECT
public:
    explicit ManageShowCollectionProperties(KMMainWidget *mainWidget, QObject *parent = nullptr);
    ~ManageShowCollectionProperties() override;

    void slotFolderMailingListProperties();
    void slotShowFolderShortcutDialog();
    void slotShowExpiryProperties();
    void slotCollectionProperties();

private:
    void slotCollectionPropertiesContinued(KJob *job);
    void slotCollectionPropertiesFinished(KJob *job);
    void showCollectionProperties(const QString &pageToShow);
    void showCollectionPropertiesContinued(const QString &pageToShow, QPointer<KPIM::ProgressItem> progressItem);

    QHash<Akonadi::Collection::Id, QPointer<Akonadi::CollectionPropertiesDialog>> mHashDialogBox;
    KMMainWidget *const mMainWidget;
    const QStringList mPages;
};
