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

#ifndef KMAIL_SEARCHWINDOW_H
#define KMAIL_SEARCHWINDOW_H

#include "mailcommon/searchpattern.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <kdialog.h>
#include <kxmlguiclient.h>

#include <QtCore/QTimer>

class QCheckBox;
class QCloseEvent;
class QKeyEvent;
class QLabel;
class QRadioButton;
class KActionMenu;
class KJob;
class KLineEdit;
class KMMainWidget;
class KStatusBar;
class KMSearchMessageModel;
class QAbstractItemModel;
class QModelIndex;
namespace Akonadi {
class EntityTreeView;
class ItemModel;
class StandardMailActionManager;
class EntityMimeTypeFilterModel;
}

namespace KMime {
class Message;
}

namespace MailCommon {
class FolderRequester;
class SearchPatternEdit;
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
     *
     * @param parent The parent widget.
     * @param collection The folder which will be pre-selected as the base folder
     *                   of search operations.
     */
    explicit SearchWindow( KMMainWidget* parent, const Akonadi::Collection &collection = Akonadi::Collection() );

    /**
     * Destroys the search window.
     */
    virtual ~SearchWindow();

    /**
     * Changes the base folder for search operations to a different folder.
     *
     * @param folder The folder to use as the new base for searches.
     */
    void activateFolder( const Akonadi::Collection &folder );

    /**
     * Provides access to the list of currently selected message in the listview.
     *
     * @return The list of currently selected search result messages.
     */
    Akonadi::Item::List selectedMessages() const;

    /**
     * Provides access to the currently selected message.
     *
     * @return the currently selected message.
     */
    Akonadi::Item selectedMessage() const;

    /**
     * Loads a search pattern into the search window, replacing the current one.
     */
    void setSearchPattern( const MailCommon::SearchPattern &pattern );

    /**
     * Loads a search pattern into the search window, appending its rules to the current one.
     */
    void addRulesToSearchPattern( const MailCommon::SearchPattern &pattern );

  protected:
    /** Reimplemented to react to Escape. */
    virtual void keyPressEvent( QKeyEvent* );

    /** Reimplemented to stop searching when the window is closed */
    virtual void closeEvent( QCloseEvent* );

    void createSearchModel();
  
    void childCollectionsFromSelectedCollection( const Akonadi::Collection& collection, KUrl::List &list );
    void getChildren( const QAbstractItemModel *model, const QModelIndex &parentIndex, KUrl::List &list );

  
  private Q_SLOTS:
    void updateCollectionStatistic(Akonadi::Collection::Id,Akonadi::CollectionStatistics);

    virtual void slotClose();
    virtual void slotSearch();
    virtual void slotStop();
    void scheduleRename( const QString& );
    void renameSearchFolder();
    void openSearchFolder();
    virtual bool slotShowMsg( const Akonadi::Item& );
    void slotViewSelectedMsg();
    virtual bool slotViewMsg( const Akonadi::Item& );
    void slotCurrentChanged( const Akonadi::Item& );
    virtual void updateContextMenuActions();
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

    /** GUI cleanup after search */
    void searchDone( KJob* );
    void enableGUI();

    void setEnabledSearchButton( bool );
    void slotSearchFolderRenameDone( KJob* );

    void slotContextMenuRequested( const QPoint& );

  private:
    bool mStopped;
    bool mCloseRequested;
    int mSortColumn;
    Qt::SortOrder mSortOrder;
    Akonadi::Collection mFolder;
    KJob *mSearchJob;
    QTimer *mTimer;

    // GC'd by Qt
    QRadioButton *mChkbxAllFolders;
    QRadioButton *mChkbxSpecificFolders;
    MailCommon::FolderRequester *mCbxFolders;
    QCheckBox *mChkSubFolders;
    KMSearchMessageModel *mResultModel;
    Akonadi::EntityTreeView *mLbxMatches;
    QLabel *mSearchFolderLbl;
    KLineEdit *mSearchFolderEdt;
    KPushButton *mSearchFolderOpenBtn;
    KPushButton *mSearchResultOpenBtn;
    KStatusBar* mStatusBar;
    QWidget* mLastFocus; // to remember the position of the focus
    QAction *mReplyAction, *mReplyAllAction, *mReplyListAction, *mSaveAsAction,
      *mForwardInlineAction, *mForwardAttachedAction, *mPrintAction, *mClearAction,
      *mSaveAtchAction;
    KActionMenu *mForwardActionMenu;
    QTimer mRenameTimer;
    QByteArray mHeaderState;
    // not owned by us
    KMMainWidget* mKMMainWidget;
    MailCommon::SearchPatternEdit *mPatternEdit;
    MailCommon::SearchPattern mSearchPattern;

    Akonadi::StandardMailActionManager *mAkonadiStandardAction;

};

}

#endif
