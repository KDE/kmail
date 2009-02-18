/*
  Copyright (c) 2001 Heiko Hund <heiko@ist.eigentlich.net>
  Copyright (c) 2001 Thorsten Zachmann <T.Zachmann@zagge.de>
  Copyright (c) 2008 Thomas McGuire <Thomas.McGuire@gmx.net>

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

#ifndef KMPOPFILTERCNFRMDLG_H
#define KMPOPFILTERCNFRMDLG_H

#include "kmpopheaders.h"

#include <kdialog.h>

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMap>
#include <QList>

class QButtonGroup;
class QKeyEvent;
class QRadioButton;
class QSignalMapper;
class QString;
class QWidget;

class KMPopFilterCnfrmDlg;

// This treewidget shows KMPopHeadersViewItems.
// It has 3 columns for radiobuttons, which show the current action of the item.
// Additionally, it shows columns for subject, date, sender, receiver etc.
// It automatically changes the action of all items when the column header
// of one of the action column is clicked.
// Also, the right and left keys can be used to select a different action
// for the selected items.
class KMPopHeadersView : public QTreeWidget
{
  Q_OBJECT

public:

  explicit KMPopHeadersView( QWidget *parent, KMPopFilterCnfrmDlg *dialog );
  ~KMPopHeadersView();

  static KMPopFilterAction mapToAction( int column ) {
    return static_cast<KMPopFilterAction>( column );
  }

  static int mapToColumn( KMPopFilterAction action ) {
    return static_cast<int>( action );
  }

protected:

  virtual void keyPressEvent( QKeyEvent *e );

public slots:

  // Call this, and only this, to sort a column (other than the first three
  // columns)
  void slotSectionClicked( int column );

protected slots:

  // Called when of the item's radiobuttons is clicked. It changes the actions
  // of all selected items.
  void slotRadioButtonClicked( QTreeWidgetItem* item, int column );

private:

  KMPopFilterCnfrmDlg *mDialog;
  int mLastSortColumn;
  Qt::SortOrder mLastSortOrder;
};


// This item represents the header of a message.
// It has three radiobuttons to select the action of this message (download,
// download later or delete).
class KMPopHeadersViewItem : public QObject, public QTreeWidgetItem
{
  Q_OBJECT

public:
  KMPopHeadersViewItem( KMPopHeadersView *parent, KMPopFilterAction action );
  ~KMPopHeadersViewItem();

  // The date and size columns can't be sorted lexically, so they need to be
  // remembered in a member variable and then compared with a custom compare
  // operator
  void setMessageSize( int messageSize ) { mSizeOfMessage = messageSize; }
  void setIsoDate( const QString & isoDate ) { mIsoDate = isoDate; }
  virtual bool operator < ( const QTreeWidgetItem & other ) const;

  KMPopFilterAction action() const { return mAction; }
  void setAction( KMPopFilterAction action );

protected slots:
  void slotActionChanged( int column );

signals:
  void radioButtonClicked( QTreeWidgetItem* item, int column );

protected:
  QRadioButton* addRadioButton( int column );
  QRadioButton* buttonForAction( KMPopFilterAction action ) const;

protected:
  QButtonGroup *mActionGroup;
  QRadioButton *mDownButton;
  QRadioButton *mDeleteButton;
  QRadioButton *mLaterButton;
  QSignalMapper *mMapper;

  KMPopHeadersView *mParent;
  KMPopFilterAction mAction;

  QString mIsoDate;
  int mSizeOfMessage;
};


// This dialog shows treewidgets which show messages. The user can choose
// to download, download later or delete these messages by clicking on the
// messages' radiobuttons. The displayed information about these messages is
// gained from the KMPopHeaders list given in the constructor.
//
// The constructor gets a list of KMPopHeaders. When the user changes the action
// of one item, the corresponding KMPopHeaders action is also changed. This is
// internally done by keeping a map of treewidget items and KMPopHeaders.
//
// The dialog consists of two treewidgets: The upper one shows the messages which
// have no action set by filters, and the lower one shows the messages where a filter
// rule already matched. The lower one is not shown by default, but can be enabled
// by a checkbox (see slotToggled).
class KMPopFilterCnfrmDlg : public KDialog
{
  friend class ::KMPopHeadersView;
  Q_OBJECT

protected:
  KMPopFilterCnfrmDlg() { }
  QMap<QTreeWidgetItem*, KMPopHeaders*> mItemMap;
  QList<KMPopHeadersViewItem *> mDelList;
  QList<KMPopHeaders *> mDDLList;
  KMPopHeadersView *mFilteredHeaders;
  bool mLowerBoxVisible;
  bool mShowLaterMsgs;
  void setupLVI( KMPopHeadersViewItem *lvi, KMMessage *msg );

public:
  KMPopFilterCnfrmDlg( const QList<KMPopHeaders *> & headers,
                       const QString & account,
                       bool showLaterMsgs = false, QWidget *parent = 0 );
  ~KMPopFilterCnfrmDlg();

  // Don't call this from the outside. It is called by KMPopHeadersView to update
  // the action of the KMPopHeaders in the map when the action of a treewidget
  // item changes.
  void setAction( QTreeWidgetItem *item, KMPopFilterAction action );

protected slots:
  void slotToggled( bool on );
};

#endif // KMPOPFILTERCNFRMDLG_H
