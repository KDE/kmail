/* -*- mode: C++; c-file-style: "gnu" -*-
 * KMAtmListViewItem Header File
 * Author: Markus Wuebben <markus.wuebben@kde.org>
 */
#ifndef __KMAIL_KMATMLISTVIEW_H__
#define __KMAIL_KMATMLISTVIEW_H__

#include <qlistview.h>
#include <qcstring.h>

class KMComposeWin;
class MessageComposer;
class QCheckBox;

class KMAtmListViewItem : public QObject, public QListViewItem
{
  Q_OBJECT

public:
  KMAtmListViewItem( QListView *parent );
  virtual ~KMAtmListViewItem();

  //A custom compare function is needed because the size column is
  //human-readable and therefore doesn't sort correctly.
  virtual int compare( QListViewItem *i, int col, bool ascending ) const;

  virtual void paintCell ( QPainter * p, const QColorGroup & cg, int column, int width, int align );

  void setUncompressedMimeType( const QCString & type, const QCString & subtype ) {
    mType = type; mSubtype = subtype;
  }
  void setAttachmentSize( int numBytes ) {
    mAttachmentSize = numBytes;
  }
  void uncompressedMimeType( QCString & type, QCString & subtype ) const {
    type = mType; subtype = mSubtype;
  }
  void setUncompressedCodec( const QCString &codec ) { mCodec = codec; }
  QCString uncompressedCodec() const { return mCodec; }

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

protected:

  void updateCheckBox( int headerSection, QCheckBox *cb );
  void updateAllCheckBoxes();

private:
  QCheckBox *mCBEncrypt;
  QCheckBox *mCBSign;
  QCheckBox *mCBCompress;
  QCString mType, mSubtype, mCodec;
  int mAttachmentSize;
};

#endif // __KMAIL_KMATMLISTVIEW_H__
