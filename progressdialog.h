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
 */

#ifndef __KMAIL_PROGRESSDIALOG_H__
#define __KMAIL_PROGRESSDIALOG_H__

#include <qdialog.h>
#include <qlistview.h>
namespace KMail {
  class ProgressItem;
  class TransactionItemListView;
}
using KMail::TransactionItemListView;
using KMail::ProgressItem;

class QProgressBar;

namespace KMail {

class TransactionItemListView : public QListView {
  Q_OBJECT
public:
  TransactionItemListView( QWidget * parent = 0,
                           const char * name = 0,
                           WFlags f = 0 );

  virtual ~TransactionItemListView()
  {}

  void resizeEvent( QResizeEvent* e );
public slots:
  void slotAdjustGeometry();
};

class TransactionItem : public QObject, public QListViewItem {

  Q_OBJECT;

public:

  TransactionItem(  QListView * parent,
                    QListViewItem *lastItem,
                    ProgressItem* item );
  TransactionItem(  QListViewItem * parent,
                    ProgressItem* item );
  ~TransactionItem();

  void setProgress( int progress );
  void setStatus( const QString& );

  void adjustGeometry();
  ProgressItem* item() const { return mItem; }

public slots:
  void slotItemCanceled();

protected:
  QProgressBar* mProgress;
  QPushButton* mCancelButton;
  ProgressItem *mItem;

private:
  void init( ProgressItem* item );

};

class ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    ProgressDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE,
                        WFlags fl = 0 );
    ~ProgressDialog();

protected slots:

  void clear();

  void slotTransactionAdded( ProgressItem *item );
  void slotTransactionCompleted( ProgressItem *item );
  void slotTransactionCanceled( ProgressItem *item );
  void slotTransactionProgress( ProgressItem *item, unsigned int progress );
  void slotTransactionStatus( ProgressItem *item, const QString& );

  void slotHide();
protected:
  virtual void closeEvent( QCloseEvent* );
  virtual void showEvent( QShowEvent* );

  TransactionItemListView* mListView;
  TransactionItem* mPreviousItem;
  QMap< const ProgressItem*, TransactionItem* > mTransactionsToListviewItems;
};


} // namespace KMail

#endif // __KMAIL_PROGRESSDIALOG_H__
