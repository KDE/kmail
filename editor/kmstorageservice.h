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

#ifndef KMSTORAGESERVICE_H
#define KMSTORAGESERVICE_H

#include <QObject>
namespace PimCommon
{
class StorageServiceManager;
class StorageServiceAbstract;
}
class KActionMenu;
class KMStorageService : public QObject
{
    Q_OBJECT
public:
    explicit KMStorageService(QWidget *parentWidget, QObject *parent = Q_NULLPTR);
    ~KMStorageService();

    KActionMenu *menuShareLinkServices() const;
    int numProgressUpdateFile() const;

Q_SIGNALS:
    void insertShareLink(const QString &link);

private Q_SLOTS:
    void slotUploadFileDone(const QString &serviceName, const QString &fileName);
    void slotUploadFileFailed(const QString &serviceName, const QString &fileName);
    void slotShareLinkDone(const QString &serviceName, const QString &link);
    void slotUploadFileStart(PimCommon::StorageServiceAbstract *service);
    void slotActionFailed(const QString &serviceName, const QString &error);

private:
    QWidget *mParentWidget;
    int mNumProgressUploadFile;
    PimCommon::StorageServiceManager *mStorageManager;
};

#endif // KMSTORAGESERVICE_H
