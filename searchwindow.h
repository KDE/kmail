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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#ifndef searchwindow_h
#define searchwindow_h

#include <QList>
#include <QPointer>
#include <QTimer>
#include <QTreeWidget>

#include <KDialog>
#include <KXMLGUIClient>

class QCheckBox;
class QCloseEvent;
class QKeyEvent;
class QLabel;
class QRadioButton;
class KActionMenu;
class KLineEdit;
class KMFolder;
class KMFolderSearch;
class KMMainWidget;
class KMMessage;
class KMMsgBase;
class KMSearchPattern;
class KMSearchPatternEdit;
class KStatusBar;

namespace KMail {
  class FolderRequester;
  class MatchListView;
}

namespace KMail {

  /**
   * The SearchWindow class provides a dialog for triggering a search on
   * folders and storing that search as a search folder. It shows the search
   * results in a listview and allows triggering of operations such as printing
   * or moving on them.
   */
class SearchWindow: public KDialog, virtual public KXMLGUIClient
{
  Q_OBJECT

public:
  /**
   * Creates a new search window.
   * @param parent The parent widget.
   * @param name The (widget) name of the dialog.
   * @param curFolder The folder which will be pre-selected as the base folder
   * of search operations.
   * @param modal Whether the dialog is to be shown modal.
   */
  explicit SearchWindow( KMMainWidget* parent, KMFolder *curFolder=0 );
  virtual ~SearchWindow();

  /**
   * Changes the base folder for search operations to a different folder.
   * @param curFolder The folder to use as the new base for searches.
   */
  void activateFolder( KMFolder* curFolder );

  /**
   * Provides access to the list of currently selected message in the listview.
   * @return The list of currently selected search result messages.
   */
  QList<KMMsgBase*> selectedMessages();

  /**
   * Provides access to the currently selected message.
   * @return the currently selected message.
   */
  KMMessage* message();

  void setSearchPattern( const KMSearchPattern &pattern );

protected slots:
  /** Update status line widget. */
  virtual void updStatus(void);

  virtual void slotClose();
  virtual void slotSearch();
  virtual void slotStop();
  void scheduleRename(const QString &);
  void renameSearchFolder();
  void openSearchFolder();
  void folderInvalidated(KMFolder *);
  virtual bool slotShowMsg(QTreeWidgetItem *,int);
  void slotViewSelectedMsg();
  virtual bool slotViewMsg( QTreeWidgetItem *, int );
  void slotCurrentChanged(QTreeWidgetItem *);
  virtual void updateContextMenuActions();
  virtual void slotContextMenuRequested( QTreeWidgetItem* );
  void slotCopySelectedMessagesToFolder( QAction* );
  void slotMoveSelectedMessagesToFolder( QAction* );
  virtual void slotFolderActivated();
  void slotClearSelection();
  void slotReplyToMsg();
  void slotReplyAllToMsg();
  void slotReplyListToMsg();
  void slotForwardMsg();
  void slotForwardAttachedMsg();
  void slotSaveMsg();
  void slotSaveAttachments();
  void slotPrintMsg();
  void slotCopyMsgs();
  void slotCutMsgs();

  /** GUI cleanup after search */
  virtual void searchDone();
  virtual void slotAddMsg(int idx);
  virtual void slotRemoveMsg(KMFolder *, quint32 serNum);
  void enableGUI();

  void setEnabledSearchButton(bool);

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
  Qt::SortOrder mSortOrder;
  QPointer<KMFolderSearch> mFolder;
  QTimer *mTimer;

  // GC'd by Qt
  QRadioButton *mChkbxAllFolders;
  QRadioButton *mChkbxSpecificFolders;
  KMail::FolderRequester *mCbxFolders;
  QCheckBox *mChkSubFolders;
  MatchListView* mLbxMatches;
  QLabel *mSearchFolderLbl;
  KLineEdit *mSearchFolderEdt;
  KPushButton *mSearchFolderOpenBtn;
  KPushButton *mSearchResultOpenBtn;
  KStatusBar* mStatusBar;
  QWidget* mLastFocus; // to remember the position of the focus
  QAction *mReplyAction, *mReplyAllAction, *mReplyListAction, *mSaveAsAction,
    *mForwardInlineAction, *mForwardAttachedAction, *mPrintAction, *mClearAction,
    *mSaveAtchAction, *mCopyAction, *mCutAction;
  KActionMenu *mForwardActionMenu;
  QList<QPointer<KMFolder> > mFolders;
  QTimer mRenameTimer;

  // not owned by us
  KMMainWidget* mKMMainWidget;
  KMSearchPatternEdit *mPatternEdit;
  KMSearchPattern *mSearchPattern;

  static const int MSGID_COLUMN;

private:
  KMMessage *indexToMessage( QTreeWidgetItem *item );

};

// QTreeWidget sub-class for dnd support
// Internal, only used by SearchWindow.
class MatchListView : public QTreeWidget
{
  Q_OBJECT

  public:
    MatchListView( QWidget *parent, SearchWindow* sw );

  protected:

    virtual void contextMenuEvent( QContextMenuEvent* event );
    virtual void startDrag( Qt::DropActions supportedActions );

  private:
    SearchWindow* mSearchWindow;
  signals:
    void contextMenuRequested( QTreeWidgetItem* item );
};

} // namespace KMail
#endif /*searchwindow_h*/
