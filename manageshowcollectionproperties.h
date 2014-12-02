/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef MANAGESHOWCOLLECTIONPROPERTIES_H
#define MANAGESHOWCOLLECTIONPROPERTIES_H

#include <QObject>
#include <QPointer>
#include <foldercollection.h>

#include <progresswidget/progressmanager.h>

namespace Akonadi {
class CollectionPropertiesDialog;
}

class KJob;
class KMMainWidget;
class ManageShowCollectionProperties : public QObject
{
    Q_OBJECT
public:
    explicit ManageShowCollectionProperties(KMMainWidget *mainWidget, QObject *parent = 0);
    ~ManageShowCollectionProperties();

public slots:
    void slotFolderMailingListProperties();
    void slotShowFolderShortcutDialog();
    void slotShowExpiryProperties();
    void slotCollectionProperties();

private slots:
    void slotCollectionPropertiesContinued(KJob *job);
    void slotCollectionPropertiesFinished(KJob *job);

private:
    void showCollectionProperties(const QString &pageToShow);
    void showCollectionPropertiesContinued(const QString &pageToShow, QPointer<KPIM::ProgressItem> progressItem);

private:
    QHash<Akonadi::Collection::Id, QPointer<Akonadi::CollectionPropertiesDialog> > mHashDialogBox;
    QStringList mPages;
    KMMainWidget *mMainWidget;
};

#endif // MANAGESHOWCOLLECTIONPROPERTIES_H
