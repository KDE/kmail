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
#include "foldershortcutactionmanager.h"

#include <MailCommon/FolderCollection>
#include "mailcommon/mailkernel.h"

#include <AkonadiCore/ChangeRecorder>
#include <AkonadiCore/EntityDisplayAttribute>
#include <AkonadiCore/EntityTreeModel>
#include <AkonadiCore/EntityMimeTypeFilterModel>

#include <QAction>
#include <KActionCollection>
#include <KLocalizedString>
#include <QIcon>

using namespace KMail;
using namespace MailCommon;

FolderShortcutCommand::FolderShortcutCommand(QWidget *mainwidget,
        const Akonadi::Collection &col)
    : QObject(mainwidget),
      mCollectionFolder(col),
      mMainWidget(mainwidget),
      mAction(nullptr)
{
    connect(this, SIGNAL(selectCollectionFolder(Akonadi::Collection)), mMainWidget, SLOT(slotSelectCollectionFolder(Akonadi::Collection)));
}

FolderShortcutCommand::~FolderShortcutCommand()
{
    if (mAction && mAction->parentWidget()) {
        mAction->parentWidget()->removeAction(mAction);
    }
    delete mAction;
}

void FolderShortcutCommand::start()
{
    Q_EMIT selectCollectionFolder(mCollectionFolder);
}

void FolderShortcutCommand::setAction(QAction *action)
{
    mAction = action;
}

FolderShortcutActionManager::FolderShortcutActionManager(QWidget *parent,
        KActionCollection *actionCollection
                                                        )
    : QObject(parent),
      mActionCollection(actionCollection),
      mParent(parent)
{
}

void FolderShortcutActionManager::createActions()
{
    // When this function is called, the ETM has not finished loading yet. Therefore, when new
    // rows are inserted in the ETM, see if we have new collections that we can assign shortcuts
    // to.
    const QAbstractItemModel *model = KernelIf->collectionModel();
    connect(model, &QAbstractItemModel::rowsInserted,
            this, &FolderShortcutActionManager::slotRowsInserted, Qt::UniqueConnection);
    connect(KernelIf->folderCollectionMonitor(), &Akonadi::Monitor::collectionRemoved,
            this, &FolderShortcutActionManager::slotCollectionRemoved, Qt::UniqueConnection);

    const int rowCount(model->rowCount());
    if (rowCount > 0) {
        updateShortcutsForIndex(QModelIndex(), 0, rowCount - 1);
    }
}

void FolderShortcutActionManager::slotRowsInserted(const QModelIndex &parent, int start, int end)
{
    updateShortcutsForIndex(parent, start, end);
}

void FolderShortcutActionManager::updateShortcutsForIndex(const QModelIndex &parent,
        int start, int end)
{
    QAbstractItemModel *model = KernelIf->collectionModel();
    for (int i = start; i <= end; ++i) {
        if (model->hasIndex(i, 0, parent)) {
            const QModelIndex child = model->index(i, 0, parent);
            Akonadi::Collection collection =
                model->data(child, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
            if (collection.isValid()) {
                shortcutChanged(collection);
            }
            if (model->rowCount(child) > 0) {
                updateShortcutsForIndex(child, 0, model->rowCount(child) - 1);
            }
        }
    }
}

void FolderShortcutActionManager::slotCollectionRemoved(const Akonadi::Collection &col)
{
    delete mFolderShortcutCommands.take(col.id());
}

void FolderShortcutActionManager::shortcutChanged(const Akonadi::Collection &col)
{
    // remove the old one, no autodelete in Qt4
    slotCollectionRemoved(col);
    const QSharedPointer<FolderCollection> folderCollection(FolderCollection::forCollection(col, false));
    const QKeySequence shortcut(folderCollection->shortcut());
    if (shortcut.isEmpty()) {
        return;
    }

    FolderShortcutCommand *command = new FolderShortcutCommand(mParent, col);
    mFolderShortcutCommands.insert(col.id(), command);

    QIcon icon(QStringLiteral("folder"));
    if (col.hasAttribute<Akonadi::EntityDisplayAttribute>() &&
            !col.attribute<Akonadi::EntityDisplayAttribute>()->iconName().isEmpty()) {
        icon = QIcon(col.attribute<Akonadi::EntityDisplayAttribute>()->iconName());
    }

    const QString actionLabel = i18n("Folder Shortcut %1", col.name());
    QString actionName = i18n("Folder Shortcut %1", QString::number(col.id()));
    actionName.replace(QLatin1Char(' '), QLatin1Char('_'));
    QAction *action = mActionCollection->addAction(actionName);
    // The folder shortcut is set in the folder shortcut dialog.
    // The shortcut set in the shortcut dialog would not be saved back to
    // the folder settings correctly.
    mActionCollection->setShortcutsConfigurable(action, false);
    action->setText(actionLabel);
    action->setShortcut(shortcut);
    action->setIcon(icon);

    connect(action, &QAction::triggered, command, &FolderShortcutCommand::start);
    command->setAction(action);   // will be deleted along with the command
}

