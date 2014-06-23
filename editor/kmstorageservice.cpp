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

#include "kmstorageservice.h"
#include "pimcommon/storageservice/storageservicemanager.h"

#include <KMessageBox>
#include <KLocalizedString>
#include <KActionMenu>

KMStorageService::KMStorageService(QWidget *parentWidget, QObject *parent)
    : QObject(parent),
      mParentWidget(parentWidget),
      mNumProgressUploadFile(0),
      mStorageManager(new PimCommon::StorageServiceManager(this))
{
    connect(mStorageManager, SIGNAL(uploadFileDone(QString,QString)), this, SLOT(slotUploadFileDone(QString,QString)));
    connect(mStorageManager, SIGNAL(uploadFileFailed(QString,QString)), this, SLOT(slotUploadFileFailed(QString,QString)));
    connect(mStorageManager, SIGNAL(shareLinkDone(QString,QString)), this, SLOT(slotShareLinkDone(QString,QString)));
    connect(mStorageManager, SIGNAL(uploadFileStart(PimCommon::StorageServiceAbstract*)), this, SLOT(slotUploadFileStart(PimCommon::StorageServiceAbstract*)));
    connect(mStorageManager, SIGNAL(actionFailed(QString,QString)), this, SLOT(slotActionFailed(QString,QString)));
}

KMStorageService::~KMStorageService()
{

}

void KMStorageService::slotUploadFileDone(const QString &serviceName, const QString &fileName)
{
    Q_UNUSED(serviceName);
    Q_UNUSED(fileName);
}

void KMStorageService::slotUploadFileFailed(const QString &serviceName, const QString &fileName)
{
    Q_UNUSED(serviceName);
    Q_UNUSED(fileName);
    KMessageBox::error(mParentWidget, i18n("An error occurred while sending the file."), i18n("Upload file"));
    --mNumProgressUploadFile;
}

void KMStorageService::slotShareLinkDone(const QString &serviceName, const QString &link)
{
    Q_UNUSED(serviceName);
    Q_EMIT insertShareLink(link);
    --mNumProgressUploadFile;
}

void KMStorageService::slotUploadFileStart(PimCommon::StorageServiceAbstract *service)
{
    Q_UNUSED(service);
    ++mNumProgressUploadFile;
}

void KMStorageService::slotActionFailed(const QString &serviceName, const QString &error)
{
    KMessageBox::error(mParentWidget, i18n("%1 return an error '%2'", serviceName, error), i18n("Error"));
    --mNumProgressUploadFile;
}

KActionMenu *KMStorageService::menuShareLinkServices() const
{
    return mStorageManager->menuShareLinkServices(mParentWidget);
}

int KMStorageService::numProgressUpdateFile() const
{
    return mNumProgressUploadFile;
}
