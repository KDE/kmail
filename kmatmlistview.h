/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>
  Copyright (c) 2007 Thomas McGuire <Thomas.McGuire@gmx.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef __KMAIL_KMATMLISTVIEW_H__
#define __KMAIL_KMATMLISTVIEW_H__

#include <QTreeWidgetItem>

class QCheckBox;
class KMMessagePart;

namespace KMail {
class AttachmentListView;
}

class KMAtmListViewItem : public QObject, public QTreeWidgetItem
{
  Q_OBJECT

public:
  explicit KMAtmListViewItem( KMail::AttachmentListView *parent, 
                              KMMessagePart* attachment );
  virtual ~KMAtmListViewItem();

  // A custom compare operator is needed because the size column is
  // human-readable and therefore doesn't sort correctly.
  virtual bool operator < ( const QTreeWidgetItem & other ) const;

  void setUncompressedMimeType( const QByteArray &type, const QByteArray &subtype )
  {
    mType = type; mSubtype = subtype;
  }
  void setAttachmentSize( int numBytes )
  {
    mAttachmentSize = numBytes;
  }
  void uncompressedMimeType( QByteArray & type, QByteArray & subtype ) const
  {
    type = mType; subtype = mSubtype;
  }
  void setUncompressedCodec( const QByteArray & codec ) { mCodec = codec; }
  QByteArray uncompressedCodec() const { return mCodec; }

  void setEncrypt( bool on );
  bool isEncrypt() const;
  void setSign( bool on );
  bool isSign() const;
  void setCompress( bool on );
  bool isCompress() const;

  // Returns a pointer to the KMMessagePart this item is associated with.
  KMMessagePart* attachment() const;

signals:
  void compress( KMAtmListViewItem* );
  void uncompress( KMAtmListViewItem* );

private slots:
  void slotCompress();

private:

  // This function adds a center-aligned checkbox to the specified column
  // and then returns that checkbox.
  QCheckBox* addCheckBox( int column );

  QCheckBox *mCBEncrypt;
  QCheckBox *mCBSign;
  QCheckBox *mCBCompress;
  QByteArray mType, mSubtype, mCodec;
  int mAttachmentSize;
  KMMessagePart* mAttachment;
  KMail::AttachmentListView *mParent;
};

#endif // __KMAIL_KMATMLISTVIEW_H__
