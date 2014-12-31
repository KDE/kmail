/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#ifndef MANAGESERVERSIDESUBSCRIPTIONJOB_H
#define MANAGESERVERSIDESUBSCRIPTIONJOB_H

#include <QObject>
#include <QSharedPointer>
#include <foldercollection.h>
class QDBusPendingCallWatcher;
class ManageServerSideSubscriptionJob : public QObject
{
    Q_OBJECT
public:
    explicit ManageServerSideSubscriptionJob(QObject *parent=0);

    ~ManageServerSideSubscriptionJob();

    void start();
    void setCurrentFolder(const QSharedPointer<MailCommon::FolderCollection> &currentFolder);

    void setParentWidget(QWidget *parentWidget);

private slots:
    void slotConfigureSubscriptionFinished(QDBusPendingCallWatcher *watcher);
private:
    QSharedPointer<MailCommon::FolderCollection> mCurrentFolder;
    QWidget *mParentWidget;
};

#endif // MANAGESERVERSIDESUBSCRIPTIONJOB_H
