/***************************************************************************
                          kmpopheadersdlg.h  -  description
                             -------------------
    begin                : Sat Nov 3 2001
    copyright            : (C) 2001 by Heiko Hund
    email                : heiko@ist.eigentlich.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KMPOPHEADERSDLG_H
#define KMPOPHEADERSDLG_H

#include "kmpopheaders.h"

#include <kdialogbase.h>
#include <klistview.h>

#include <qwidget.h>
#include <qptrlist.h>
#include <qmap.h>
#include <qvgroupbox.h>
#include <qstring.h>
/**
  * @author Heiko Hund
  */
class KMPopHeadersView : public KListView
{
  Q_OBJECT

public:
  KMPopHeadersView(QWidget *aParent = 0);
  ~KMPopHeadersView();
  int mapToColumn(KMPopFilterAction aAction);
  KMPopFilterAction mapToAction(int aColumn);
  const static char *mUnchecked[26];
  const static char *mChecked[26];
protected:
  const static char *mLater[25];
  const static char *mDown[20];
  const static char *mDel[19];
  QMap<KMPopFilterAction, int> mColumnOf;
  QMap<int, KMPopFilterAction> mActionAt;

protected slots: // Protected slots
  void slotPressed(QListViewItem* aItem, const QPoint& aPoint, int aColumn);
  void slotIndexChanged(int aSection, int aFromIndex, int aToIndex);
};



class KMPopHeadersViewItem : public KListViewItem
{
public:
  KMPopHeadersViewItem(KMPopHeadersView *aParent, KMPopFilterAction aAction);
  ~KMPopHeadersViewItem();
  void check(KMPopFilterAction aAction);
  virtual void paintFocus(QPainter *, const QColorGroup & cg, const QRect &r);
	virtual int compare(QListViewItem * i, int col, bool ascending);
protected:
  KMPopHeadersView *mParent;
  QMap<KMPopFilterAction, bool> mChecked;
};


class KMPopFilterCnfrmDlg : public KDialogBase
{
  Q_OBJECT
protected:
  KMPopFilterCnfrmDlg() { };
  QPtrList<KMPopHeaders> *mHeaders;
  QMap<QListViewItem*, KMPopHeaders*> mItemMap;
  QPtrList<KMPopHeadersViewItem> mDelList;
  QPtrList<KMPopHeaders> mDDLList;
  KMPopHeadersView *mFilteredHeaders;
  bool mLowerBoxVisible;
  bool mShowLaterMsgs;

public:
  KMPopFilterCnfrmDlg(QPtrList<KMPopHeaders> *aHeaders, const QString &aAccount, bool aShowLaterMsgs = false, QWidget *aParent=0, const char *aName=0);
  ~KMPopFilterCnfrmDlg();

protected slots: // Protected slots
  /**
    This Slot is called whenever a ListView item was pressed.
    It checks for the column the button was pressed in and changes the action if the
    click happened over a radio button column.
    Of course the radio button state is changed as well if the above is true.
*/
  void slotPressed(QListViewItem *aItem, const QPoint &aPnt, int aColumn);
  void slotToggled(bool aOn);
  void slotUpdateMinimumSize();
};

#endif
