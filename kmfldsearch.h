/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 * Copyright (c) 2001 Aaron J. Seigo <aseigo@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#ifndef kmfldsearch_h
#define kmfldsearch_h

#include <qvaluelist.h>
#include <qptrlist.h>
#include <qstringlist.h>
#include <qguardedptr.h>

#include <kdialogbase.h>
#include <kxmlguiclient.h>
#include <mimelib/string.h>

class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class KListView;
class QListViewItem;
class QPushButton;
class QRadioButton;
class KAction;
class KActionMenu;
class KMFolder;
class KMFolderSearch;
class KMFolderImap;
class KMFolderMgr;
class KMMainWidget;
class KMMessage;
class KMSearchPattern;
class KMSearchPatternEdit;
class KStatusBar;
class DwBoyerMoore;
namespace KMail {
  class FolderRequester;
}

typedef QPtrList<KMMsgBase> KMMessageList;

class KMFldSearch: public KDialogBase, virtual public KXMLGUIClient
{
  Q_OBJECT

public:
  KMFldSearch(KMMainWidget* parent, const char* name=0,
              KMFolder *curFolder=0, bool modal=FALSE);
  virtual ~KMFldSearch();

  void activateFolder(KMFolder* curFolder);
  KMMessageList selectedMessages();
  KMMessage* message();

protected slots:
  /** Update status line widget. */
  virtual void updStatus(void);

  virtual void slotClose();
  virtual void slotSearch();
  virtual void slotStop();
  void updateCreateButton( const QString &);
  void renameSearchFolder();
  void openSearchFolder();
  void folderInvalidated(KMFolder *);
  virtual bool slotShowMsg(QListViewItem *);
  virtual void updateContextMenuActions();
  virtual void slotContextMenuRequested( QListViewItem*, const QPoint &, int );
  virtual void copySelectedToFolder( int menuId );
  virtual void moveSelectedToFolder( int menuId );
  virtual void slotFolderActivated( KMFolder* );
  void slotClearSelection();
  void slotReplyToMsg();
  void slotReplyAllToMsg();
  void slotReplyListToMsg();
  void slotForwardMsg();
  void slotForwardAttachedMsg();
  void slotSaveMsg();
  void slotSaveAttachments();
  void slotPrintMsg();

  /** GUI cleanup after search */
  virtual void searchDone();
  virtual void slotAddMsg(int idx);
  virtual void slotRemoveMsg(KMFolder *, Q_UINT32 serNum);
  void enableGUI();

protected:

  /** Reimplemented to react to Escape. */
  virtual void keyPressEvent(QKeyEvent*);

  /** Reimplemented to stop searching when the window is closed */
  virtual void closeEvent(QCloseEvent*);

protected:
  bool mStopped;
  bool mCloseRequested;
  int mFetchingInProgress;
  int mSortColumn;
  SortOrder mSortOrder;
  QGuardedPtr<KMFolderSearch> mFolder;
  QTimer *mTimer;

  // GC'd by Qt
  QRadioButton *mChkbxAllFolders;
  QRadioButton *mChkbxSpecificFolders;
  KMail::FolderRequester *mCbxFolders;
  QPushButton *mBtnSearch;
  QPushButton *mBtnStop;
  QCheckBox *mChkSubFolders;
  KListView* mLbxMatches;
  QLabel *mSearchFolderLbl;
  QLineEdit *mSearchFolderEdt;
  QPushButton *mSearchFolderBtn;
  QPushButton *mSearchFolderOpenBtn;
  KStatusBar* mStatusBar;
  QWidget* mLastFocus; // to remember the position of the focus
  QMap<int,KMFolder*> mMenuToFolder;
  KAction *mReplyAction, *mReplyAllAction, *mReplyListAction, *mSaveAsAction,
    *mForwardAction, *mForwardAttachedAction, *mPrintAction, *mClearAction,
    *mSaveAtchAction;
  KActionMenu *mForwardActionMenu;
  QValueList<QGuardedPtr<KMFolder> > mFolders;

  // not owned by us
  KMMainWidget* mKMMainWidget;
  KMSearchPatternEdit *mPatternEdit;
  KMSearchPattern *mSearchPattern;

  static const int MSGID_COLUMN;
};
#endif /*kmfldsearch_h*/
