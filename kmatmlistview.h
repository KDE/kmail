/* -*- mode: C++; c-file-style: "gnu" -*-
 * KMComposeWin Header File
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
  friend class ::KMComposeWin;
  friend class ::MessageComposer;

public:
  KMAtmListViewItem(QListView * parent);
  virtual ~KMAtmListViewItem();
  virtual void paintCell( QPainter * p, const QColorGroup & cg,
                          int column, int width, int align );

  void setUncompressedMimeType( const QCString & type, const QCString & subtype ) {
    mType = type; mSubtype = subtype;
  }
  void uncompressedMimeType( QCString & type, QCString & subtype ) const {
    type = mType; subtype = mSubtype;
  }
  void setUncompressedCodec( const QCString & codec ) { mCodec = codec; }
  QCString uncompressedCodec() const { return mCodec; }

signals:
  void compress( int );
  void uncompress( int );

protected:
  void enableCryptoCBs(bool on);
  void setEncrypt(bool on);
  bool isEncrypt();
  void setSign(bool on);
  bool isSign();
  void setCompress(bool on);
  bool isCompress();

private slots:
  void slotCompress();

private:
  QListView* mListview;
  QCheckBox* mCBEncrypt;
  QCheckBox* mCBSign;
  QCheckBox* mCBCompress;
  bool mCBSignEnabled, mCBEncryptEnabled;
  QCString mType, mSubtype, mCodec;
};


#endif // __KMAIL_KMATMLISTVIEW_H__
