/*
   SPDX-FileCopyrightText: 2014-2020 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MANAGESHOWCOLLECTIONPROPERTIES_H
#define MANAGESHOWCOLLECTIONPROPERTIES_H

#include <QObject>
#include <QPointer>
#include <MailCommon/FolderSettings>

#include <Libkdepim/ProgressManager>

namespace Akonadi {
class CollectionPropertiesDialog;
}

class KJob;
class KMMainWidget;
class ManageShowCollectionProperties : public QObject
{
    Q_OBJECT
public:
    explicit ManageShowCollectionProperties(KMMainWidget *mainWidget, QObject *parent = nullptr);
    ~ManageShowCollectionProperties();

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

    QHash<Akonadi::Collection::Id, QPointer<Akonadi::CollectionPropertiesDialog> > mHashDialogBox;
    QStringList mPages;
    KMMainWidget *const mMainWidget;
};

#endif // MANAGESHOWCOLLECTIONPROPERTIES_H
