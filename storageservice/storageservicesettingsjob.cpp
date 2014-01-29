/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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

#include "storageservicesettingsjob.h"

using namespace PimCommon;

StorageServiceSettingsJob::StorageServiceSettingsJob(QObject *parent)
    : QObject(parent)
{
}

StorageServiceSettingsJob::~StorageServiceSettingsJob()
{

}

QString StorageServiceSettingsJob::youSendItApiKey() const
{
    //TODO customize it
    return QLatin1String("fnab8fkgwrka7v6zs2ycd34a");
}

QString StorageServiceSettingsJob::dropboxOauthConsumerKey() const
{
    //TODO customize it
    return QLatin1String("e40dvomckrm48ci");
}

QString StorageServiceSettingsJob::dropboxOauthSignature() const
{
    //TODO customize it
    return QLatin1String("0icikya464lny9g&");
}

QString StorageServiceSettingsJob::boxClientId() const
{
    return QLatin1String("o4sn4e0dvz50pd3ps6ao3qxehvqv8dyo");
}

QString StorageServiceSettingsJob::boxClientSecret() const
{
    return QLatin1String("wLdaOgrblYzi1Y6WN437wStvqighmSJt");
}

QString StorageServiceSettingsJob::hubicClientId() const
{
    return QLatin1String("api_hubic_zBKQ6UDUj2vDT7ciDsgjmXA78OVDnzJi");
}

QString StorageServiceSettingsJob::hubicClientSecret() const
{
    return QLatin1String("pkChgk2sRrrCEoVHmYYCglEI9E2Y2833Te5Vn8n2J6qPdxLU6K8NPUvzo1mEhyzf");
}

QString StorageServiceSettingsJob::dropboxRootPath() const
{
    return QLatin1String("dropbox");
}

QString StorageServiceSettingsJob::oauth2RedirectUrl() const
{
    return QLatin1String("https://bugs.kde.org/");
}

QString StorageServiceSettingsJob::ubuntuOneAttachmentVolume() const
{
    return QLatin1String("/~/KMail Attachments");
}

QString StorageServiceSettingsJob::hubicScope() const
{
    return QLatin1String("usage.r,account.r,credentials.r,links.wd");
}

QString StorageServiceSettingsJob::ubuntuOneTokenName() const
{
    return QLatin1String("kmail");
}

QString StorageServiceSettingsJob::gdriveClientId() const
{
    //TODO
    return QString();
}

QString StorageServiceSettingsJob::gdriveClientSecret() const
{
    //TODO
    return QString();
}

QString StorageServiceSettingsJob::defaultUploadFolder() const
{
    return QLatin1String("KMail Attachment");
}

#include "moc_storageservicesettingsjob.cpp"
