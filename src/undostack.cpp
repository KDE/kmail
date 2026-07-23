/*
    This file is part of KMail

    SPDX-FileCopyrightText: 1999 Waldo Bastian <bastian@kde.org>
    SPDX-FileCopyrightText: 2003 Zack Rusin <zack@kde.org>
    SPDX-FileCopyrightText: 2026 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: GPL-2.0-only
*/

#include "undostack.h"

#include "kmkernel.h"
#include <Akonadi/ItemMoveJob>
#include <KJob>

#include <KLocalizedString>
#include <KMessageBox>

using namespace KMail;

UndoStack::UndoStack(int size)
    : QObject(nullptr)
    , mSize(size)
{
}

UndoStack::~UndoStack()
{
    clear();
}

void UndoStack::clear()
{
    qDeleteAll(mStack);
    mStack.clear();
}

QString UndoStack::undoInfo() const
{
    if (!mStack.isEmpty()) {
        UndoInfoBase *info = mStack.first();
        return info->undoInfo();
    } else {
        return {};
    }
}

int UndoStack::newUndoMoveAction(const Akonadi::Collection &srcFolder, const Akonadi::Collection &destFolder)
{
    auto info = new UndoInfoMoveItems;
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
}

void UndoStack::addMsgToMoveAction(int undoId, const Akonadi::Item &item)
{
    if (!mCachedInfo || mCachedInfo->id != undoId) {
        QList<UndoInfoBase *>::const_iterator itr = mStack.constBegin();
        while (itr != mStack.constEnd()) {
            if ((*itr)->id == undoId) {
                mCachedInfo = (*itr);
                break;
            }
            ++itr;
        }
    }

    Q_ASSERT(mCachedInfo);
    UndoInfoMoveItems *moveItems = dynamic_cast<UndoInfoMoveItems *>(mCachedInfo);
    moveItems->items.append(item);
}

bool UndoStack::isEmpty() const
{
    return mStack.isEmpty();
}

void UndoStack::undo()
{
    if (!mStack.isEmpty()) {
        UndoInfoBase *info = mStack.takeFirst();
        info->undo();
        Q_EMIT undoStackChanged();
    } else {
        // Sorry.. stack is empty..
        KMessageBox::error(kmkernel->mainWin(), i18n("There is nothing to undo."));
    }
}

void UndoInfoMoveItems::slotMoveResult(KJob *job)
{
    if (job->error()) {
        KMessageBox::error(kmkernel->mainWin(), i18n("Cannot move message. %1", job->errorString()));
    }
    deleteLater();
}

UndoInfoMoveItems::UndoInfoMoveItems() = default;

UndoInfoMoveItems::~UndoInfoMoveItems() = default;

QString UndoInfoMoveItems::undoInfo() const
{
    return moveToTrash ? i18n("Move To Trash") : i18np("Move Message", "Move Messages", items.count());
}

void UndoInfoMoveItems::undo()
{
    auto job = new Akonadi::ItemMoveJob(items, srcFolder, this);
    connect(job, &Akonadi::ItemMoveJob::result, this, &UndoInfoMoveItems::slotMoveResult);
}

#include "moc_undostack.cpp"
