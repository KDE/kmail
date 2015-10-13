/*
  Copyright (c) 2014-2015 Montel Laurent <montel@kde.org>

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

#ifndef StorageServiceSettingsJob_H
#define StorageServiceSettingsJob_H

#include <QObject>
#include "storageservice/interface/storageserviceinterface.h"

class StorageServiceSettingsJob : public PimCommon::ISettingsJob
{
public:
    StorageServiceSettingsJob();
    ~StorageServiceSettingsJob();

    QString youSendItApiKey() const Q_DECL_OVERRIDE;
    QString dropboxOauthConsumerKey() const Q_DECL_OVERRIDE;
    QString dropboxOauthSignature() const Q_DECL_OVERRIDE;
    QString boxClientId() const Q_DECL_OVERRIDE;
    QString boxClientSecret() const Q_DECL_OVERRIDE;
    QString hubicClientId() const Q_DECL_OVERRIDE;
    QString hubicClientSecret() const Q_DECL_OVERRIDE;
    QString dropboxRootPath() const Q_DECL_OVERRIDE;
    QString oauth2RedirectUrl() const Q_DECL_OVERRIDE;
    QString hubicScope() const Q_DECL_OVERRIDE;
    QString gdriveClientId() const Q_DECL_OVERRIDE;
    QString gdriveClientSecret() const Q_DECL_OVERRIDE;
    QString defaultUploadFolder() const Q_DECL_OVERRIDE;
};

#endif // StorageServiceSettingsJob_H
