/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>

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

#include <q3listview.h>

class QCheckBox;

class KMAtmListViewItem : public QObject, public Q3ListViewItem
{
  Q_OBJECT

public:
  KMAtmListViewItem( Q3ListView *parent );
  virtual ~KMAtmListViewItem();

  //A custom compare function is needed because the size column is
  //human-readable and therefore doesn't sort correctly.
  virtual int compare( Q3ListViewItem *i, int col, bool ascending ) const;

  virtual void paintCell( QPainter *p, const QColorGroup &cg, int column,
                          int width, int align );

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

  void enableCryptoCBs( bool on );
  void setEncrypt( bool on );
  bool isEncrypt();
  void setSign( bool on );
  bool isSign();
  void setCompress( bool on );
  bool isCompress();

signals:
  void compress( int );
  void uncompress( int );

private slots:
  void slotCompress();
  void slotHeaderChange( int, int, int );
  void slotHeaderClick( int );

private:
  void updateCheckBox( int headerSection, QCheckBox *cb );
  void updateAllCheckBoxes();

  QCheckBox *mCBEncrypt;
  QCheckBox *mCBSign;
  QCheckBox *mCBCompress;
  QByteArray mType, mSubtype, mCodec;
  int mAttachmentSize;
};

#endif // __KMAIL_KMATMLISTVIEW_H__
