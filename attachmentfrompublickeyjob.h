/*
  Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef KMAIL_ATTACHMENTFROMPUBLICKEY_H
#define KMAIL_ATTACHMENTFROMPUBLICKEY_H

#include <libkdepim/attachmentloadjob.h>

namespace KMail {

/**
*/
// TODO I have no idea how to test this.  Have a fake keyring???
class AttachmentFromPublicKeyJob : public KPIM::AttachmentLoadJob
{
  Q_OBJECT

  public:
    explicit AttachmentFromPublicKeyJob( const QString &fingerprint, QObject *parent = 0 );
    virtual ~AttachmentFromPublicKeyJob();

    QString fingerprint() const;
    void setFingerprint( const QString &fingerprint );

  protected slots:
    virtual void doStart();

  private:
    class Private;
    friend class Private;
    Private *const d;

    Q_PRIVATE_SLOT( d, void exportResult( GpgME::Error, QByteArray ) )
};

} // namespace KMail

#endif
