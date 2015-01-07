/*  
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2003 Zack Rusin <zack@kde.org>
 *
 *  KMail is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License, version 2, as
 *  published by the Free Software Foundation.
 *
 *  KMail is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of this program with any edition of
 *  the Qt library by Trolltech AS, Norway (or with modified versions
 *  of Qt that use the same license as Qt), and distribute linked
 *  combinations including the two.  You must obey the GNU General
 *  Public License in all respects for all of the code used other than
 *  Qt.  If you modify this file, you may extend this exception to
 *  your version of the file, but you are not obligated to do so.  If
 *  you do not wish to do so, delete this exception statement from
 *  your version.
 */
#ifndef MAILSERVICEIMPL_H
#define MAILSERVICEIMPL_H

class QByteArray;
class QString;
#include <QObject>

namespace KMail
{

// This class implements the D-Bus interface
// libkdepim/interfaces/org.kde.mailtransport.service.xml
class MailServiceImpl : public QObject
{
    Q_OBJECT
public:
    MailServiceImpl();
    bool sendMessage(const QString &from, const QString &to,
                     const QString &cc, const QString &bcc,
                     const QString &subject, const QString &body,
                     const QStringList &attachments);

    bool sendMessage(const QString &from, const QString &to,
                     const QString &cc, const QString &bcc,
                     const QString &subject, const QString &body,
                     const QByteArray &attachment);

};
}

#endif
