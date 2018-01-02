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

#include "MailCommon/SearchPattern"
#include "ui_searchwindow.h"

#include <AkonadiCore/collection.h>
#include <AkonadiCore/item.h>
#include <QDialog>
#include <kxmlguiclient.h>
#include <KGuiItem>
#include <QTimer>

class QCloseEvent;
class QKeyEvent;
class KActionMenu;
class KJob;
class KMMainWidget;
class KMSearchMessageModel;

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
class SearchWindow : public QDialog, public KXMLGUIClient
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
    explicit SearchWindow(KMMainWidget *parent, const Akonadi::Collection &collection = Akonadi::Collection());

    /**
     * Destroys the search window.
     */
    ~SearchWindow();

    /**
     * Changes the base folder for search operations to a different folder.
     *
     * @param folder The folder to use as the new base for searches.
     */
    void activateFolder(const Akonadi::Collection &folder);

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
    void addRulesToSearchPattern(const MailCommon::SearchPattern &pattern);

protected:
    /** Reimplemented to react to Escape. */
    void keyPressEvent(QKeyEvent *) override;

    /** Reimplemented to stop searching when the window is closed */
    void closeEvent(QCloseEvent *) override;

    void createSearchModel();

private:
    void updateCollectionStatistic(Akonadi::Collection::Id, const Akonadi::CollectionStatistics &);

    void slotClose();
    void slotSearch();
    void slotStop();
    void scheduleRename(const QString &);
    void renameSearchFolder();
    void openSearchFolder();
    void slotViewSelectedMsg();
    void slotViewMsg(const Akonadi::Item &);
    void slotCurrentChanged(const Akonadi::Item &);
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
    void slotCollectionStatisticsRetrieved(KJob *job);
    void searchDone(KJob *);
    void enableGUI();

    void setEnabledSearchButton(bool);
    void slotSearchFolderRenameDone(KJob *);

    void slotContextMenuRequested(const QPoint &);
    void slotSelectMultipleFolders();

    void slotSearchCollectionsFetched(KJob *job);

    void slotJumpToFolder();

private:
    void doSearch();
    QVector<qint64> checkIncompleteIndex(const Akonadi::Collection::List &searchCols, bool recursive);
    Akonadi::Collection::List searchCollectionsRecursive(const Akonadi::Collection::List &cols) const;
    QPointer<PimCommon::SelectMultiCollectionDialog> mSelectMultiCollectionDialog;
    QVector<Akonadi::Collection> mCollectionId;
    Akonadi::SearchQuery mQuery;
    Qt::SortOrder mSortOrder = Qt::AscendingOrder;
    Akonadi::Collection mFolder;

    Ui_SearchWindow mUi;
    KGuiItem mStartSearchGuiItem;
    KGuiItem mStopSearchGuiItem;
    MailCommon::SearchPattern mSearchPattern;

    bool mCloseRequested = false;
    int mSortColumn = 0;

    KJob *mSearchJob = nullptr;
    KMSearchMessageModel *mResultModel = nullptr;
    QPushButton *mSearchButton = nullptr;

    QAction *mReplyAction = nullptr;
    QAction *mReplyAllAction = nullptr;
    QAction *mReplyListAction = nullptr;
    QAction *mSaveAsAction = nullptr;
    QAction *mForwardInlineAction = nullptr;
    QAction *mForwardAttachedAction = nullptr;
    QAction *mPrintAction = nullptr;
    QAction *mClearAction = nullptr;
    QAction *mSaveAtchAction = nullptr;
    QAction *mJumpToFolderAction = nullptr;
    KActionMenu *mForwardActionMenu = nullptr;
    QTimer mRenameTimer;
    QByteArray mHeaderState;
    // not owned by us
    KMMainWidget *mKMMainWidget = nullptr;
    SearchPatternWarning *mSearchPatternWidget = nullptr;

    Akonadi::StandardMailActionManager *mAkonadiStandardAction = nullptr;
};
}

#endif
