/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef KMAIL_MESSAGECOPYHELPER_H
#define KMAIL_MESSAGECOPYHELPER_H

#include <QPointer>
#include <QObject>
#include <QList>
#include <QMap>

#include <maillistdrag.h>

class KMCommand;
class KMFolder;
class KMMsgBase;

namespace KMail {

/**
  Helper class to copy/move a set of messages defined by their serial
  numbers from arbitrary folders into a common destination folder.
*/
class MessageCopyHelper : public QObject
{
  Q_OBJECT

  public:
    /**
      Creates new MessageCopyHelper object to copy the given messages
      to the specified destination folder.
      @param msgs List of serial numbers.
      @param dest Destination folder.
      @param move If set to true, messages will be moved instead of copied
      @param parent The parent object.
    */
    MessageCopyHelper( const QList<quint32> &msgs, KMFolder *dest,
                       bool move, QObject *parent = 0 );

    /**
      Converts a MailList into a serial number list.
    */
    static QList<quint32> serNumListFromMailList( const KPIM::MailList &list );

    /**
      Converts a KMMsgsBase* list into a serial number list.
    */
    static QList<quint32> serNumListFromMsgList( QList<KMMsgBase*> list );

  private slots:
    void copyCompleted( KMCommand *cmd );

  private:
    QMap<QPointer<KMFolder>, int> mOpenFolders;
};

}

#endif
