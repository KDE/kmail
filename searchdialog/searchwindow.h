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

#include "mailcommon/search/searchpattern.h"
#include "ui_searchwindow.h"

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <kdialog.h>
#include <kxmlguiclient.h>

#include <QtCore/QTimer>

class QCloseEvent;
class QKeyEvent;
class KActionMenu;
class KJob;
class KMMainWidget;
class KMSearchMessageModel;
class QAbstractItemModel;
class QModelIndex;

namespace PimCommon {
class SelectMultiCollectionDialog;
}

namespace Akonadi {
class StandardMailActionManager;
}

namespace KMime {
class Message;
}

namespace KMail {

/**
   * The SearchWindow class provides a dialog for triggering a search on
   * folders and storing that search as a search folder. It shows the search
   * results in a listview and allows triggering of operations such as printing
   * or moving on them.
   */
class SearchPatternWarning;
class SearchWindow: public KDialog, public KXMLGUIClient
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
    ~SearchWindow();

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
     * Loads a search pattern into the search window, appending its rules to the current one.
     */
    void addRulesToSearchPattern( const MailCommon::SearchPattern &pattern );

protected:
    /** Reimplemented to react to Escape. */
    void keyPressEvent( QKeyEvent* );

    /** Reimplemented to stop searching when the window is closed */
    void closeEvent( QCloseEvent* );

    void createSearchModel();

private Q_SLOTS:
    void updateCollectionStatistic(Akonadi::Collection::Id, const Akonadi::CollectionStatistics &);

    void slotClose();
    void slotSearch();
    void slotStop();
    void scheduleRename( const QString& );
    void renameSearchFolder();
    void openSearchFolder();
    void slotViewSelectedMsg();
    void slotViewMsg( const Akonadi::Item& );
    void slotCurrentChanged( const Akonadi::Item& );
    void updateContextMenuActions();
    void slotFolderActivated();
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
    void slotSelectMultipleFolders();

    void slotSearchCollectionsFetched( KJob *job );

private:
    void doSearch();
    QPointer<PimCommon::SelectMultiCollectionDialog> mSelectMultiCollectionDialog;
    QList<Akonadi::Collection> mCollectionId;
    Akonadi::SearchQuery mQuery;
    bool mCloseRequested;
    int mSortColumn;
    Qt::SortOrder mSortOrder;
    Akonadi::Collection mFolder;
    KJob *mSearchJob;

    KMSearchMessageModel *mResultModel;
    Ui_SearchWindow mUi;
    KGuiItem mStartSearchGuiItem;
    KGuiItem mStopSearchGuiItem;
    KPushButton *mSearchButton;

    QAction *mReplyAction, *mReplyAllAction, *mReplyListAction, *mSaveAsAction,
    *mForwardInlineAction, *mForwardAttachedAction, *mPrintAction, *mClearAction,
    *mSaveAtchAction;
    KActionMenu *mForwardActionMenu;
    QTimer mRenameTimer;
    QByteArray mHeaderState;
    // not owned by us
    KMMainWidget* mKMMainWidget;
    MailCommon::SearchPattern mSearchPattern;
    SearchPatternWarning *mSearchPatternWidget;

    Akonadi::StandardMailActionManager *mAkonadiStandardAction;
};

}

#endif
