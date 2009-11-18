/*  -*- mode: C++; c-file-style: "gnu" -*-
 *
 *  This file is part of KMail, the KDE mail client.
 *  Copyright (c) 2009 Philip Van Hoof <philip@codeminded.be>
 *
 *  KMail is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License,
 *  or (at your option) any later version.
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
#ifndef MAILMANAGERIMPL_H
#define MAILMANAGERIMPL_H

#include <QObject>
#include <QtDBus/QtDBus>

#include <kmime/kmime_message.h>

class QString;

namespace KMail {

  // This class implements the D-Bus interface
  // libkdepim/interfaces/org.freedesktop.email.metadata.Manager.xml
  class MailManagerImpl : public QObject,
                          protected QDBusContext
  {
    Q_OBJECT
  private:
    QList< QDBusObjectPath > registrars;
    void processMsgBase ( const KMime::Message::Ptr &msg, QStringList &subjects,
                          QVector<QStringList> &predicatesArray,
                          QVector<QStringList> &valuesArray );

  public:
    MailManagerImpl();
    void Register( const QDBusObjectPath &registrarPath, uint lastModseq );
  };
}

#endif
