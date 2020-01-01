/*
  Copyright (c) 2015-2020 Laurent Montel <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#ifndef REMOVEDUPLICATEMESSAGEINFOLDERANDSUBFOLDERJOB_H
#define REMOVEDUPLICATEMESSAGEINFOLDERANDSUBFOLDERJOB_H

#include <QObject>
#include <AkonadiCore/Collection>
class KJob;
namespace KPIM {
class ProgressItem;
}
class RemoveDuplicateMessageInFolderAndSubFolderJob : public QObject
{
    Q_OBJECT
public:
    explicit RemoveDuplicateMessageInFolderAndSubFolderJob(QObject *parent = nullptr, QWidget *parentWidget = nullptr);
    ~RemoveDuplicateMessageInFolderAndSubFolderJob();

    void start();

    void setTopLevelCollection(const Akonadi::Collection &topLevelCollection);

private:
    Q_DISABLE_COPY(RemoveDuplicateMessageInFolderAndSubFolderJob)
    void slotFetchCollectionFailed();
    void slotFetchCollectionDone(const Akonadi::Collection::List &list);
    void slotFinished(KJob *job);
    void slotRemoveDuplicatesUpdate(KJob *job, const QString &description);
    void slotRemoveDuplicatesCanceled(KPIM::ProgressItem *item);
    Akonadi::Collection mTopLevelCollection;
    QWidget *mParentWidget = nullptr;
};

#endif // REMOVEDUPLICATEMESSAGEINFOLDERANDSUBFOLDERJOB_H
