/** -*- c++ -*-
 * imapprogressdialog.h
 *
 * Copyright (c) 2002-2003 Klarälvdalens Datakonsult AB
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

#ifndef __KMAIL_IMAPPROGRESSDIALOG_H__
#define __KMAIL_IMAPPROGRESSDIALOG_H__

#include <qdialog.h>
#include <qlistview.h>

class QProgressBar;

namespace KMail {

class ProgressListViewItem : public QListViewItem {
public:
  ProgressListViewItem(  int pBColumn, int pPro, QListView * parent,
			 const QString&, const QString& = QString::null,
			 const QString& = QString::null, 
                         const QString& = QString::null,
			 const QString& = QString::null, 
                         const QString& = QString::null,
			 const QString& = QString::null, 
                         const QString& = QString::null);
  ProgressListViewItem(  int pBColumn, int pPro, QListView * parent,
                         ProgressListViewItem* after,
			 const QString&, const QString& = QString::null,
			 const QString& = QString::null, 
                         const QString& = QString::null,
			 const QString& = QString::null, 
                         const QString& = QString::null,
			 const QString& = QString::null, 
                         const QString& = QString::null);
  
  ~ProgressListViewItem();

  void setProgress( int progress );
  
protected:

  void paintCell( QPainter *p, const QColorGroup &cg, int column, int width, int alignm );
  
  int pbcol, prog;
  
  
  QProgressBar* mProgress;
};

class IMAPProgressDialog : public QDialog
{
    Q_OBJECT

public:
    IMAPProgressDialog( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, 
			WFlags fl = 0 );
    ~IMAPProgressDialog();
public slots:

  void syncState( const QString& folderName, int progress, const QString& syncStatus );

  void clear();
protected:
  virtual void closeEvent( QCloseEvent* );

  QListView* mSyncEditorListView;
  ProgressListViewItem* mPreviousItem;
};


}; // namespace KMail

#endif // __KMAIL_IMAPPROGRESSDIALOG_H__
