/*
   SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kmundoredomanager.h"
#include "kmkernel.h"
#include <Akonadi/ItemMoveJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <QUndoStack>

using namespace KMail;
KMUndoRedoManager::KMUndoRedoManager(QObject *parent)
    : QObject{parent}
    , mUndoStack(new QUndoStack(this))
{
}

KMUndoRedoManager::~KMUndoRedoManager() = default;

void KMUndoRedoManager::moveItems()
{
#if 0
    auto job = new Akonadi::ItemMoveJob(mItems, mSrcFolder, nullptr);
    connect(job, &Akonadi::ItemMoveJob::result, this, &KMUndoRedoManager::slotMoveResult);
#endif
    // TODO
}

void KMUndoRedoManager::slotMoveResult(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(kmkernel->mainWin(), i18n("Cannot move message. %1", job->errorString()));
    }
}

int KMUndoRedoManager::newUndoMoveAction(const Akonadi::Collection &srcFolder, const Akonadi::Collection &destFolder)
{
    auto info = new KMUndoInfoMoveItems(this);
    mUndoStack->push(info);
#if 0
    info->id = ++mLastId;
    info->srcFolder = srcFolder;
    info->destFolder = destFolder;
    info->moveToTrash = (destFolder == CommonKernel->trashCollectionFolder());
    if (static_cast<int>(mStack.count()) == mSize) {
        delete mStack.last();
        mStack.removeLast();
    }
    mStack.prepend(info);
    Q_EMIT undoStackChanged();
    return info->id;
#endif
    // TODO
    return -1;
}

void KMUndoRedoManager::addMsgToMoveAction(int undoId, const Akonadi::Item &item)
{
    // TODO push push()
    // TODO
}

KMUndoInfoMoveItems::KMUndoInfoMoveItems(KMUndoRedoManager *manager, QUndoCommand *parent)
    : QUndoCommand(parent)
    , mManager(manager)
{
}

void KMUndoInfoMoveItems::undo()
{
#if 0
    auto job = new Akonadi::ItemMoveJob(mItems, mSrcFolder, nullptr);
    connect(job, &Akonadi::ItemMoveJob::result, this, &KMUndoInfoMoveItems::slotMoveResult);
#endif
    // TODO
}

void KMUndoInfoMoveItems::redo()
{
    // TODO
}

Akonadi::Item::List KMUndoInfoMoveItems::items() const
{
    return mItems;
}

void KMUndoInfoMoveItems::setItems(const Akonadi::Item::List &newItems)
{
    mItems = newItems;
}

void KMUndoInfoMoveItems::addItem(const Akonadi::Item &item)
{
    mItems.append(item);
}

Akonadi::Collection KMUndoInfoMoveItems::srcFolder() const
{
    return mSrcFolder;
}

void KMUndoInfoMoveItems::setSrcFolder(const Akonadi::Collection &newSrcFolder)
{
    mSrcFolder = newSrcFolder;
}

Akonadi::Collection KMUndoInfoMoveItems::destFolder() const
{
    return mDestFolder;
}

void KMUndoInfoMoveItems::setDestFolder(const Akonadi::Collection &newDestFolder)
{
    mDestFolder = newDestFolder;
}

bool KMUndoInfoMoveItems::moveToTrash() const
{
    return mMoveToTrash;
}

void KMUndoInfoMoveItems::setMoveToTrash(bool newMoveToTrash)
{
    mMoveToTrash = newMoveToTrash;
}
