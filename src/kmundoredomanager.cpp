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

void KMUndoRedoManager::moveItems(const Akonadi::Item::List &items, const Akonadi::Collection &collection)
{
    auto job = new Akonadi::ItemMoveJob(items, collection, nullptr);
    connect(job, &Akonadi::ItemMoveJob::result, this, &KMUndoRedoManager::slotMoveResult);
}

void KMUndoRedoManager::slotMoveResult(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(kmkernel->mainWin(), i18n("Cannot move message. %1", job->errorString()));
    }
}

QUndoCommand *KMUndoRedoManager::newUndoMoveAction(const Akonadi::Collection &srcFolder, const Akonadi::Collection &destFolder)
{
    auto info = new KMUndoInfoMoveItems(this);
    info->setSrcFolder(srcFolder);
    info->setDestFolder(destFolder);
    info->setMoveToTrash(destFolder == CommonKernel->trashCollectionFolder());
    mUndoStack->push(info);
    return info;
}

void KMUndoRedoManager::addMsgToMoveAction(QUndoCommand *command, const Akonadi::Item &item)
{
    KMUndoInfoMoveItems *commandUndoInfoMoveItems = dynamic_cast<KMUndoInfoMoveItems *>(command);
    if (commandUndoInfoMoveItems) {
        commandUndoInfoMoveItems->addItem(item);
    }
}

KMUndoInfoMoveItems::KMUndoInfoMoveItems(KMUndoRedoManager *manager, QUndoCommand *parent)
    : QUndoCommand(parent)
    , mManager(manager)
{
}

void KMUndoInfoMoveItems::undo()
{
    mManager->moveItems(mItems, mDestFolder);
}

void KMUndoInfoMoveItems::redo()
{
    mManager->moveItems(mItems, mSrcFolder);
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
