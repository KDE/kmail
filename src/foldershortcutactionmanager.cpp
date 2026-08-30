/* SPDX-FileCopyrightText: 2010 Thomas McGuire <mcguire@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/
#include "foldershortcutactionmanager.h"

#include <MailCommon/FolderSettings>
#include <MailCommon/MailKernel>

#include <Akonadi/ChangeRecorder>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <Akonadi/EntityTreeModel>

#include <KActionCollection>
#include <KLocalizedString>
#include <QAction>
#include <QIcon>
#include <QModelIndex>

using namespace KMail;
using namespace MailCommon;
using namespace Qt::Literals::StringLiterals;

FolderShortcutCommand::FolderShortcutCommand(QWidget *mainwidget, const Akonadi::Collection &col)
    : QObject(mainwidget)
    , mCollectionFolder(col)
    , mMainWidget(mainwidget)
{
    connect(this, SIGNAL(selectCollectionFolder(Akonadi::Collection)), mMainWidget, SLOT(slotSelectCollectionFolder(Akonadi::Collection)));
}

FolderShortcutCommand::~FolderShortcutCommand()
{
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

FolderShortcutActionManager::FolderShortcutActionManager(QWidget *parent, KActionCollection *actionCollection)
    : QObject(parent)
    , mActionCollection(actionCollection)
    , mParent(parent)
{
}

void FolderShortcutActionManager::createActions()
{
    // When this function is called, the ETM has not finished loading yet. Therefore, when new
    // rows are inserted in the ETM, see if we have new collections that we can assign shortcuts
    // to.
    const QAbstractItemModel *model = KernelIf->collectionModel();
    connect(model, &QAbstractItemModel::rowsInserted, this, &FolderShortcutActionManager::slotRowsInserted, Qt::UniqueConnection);
    connect(KernelIf->folderCollectionMonitor(),
            &Akonadi::Monitor::collectionRemoved,
            this,
            &FolderShortcutActionManager::slotCollectionRemoved,
            Qt::UniqueConnection);

    if (const int rowCount(model->rowCount()); rowCount > 0) {
        updateShortcutsForIndex(QModelIndex(), 0, rowCount - 1);
    }
}

void FolderShortcutActionManager::slotRowsInserted(const QModelIndex &parent, int start, int end)
{
    updateShortcutsForIndex(parent, start, end);
}

void FolderShortcutActionManager::updateShortcutsForIndex(const QModelIndex &parent, int start, int end)
{
    QAbstractItemModel *model = KernelIf->collectionModel();
    for (int i = start; i <= end; ++i) {
        if (model->hasIndex(i, 0, parent)) {
            const QModelIndex child = model->index(i, 0, parent);
            if (auto collection = model->data(child, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>(); collection.isValid()) {
                shortcutChanged(collection);
            }
            if (model->hasChildren(child)) {
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
    const QSharedPointer<FolderSettings> folderCollection(FolderSettings::forCollection(col, false));
    const QKeySequence shortcut(folderCollection->shortcut());
    if (shortcut.isEmpty()) {
        return;
    }

    auto command = new FolderShortcutCommand(mParent, col);
    mFolderShortcutCommands.insert(col.id(), command);

    QIcon icon = QIcon::fromTheme(u"folder"_s);
    if (const auto *attribute = col.attribute<Akonadi::EntityDisplayAttribute>(); attribute && !attribute->iconName().isEmpty()) {
        icon = QIcon::fromTheme(attribute->iconName());
    }

    const QString actionLabel = i18n("Folder Shortcut %1", col.name());
    // The action name is an internal identifier used as config key, it must not be translated.
    const QString actionName = u"folder_shortcut_%1"_s.arg(col.id());
    QAction *action = mActionCollection->addAction(actionName);
    // The folder shortcut is set in the folder shortcut dialog.
    // The shortcut set in the shortcut dialog would not be saved back to
    // the folder settings correctly.
    mActionCollection->setShortcutsConfigurable(action, false);
    action->setText(actionLabel);
    mActionCollection->setDefaultShortcut(action, shortcut);
    action->setIcon(icon);

    connect(action, &QAction::triggered, command, &FolderShortcutCommand::start);
    command->setAction(action); // will be deleted along with the command
}

#include "moc_foldershortcutactionmanager.cpp"
