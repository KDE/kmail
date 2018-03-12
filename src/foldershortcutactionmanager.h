/* Copyright 2010 Thomas McGuire <mcguire@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef FOLDERSHORTCUTACTIONMANAGER_H
#define FOLDERSHORTCUTACTIONMANAGER_H

#include "kmail_export.h"

#include <AkonadiCore/Collection>

#include <QHash>
#include <QModelIndex>
#include <QObject>

class QAction;

class KActionCollection;

namespace KMail {
class FolderShortcutCommand : public QObject
{
    Q_OBJECT

public:
    FolderShortcutCommand(QWidget *mainwidget, const Akonadi::Collection &col);
    ~FolderShortcutCommand();

public Q_SLOTS:
    void start();
    /** Assign a QAction to the command which is used to trigger it. This
    * action will be deleted along with the command, so you don't need to
    * keep track of it separately. */
    void setAction(QAction *);

Q_SIGNALS:
    void selectCollectionFolder(const Akonadi::Collection &col);

private:
    Q_DISABLE_COPY(FolderShortcutCommand)
    Akonadi::Collection mCollectionFolder;
    QWidget *mMainWidget = nullptr;
    QAction *mAction = nullptr;
};

class KMAIL_EXPORT FolderShortcutActionManager : public QObject
{
    Q_OBJECT

public:
    explicit FolderShortcutActionManager(QWidget *parent, KActionCollection *actionCollection);
    void createActions();

public Q_SLOTS:

    /**
       * Updates the shortcut action for this collection. Call this when a shortcut was
       * added, removed or changed.
       */
    void shortcutChanged(const Akonadi::Collection &collection);

private:
    /**
       * Removes the shortcut actions associated with a folder.
       */
    void slotCollectionRemoved(const Akonadi::Collection &collection);

    void slotRowsInserted(const QModelIndex &parent, int start, int end);

private:
    Q_DISABLE_COPY(FolderShortcutActionManager)
    void updateShortcutsForIndex(const QModelIndex &parent, int start, int end);
    QHash< Akonadi::Collection::Id, FolderShortcutCommand * > mFolderShortcutCommands;
    KActionCollection *mActionCollection = nullptr;
    QWidget *mParent = nullptr;
};
}

#endif
