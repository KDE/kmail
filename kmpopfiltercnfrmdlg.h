/*
  Copyright (c) 2001 Heiko Hund <heiko@ist.eigentlich.net>
  Copyright (c) 2001 Thorsten Zachmann <T.Zachmann@zagge.de>

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
#include <k3listview.h>

#include <QMap>
#include <QKeyEvent>
#include <QList>

class QWidget;
class QString;

class KMPopFilterCnfrmDlg;
/**
  * @author Heiko Hund
  */
class KMPopHeadersView : public K3ListView
{
  Q_OBJECT

public:
  explicit KMPopHeadersView(QWidget *aParent=0, KMPopFilterCnfrmDlg *aDialog=0);
  ~KMPopHeadersView();
  static const KMPopFilterAction mapToAction(int aColumn) { return (KMPopFilterAction)aColumn;}
  static const int mapToColumn(KMPopFilterAction aAction) { return (int)aAction;}
  static const char *mUnchecked[26];
  static const char *mChecked[26];
protected:
  static const char *mLater[25];
  static const char *mDown[20];
  static const char *mDel[19];
  void keyPressEvent( QKeyEvent *k);

protected slots: // Protected slots
  void slotPressed(Q3ListViewItem* aItem, const QPoint& aPoint, int aColumn);

private:
  KMPopFilterCnfrmDlg *mDialog;
};



class KMPopHeadersViewItem : public K3ListViewItem
{
public:
  KMPopHeadersViewItem(KMPopHeadersView *aParent, KMPopFilterAction aAction);
  ~KMPopHeadersViewItem();
  void setAction(KMPopFilterAction aAction);
  KMPopFilterAction action() { return mAction; }
  virtual void paintFocus(QPainter *, const QColorGroup & cg, const QRect &r);
  virtual QString key(int col, bool ascending) const;
protected:
  KMPopHeadersView *mParent;
  KMPopFilterAction mAction;
};


class KMPopFilterCnfrmDlg : public KDialog
{
	friend class ::KMPopHeadersView;
  Q_OBJECT
protected:
  KMPopFilterCnfrmDlg() { }
  QMap<Q3ListViewItem*, KMPopHeaders*> mItemMap;
  QList<KMPopHeadersViewItem *> mDelList;
  QList<KMPopHeaders *> mDDLList;
  KMPopHeadersView *mFilteredHeaders;
  bool mLowerBoxVisible;
  bool mShowLaterMsgs;
  void setupLVI(KMPopHeadersViewItem *lvi, KMMessage *msg);


public:
  KMPopFilterCnfrmDlg( const QList<KMPopHeaders *> & aHeaders, const QString & aAccount,
                       bool aShowLaterMsgs = false, QWidget *aParent = 0 );
  ~KMPopFilterCnfrmDlg();

public:
  void setAction(Q3ListViewItem *aItem, KMPopFilterAction aAction);

protected slots: // Protected slots
  /**
    This Slot is called whenever a ListView item was pressed.
    It checks for the column the button was pressed in and changes the action if the
    click happened over a radio button column.
    Of course the radio button state is changed as well if the above is true.
*/
  void slotPressed(Q3ListViewItem *aItem, const QPoint &aPnt, int aColumn);
  void slotToggled(bool aOn);
  void slotUpdateMinimumSize();
};

#endif // KMPOPFILTERCNFRMDLG_H
