/** -*- c++ -*-
 * progressdialog.h
 *
 *  Copyright (c) 2004 Till Adam <adam@kde.org>
 *  based on imapprogressdialog.cpp ,which is
 *  Copyright (c) 2002-2003 Klar�vdalens Datakonsult AB
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
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

#ifndef __KPIM_PROGRESSDIALOG_H__
#define __KPIM_PROGRESSDIALOG_H__

#include <qdialog.h>
#include <qlistview.h>
#include <qlabel.h>
#include <qvbox.h>
#include "overlaywidget.h"

class QProgressBar;
class QScrollView;
class QFrame;

namespace KPIM {
class ProgressItem;
class TransactionItemListView;
class TransactionItem;
class SSLLabel;

class TransactionItemView : public QScrollView {
  Q_OBJECT
public:
  TransactionItemView( QWidget * parent = 0,
                       const char * name = 0,
                       WFlags f = 0 );

  virtual ~TransactionItemView()
  {}
  TransactionItem* addTransactionItem( ProgressItem *item, bool first );


  QSize sizeHint() const;
  QSize minimumSizeHint() const;
public slots:
  void slotLayoutFirstItem();

protected:
  virtual void resizeContents ( int w, int h );

private:
  QVBox *                  mBigBox;
};

class TransactionItem : public QVBox {

  Q_OBJECT;

public:
  TransactionItem( QWidget * parent,
                   ProgressItem* item, bool first );

  ~TransactionItem();

  void hideHLine();

  void setProgress( int progress );
  void setLabel( const QString& );
  void setStatus( const QString& );
  void setCrypto( bool );

  ProgressItem* item() const { return mItem; }

  void addSubTransaction( ProgressItem *item);

  // The progressitem is deleted immediately, we take 5s to go out,
  // so better not use mItem during this time.
  void setItemComplete() { mItem = 0; }

public slots:
  void slotItemCanceled();

protected:
  QProgressBar* mProgress;
  QPushButton*  mCancelButton;
  QLabel*       mItemLabel;
  QLabel*       mItemStatus;
  QFrame*       mFrame;
  SSLLabel*     mSSLLabel;
  ProgressItem* mItem;
};

class ProgressDialog : public OverlayWidget
{
    Q_OBJECT

public:
  ProgressDialog( QWidget* alignWidget, QWidget* parent, const char* name = 0 );
  ~ProgressDialog();

  void setVisible( bool b );

public slots:
  void slotToggleVisibility();

protected slots:
  void slotTransactionAdded( ProgressItem *item );
  void slotTransactionCompleted( ProgressItem *item );
  void slotTransactionCanceled( ProgressItem *item );
  void slotTransactionProgress( ProgressItem *item, unsigned int progress );
  void slotTransactionStatus( ProgressItem *item, const QString& );
  void slotTransactionLabel( ProgressItem *item, const QString& );
  void slotTransactionUsesCrypto( ProgressItem *item, bool );

  void slotClose();
  void slotHide();

signals:
  void visibilityChanged( bool );

protected:
  virtual void closeEvent( QCloseEvent* );

  TransactionItemView* mScrollView;
  TransactionItem* mPreviousItem;
  QMap< const ProgressItem*, TransactionItem* > mTransactionsToListviewItems;
};


} // namespace KPIM

#endif // __KPIM_PROGRESSDIALOG_H__
