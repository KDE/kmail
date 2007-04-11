/* -*- mode: C++; c-file-style: "gnu" -*-
 * KMAtmListViewItem Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMAIL_KMATMLISTVIEW_H__
#define __KMAIL_KMATMLISTVIEW_H__

#include <q3listview.h>

class KMComposeWin;
class MessageComposer;
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
