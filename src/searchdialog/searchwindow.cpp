/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 * Copyright (c) 2001 Aaron J. Seigo <aseigo@kde.org>
 * Copyright (c) 2010 Till Adam <adam@kde.org>
 * Copyright (C) 2011-2018 Laurent Montel <montel@kde.org>
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

#include "searchwindow.h"
#include "incompleteindexdialog.h"

#include "MailCommon/FolderRequester"
#include "kmcommands.h"
#include "kmmainwidget.h"
#include "MailCommon/MailKernel"
#include "MailCommon/SearchPatternEdit"
#include "searchdescriptionattribute.h"
#include "MailCommon/FolderTreeView"
#include "kmsearchmessagemodel.h"
#include "searchpatternwarning.h"
#include "PimCommonAkonadi/SelectMultiCollectionDialog"
#include <PimCommon/PimUtil>

#include <AkonadiCore/CollectionModifyJob>
#include <AkonadiCore/CollectionFetchJob>
#include <AkonadiWidgets/EntityTreeView>
#include <AkonadiCore/persistentsearchattribute.h>
#include <AkonadiCore/SearchCreateJob>
#include <AkonadiCore/ChangeRecorder>
#include <AkonadiWidgets/standardactionmanager.h>
#include <AkonadiCore/EntityMimeTypeFilterModel>
#include <AkonadiCore/CachePolicy>
#include <AkonadiCore/EntityHiddenAttribute>
#include <KActionMenu>
#include "kmail_debug.h"
#include <QIcon>
#include <QSortFilterProxyModel>
#include <KIconLoader>
#include <kmime/kmime_message.h>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <KWindowSystem>
#include <KMessageBox>
#include <AkonadiSearch/PIM/indexeditems.h>

#include <QCheckBox>
#include <QCloseEvent>
#include <QCursor>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>

using namespace KPIM;
using namespace MailCommon;

using namespace KMail;

SearchWindow::SearchWindow(KMMainWidget *widget, const Akonadi::Collection &collection)
    : QDialog(nullptr)
    , mKMMainWidget(widget)
{
    setWindowTitle(i18n("Find Messages"));

    KWindowSystem::setIcons(winId(), qApp->windowIcon().pixmap(IconSize(KIconLoader::Desktop),
                                                               IconSize(KIconLoader::Desktop)),
                            qApp->windowIcon().pixmap(IconSize(KIconLoader::Small),
                                                      IconSize(KIconLoader::Small)));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QWidget *topWidget = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout;
    lay->setMargin(0);
    topWidget->setLayout(lay);
    mSearchPatternWidget = new SearchPatternWarning;
    lay->addWidget(mSearchPatternWidget);
    mainLayout->addWidget(topWidget);

    QWidget *searchWidget = new QWidget(this);
    mUi.setupUi(searchWidget);

    lay->addWidget(searchWidget);

    mStartSearchGuiItem = KGuiItem(i18nc("@action:button Search for messages", "&Search"), QStringLiteral("edit-find"));
    mStopSearchGuiItem = KStandardGuiItem::stop();
    mSearchButton = new QPushButton;
    KGuiItem::assign(mSearchButton, mStartSearchGuiItem);
    mUi.mButtonBox->addButton(mSearchButton, QDialogButtonBox::ActionRole);
    connect(mUi.mButtonBox, &QDialogButtonBox::rejected, this, &SearchWindow::slotClose);
    searchWidget->layout()->setMargin(0);

    mUi.mCbxFolders->setMustBeReadWrite(false);
    mUi.mCbxFolders->setNotAllowToCreateNewFolder(true);
    activateFolder(collection);
    connect(mUi.mPatternEdit, &KMail::KMailSearchPatternEdit::returnPressed, this, &SearchWindow::slotSearch);

    // enable/disable widgets depending on radio buttons:
    connect(mUi.mChkbxAllFolders, &QRadioButton::toggled, this, &SearchWindow::setEnabledSearchButton);

    mUi.mLbxMatches->setXmlGuiClient(mKMMainWidget->guiClient());

    /*
    Default is to sort by date. TODO: Unfortunately this sorts *while*
    inserting, which looks rather strange - the user cannot read
    the results so far as they are constantly re-sorted --dnaber

    Sorting is now disabled when a search is started and reenabled
    when it stops. Items are appended to the list. This not only
    solves the above problem, but speeds searches with many hits
    up considerably. - till

    TODO: subclass QTreeWidgetItem and do proper (and performant)
    compare functions
    */
    mUi.mLbxMatches->setSortingEnabled(true);

    connect(mUi.mLbxMatches, &Akonadi::EntityTreeView::customContextMenuRequested, this, &SearchWindow::slotContextMenuRequested);
    connect(mUi.mLbxMatches, QOverload<const Akonadi::Item &>::of(&Akonadi::EntityTreeView::doubleClicked),
            this, &SearchWindow::slotViewMsg);
    connect(mUi.mLbxMatches, QOverload<const Akonadi::Item &>::of(&Akonadi::EntityTreeView::currentChanged),
            this, &SearchWindow::slotCurrentChanged);
    connect(mUi.selectMultipleFolders, &QPushButton::clicked, this, &SearchWindow::slotSelectMultipleFolders);

    connect(KMKernel::self()->folderCollectionMonitor(), &Akonadi::Monitor::collectionStatisticsChanged, this, &SearchWindow::updateCollectionStatistic);

    connect(mUi.mSearchFolderEdt, &KLineEdit::textChanged, this, &SearchWindow::scheduleRename);
    connect(&mRenameTimer, &QTimer::timeout, this, &SearchWindow::renameSearchFolder);
    connect(mUi.mSearchFolderOpenBtn, &QPushButton::clicked, this, &SearchWindow::openSearchFolder);

    connect(mUi.mSearchResultOpenBtn, &QPushButton::clicked, this, &SearchWindow::slotViewSelectedMsg);

    const int mainWidth = KMailSettings::self()->searchWidgetWidth();
    const int mainHeight = KMailSettings::self()->searchWidgetHeight();

    if (mainWidth || mainHeight) {
        resize(mainWidth, mainHeight);
    }

    connect(mSearchButton, &QPushButton::clicked, this, &SearchWindow::slotSearch);
    connect(this, &SearchWindow::finished, this, &SearchWindow::deleteLater);
    connect(mUi.mButtonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &SearchWindow::slotClose);

    // give focus to the value field of the first search rule
    KLineEdit *r = mUi.mPatternEdit->findChild<KLineEdit *>(QStringLiteral("regExpLineEdit"));
    if (r) {
        r->setFocus();
    } else {
        qCDebug(KMAIL_LOG) << "SearchWindow: regExpLineEdit not found";
    }

    //set up actions
    KActionCollection *ac = actionCollection();
    mReplyAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-reply-sender")), i18n("&Reply..."), this);
    actionCollection()->addAction(QStringLiteral("search_reply"), mReplyAction);
    connect(mReplyAction, &QAction::triggered, this, &SearchWindow::slotReplyToMsg);

    mReplyAllAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-reply-all")), i18n("Reply to &All..."), this);
    actionCollection()->addAction(QStringLiteral("search_reply_all"), mReplyAllAction);
    connect(mReplyAllAction, &QAction::triggered, this, &SearchWindow::slotReplyAllToMsg);

    mReplyListAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-reply-list")), i18n("Reply to Mailing-&List..."), this);
    actionCollection()->addAction(QStringLiteral("search_reply_list"), mReplyListAction);
    connect(mReplyListAction, &QAction::triggered, this, &SearchWindow::slotReplyListToMsg);

    mForwardActionMenu = new KActionMenu(QIcon::fromTheme(QStringLiteral("mail-forward")), i18nc("Message->", "&Forward"), this);
    actionCollection()->addAction(QStringLiteral("search_message_forward"), mForwardActionMenu);
    connect(mForwardActionMenu, &KActionMenu::triggered, this, &SearchWindow::slotForwardMsg);

    mForwardInlineAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-forward")),
                                       i18nc("@action:inmenu Forward message inline.", "&Inline..."),
                                       this);
    actionCollection()->addAction(QStringLiteral("search_message_forward_inline"), mForwardInlineAction);
    connect(mForwardInlineAction, &QAction::triggered, this, &SearchWindow::slotForwardMsg);

    mForwardAttachedAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-forward")), i18nc("Message->Forward->", "As &Attachment..."), this);
    actionCollection()->addAction(QStringLiteral("search_message_forward_as_attachment"), mForwardAttachedAction);
    connect(mForwardAttachedAction, &QAction::triggered, this, &SearchWindow::slotForwardAttachedMsg);

    if (KMailSettings::self()->forwardingInlineByDefault()) {
        mForwardActionMenu->addAction(mForwardInlineAction);
        mForwardActionMenu->addAction(mForwardAttachedAction);
    } else {
        mForwardActionMenu->addAction(mForwardAttachedAction);
        mForwardActionMenu->addAction(mForwardInlineAction);
    }

    mSaveAsAction = actionCollection()->addAction(KStandardAction::SaveAs, QStringLiteral("search_file_save_as"));
    connect(mSaveAsAction, &QAction::triggered, this, &SearchWindow::slotSaveMsg);

    mSaveAtchAction = new QAction(QIcon::fromTheme(QStringLiteral("mail-attachment")), i18n("Save Attachments..."), this);
    actionCollection()->addAction(QStringLiteral("search_save_attachments"), mSaveAtchAction);
    connect(mSaveAtchAction, &QAction::triggered, this, &SearchWindow::slotSaveAttachments);

    mPrintAction = actionCollection()->addAction(KStandardAction::Print, QStringLiteral("search_print"));
    connect(mPrintAction, &QAction::triggered, this, &SearchWindow::slotPrintMsg);

    mClearAction = new QAction(i18n("Clear Selection"), this);
    actionCollection()->addAction(QStringLiteral("search_clear_selection"), mClearAction);
    connect(mClearAction, &QAction::triggered, this, &SearchWindow::slotClearSelection);

    mJumpToFolderAction = new QAction(i18n("Jump to original folder"), this);
    actionCollection()->addAction(QStringLiteral("search_jump_folder"), mJumpToFolderAction);
    connect(mJumpToFolderAction, &QAction::triggered, this, &SearchWindow::slotJumpToFolder);

    connect(mUi.mCbxFolders, &MailCommon::FolderRequester::folderChanged, this, &SearchWindow::slotFolderActivated);

    ac->addAssociatedWidget(this);
    const QList<QAction *> actList = ac->actions();
    for (QAction *action : actList) {
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    }
}

SearchWindow::~SearchWindow()
{
    if (mResultModel) {
        if (mUi.mLbxMatches->columnWidth(0) > 0) {
            KMailSettings::self()->setCollectionWidth(mUi.mLbxMatches->columnWidth(0));
        }
        if (mUi.mLbxMatches->columnWidth(1) > 0) {
            KMailSettings::self()->setSubjectWidth(mUi.mLbxMatches->columnWidth(1));
        }
        if (mUi.mLbxMatches->columnWidth(2) > 0) {
            KMailSettings::self()->setSenderWidth(mUi.mLbxMatches->columnWidth(2));
        }
        if (mUi.mLbxMatches->columnWidth(3) > 0) {
            KMailSettings::self()->setReceiverWidth(mUi.mLbxMatches->columnWidth(3));
        }
        if (mUi.mLbxMatches->columnWidth(4) > 0) {
            KMailSettings::self()->setDateWidth(mUi.mLbxMatches->columnWidth(4));
        }
        if (mUi.mLbxMatches->columnWidth(5) > 0) {
            KMailSettings::self()->setFolderWidth(mUi.mLbxMatches->columnWidth(5));
        }
        KMailSettings::self()->setSearchWidgetWidth(width());
        KMailSettings::self()->setSearchWidgetHeight(height());
        KMailSettings::self()->requestSync();
        mResultModel->deleteLater();
    }
}

void SearchWindow::createSearchModel()
{
    if (mResultModel) {
        mResultModel->deleteLater();
    }
    mResultModel = new KMSearchMessageModel(this);
    mResultModel->setCollection(mFolder);
    QSortFilterProxyModel *sortproxy = new QSortFilterProxyModel(mResultModel);
    sortproxy->setDynamicSortFilter(true);
    sortproxy->setSortRole(Qt::EditRole);
    sortproxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    sortproxy->setSourceModel(mResultModel);
    mUi.mLbxMatches->setModel(sortproxy);

    mUi.mLbxMatches->setColumnWidth(0, KMailSettings::self()->collectionWidth());
    mUi.mLbxMatches->setColumnWidth(1, KMailSettings::self()->subjectWidth());
    mUi.mLbxMatches->setColumnWidth(2, KMailSettings::self()->senderWidth());
    mUi.mLbxMatches->setColumnWidth(3, KMailSettings::self()->receiverWidth());
    mUi.mLbxMatches->setColumnWidth(4, KMailSettings::self()->dateWidth());
    mUi.mLbxMatches->setColumnWidth(5, KMailSettings::self()->folderWidth());
    mUi.mLbxMatches->setColumnHidden(6, true);
    mUi.mLbxMatches->setColumnHidden(7, true);
    mUi.mLbxMatches->header()->setSortIndicator(2, Qt::DescendingOrder);
    mUi.mLbxMatches->header()->setStretchLastSection(false);
    mUi.mLbxMatches->header()->restoreState(mHeaderState);
    //mUi.mLbxMatches->header()->setResizeMode( 3, QHeaderView::Stretch );
    if (!mAkonadiStandardAction) {
        mAkonadiStandardAction = new Akonadi::StandardMailActionManager(actionCollection(), this);
    }
    mAkonadiStandardAction->setItemSelectionModel(mUi.mLbxMatches->selectionModel());
    mAkonadiStandardAction->setCollectionSelectionModel(mKMMainWidget->folderTreeView()->selectionModel());
}

void SearchWindow::setEnabledSearchButton(bool)
{
    //Make sure that button is enable
    //Before when we selected a folder == "Local Folder" as that it was not a folder
    //search button was disable, and when we select "Search in all local folder"
    //Search button was never enabled :(
    mSearchButton->setEnabled(true);
}

void SearchWindow::updateCollectionStatistic(Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &statistic)
{
    QString genMsg;
    if (id == mFolder.id()) {
        genMsg = i18np("%1 match", "%1 matches", statistic.count());
    }
    mUi.mStatusLbl->setText(genMsg);
}

void SearchWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && mSearchJob) {
        slotStop();
        return;
    }

    QDialog::keyPressEvent(event);
}

void SearchWindow::slotFolderActivated()
{
    mUi.mChkbxSpecificFolders->setChecked(true);
}

void SearchWindow::activateFolder(const Akonadi::Collection &collection)
{
    mUi.mChkbxSpecificFolders->setChecked(true);
    mSearchPattern.clear();
    bool currentFolderIsSearchFolder = false;

    if (!collection.hasAttribute<Akonadi::PersistentSearchAttribute>()) {
        // it's not a search folder, make a new search
        mSearchPattern.append(SearchRule::createInstance("Subject"));
        mUi.mCbxFolders->setCollection(collection);
    } else {
        // it's a search folder
        if (collection.hasAttribute<Akonadi::SearchDescriptionAttribute>()) {
            currentFolderIsSearchFolder = true; // FIXME is there a better way to tell?

            const Akonadi::SearchDescriptionAttribute *searchDescription = collection.attribute<Akonadi::SearchDescriptionAttribute>();
            mSearchPattern.deserialize(searchDescription->description());

            const QList<Akonadi::Collection::Id> lst = searchDescription->listCollection();
            if (!lst.isEmpty()) {
                mUi.mChkMultiFolders->setChecked(true);
                mCollectionId.clear();
                for (Akonadi::Collection::Id col : lst) {
                    mCollectionId.append(Akonadi::Collection(col));
                }
            } else {
                const Akonadi::Collection col = searchDescription->baseCollection();
                if (col.isValid()) {
                    mUi.mChkbxSpecificFolders->setChecked(true);
                    mUi.mCbxFolders->setCollection(col);
                    mUi.mChkSubFolders->setChecked(searchDescription->recursive());
                } else {
                    mUi.mChkbxAllFolders->setChecked(true);
                    mUi.mChkSubFolders->setChecked(searchDescription->recursive());
                }
            }
        } else {
            // it's a search folder, but not one of ours, warn the user that we can't edit it
            // FIXME show results, but disable edit GUI
            qCWarning(KMAIL_LOG) << "This search was not created with KMail. It cannot be edited within it.";
            mSearchPattern.clear();
        }
    }

    mUi.mPatternEdit->setSearchPattern(&mSearchPattern);
    if (currentFolderIsSearchFolder) {
        mFolder = collection;
        mUi.mSearchFolderEdt->setText(collection.name());
        createSearchModel();
    } else if (mUi.mSearchFolderEdt->text().isEmpty()) {
        mUi.mSearchFolderEdt->setText(i18n("Last Search"));
        // find last search and reuse it if possible
        mFolder = CommonKernel->collectionFromId(KMailSettings::lastSearchCollectionId());
        // when the last folder got renamed, create a new one
        if (mFolder.isValid() && mFolder.name() != mUi.mSearchFolderEdt->text()) {
            mFolder = Akonadi::Collection();
        }
    }
}

void SearchWindow::slotSearch()
{
    if (mFolder.isValid()) {
        doSearch();
        return;
    }
    //We're going to try to create a new search folder, let's ensure first the name is not yet used.

    //Fetch all search collections
    Akonadi::CollectionFetchJob *fetchJob = new Akonadi::CollectionFetchJob(Akonadi::Collection(1), Akonadi::CollectionFetchJob::FirstLevel);
    connect(fetchJob, &Akonadi::CollectionFetchJob::result, this, &SearchWindow::slotSearchCollectionsFetched);
}

void SearchWindow::slotSearchCollectionsFetched(KJob *job)
{
    if (job->error()) {
        qCWarning(KMAIL_LOG) << job->errorString();
    }
    Akonadi::CollectionFetchJob *fetchJob = static_cast<Akonadi::CollectionFetchJob *>(job);
    const Akonadi::Collection::List lstCol = fetchJob->collections();
    for (const Akonadi::Collection &col : lstCol) {
        if (col.name() == mUi.mSearchFolderEdt->text()) {
            mFolder = col;
        }
    }
    doSearch();
}

void SearchWindow::doSearch()
{
    mSearchPatternWidget->hideWarningPattern();
    if (mUi.mSearchFolderEdt->text().isEmpty()) {
        mUi.mSearchFolderEdt->setText(i18n("Last Search"));
    }

    if (mResultModel) {
        mHeaderState = mUi.mLbxMatches->header()->saveState();
    }

    mUi.mLbxMatches->setModel(nullptr);

    mSortColumn = mUi.mLbxMatches->header()->sortIndicatorSection();
    mSortOrder = mUi.mLbxMatches->header()->sortIndicatorOrder();
    mUi.mLbxMatches->setSortingEnabled(false);

    if (mSearchJob) {
        mSearchJob->kill(KJob::Quietly);
        mSearchJob->deleteLater();
        mSearchJob = nullptr;
    }

    mUi.mSearchFolderEdt->setEnabled(false);

    QVector<Akonadi::Collection> searchCollections;
    bool recursive = false;
    if (mUi.mChkbxSpecificFolders->isChecked()) {
        const Akonadi::Collection col = mUi.mCbxFolders->collection();
        if (!col.isValid()) {
            mSearchPatternWidget->showWarningPattern(QStringList() << i18n("You did not selected a valid folder."));
            mUi.mSearchFolderEdt->setEnabled(true);
            return;
        }
        searchCollections << col;
        if (mUi.mChkSubFolders->isChecked()) {
            recursive = true;
        }
    } else if (mUi.mChkMultiFolders->isChecked()) {
        if (mSelectMultiCollectionDialog) {
            mCollectionId = mSelectMultiCollectionDialog->selectedCollection();
        }
        if (mCollectionId.isEmpty()) {
            mUi.mSearchFolderEdt->setEnabled(true);
            mSearchPatternWidget->showWarningPattern(QStringList() << i18n("You forgot to select collections."));
            mQuery = Akonadi::SearchQuery();
            return;
        }
        searchCollections << mCollectionId;
    }

    mUi.mPatternEdit->updateSearchPattern();

    SearchPattern searchPattern(mSearchPattern);
    searchPattern.purify();

    MailCommon::SearchPattern::SparqlQueryError queryError = searchPattern.asAkonadiQuery(mQuery);
    switch (queryError) {
    case MailCommon::SearchPattern::NoError:
        break;
    case MailCommon::SearchPattern::MissingCheck:
        mUi.mSearchFolderEdt->setEnabled(true);
        mSearchPatternWidget->showWarningPattern(QStringList() << i18n("You forgot to define condition."));
        mQuery = Akonadi::SearchQuery();
        return;
    case MailCommon::SearchPattern::FolderEmptyOrNotIndexed:
        mUi.mSearchFolderEdt->setEnabled(true);
        mSearchPatternWidget->showWarningPattern(QStringList() << i18n("All folders selected are empty or were not indexed."));
        mQuery = Akonadi::SearchQuery();
        return;
    case MailCommon::SearchPattern::EmptyResult:
        mUi.mSearchFolderEdt->setEnabled(true);
        mQuery = Akonadi::SearchQuery();
        mSearchPatternWidget->showWarningPattern(QStringList() << i18n("You forgot to add conditions."));
        return;
    case MailCommon::SearchPattern::NotEnoughCharacters:
        mUi.mSearchFolderEdt->setEnabled(true);
        mSearchPatternWidget->showWarningPattern(QStringList() << i18n("Contains condition cannot be used with a number of characters inferior to 4."));
        mQuery = Akonadi::SearchQuery();
        return;
    }
    mSearchPatternWidget->hideWarningPattern();
    qCDebug(KMAIL_LOG) << mQuery.toJSON();
    mUi.mSearchFolderOpenBtn->setEnabled(true);

    const QVector<qint64> unindexedCollections = checkIncompleteIndex(searchCollections, recursive);
    if (!unindexedCollections.isEmpty()) {
        QScopedPointer<IncompleteIndexDialog> dlg(new IncompleteIndexDialog(unindexedCollections));
        dlg->exec();
    }

    if (!mFolder.isValid()) {
        qCDebug(KMAIL_LOG) << " create new folder " << mUi.mSearchFolderEdt->text();
        Akonadi::SearchCreateJob *searchJob = new Akonadi::SearchCreateJob(mUi.mSearchFolderEdt->text(), mQuery, this);
        searchJob->setSearchMimeTypes(QStringList() << QStringLiteral("message/rfc822"));
        searchJob->setSearchCollections(searchCollections);
        searchJob->setRecursive(recursive);
        searchJob->setRemoteSearchEnabled(false);
        mSearchJob = searchJob;
    } else {
        qCDebug(KMAIL_LOG) << " use existing folder " << mFolder.id();
        Akonadi::PersistentSearchAttribute *attribute = new Akonadi::PersistentSearchAttribute();
        mFolder.setContentMimeTypes(QStringList() << QStringLiteral("message/rfc822"));
        attribute->setQueryString(QString::fromLatin1(mQuery.toJSON()));
        attribute->setQueryCollections(searchCollections);
        attribute->setRecursive(recursive);
        attribute->setRemoteSearchEnabled(false);
        mFolder.addAttribute(attribute);
        mSearchJob = new Akonadi::CollectionModifyJob(mFolder, this);
    }

    connect(mSearchJob, &Akonadi::CollectionModifyJob::result, this, &SearchWindow::searchDone);
    mUi.mProgressIndicator->start();
    enableGUI();
    mUi.mStatusLbl->setText(i18n("Searching..."));
}

void SearchWindow::searchDone(KJob *job)
{
    Q_ASSERT(job == mSearchJob);
    mSearchJob = nullptr;
    QMetaObject::invokeMethod(this, &SearchWindow::enableGUI, Qt::QueuedConnection);

    mUi.mProgressIndicator->stop();
    if (job->error()) {
        qCDebug(KMAIL_LOG) << job->errorString();
        KMessageBox::sorry(this, i18n("Cannot get search result. %1", job->errorString()));
        enableGUI();
        mUi.mSearchFolderEdt->setEnabled(true);
        mUi.mStatusLbl->setText(i18n("Search failed."));
    } else {
        if (const auto *searchJob = qobject_cast<Akonadi::SearchCreateJob *>(job)) {
            mFolder = searchJob->createdCollection();
        } else if (const auto *modifyJob = qobject_cast<Akonadi::CollectionModifyJob *>(job)) {
            mFolder = modifyJob->collection();
        }
        /// TODO: cope better with cases where this fails
        Q_ASSERT(mFolder.isValid());
        Q_ASSERT(mFolder.hasAttribute<Akonadi::PersistentSearchAttribute>());

        KMailSettings::setLastSearchCollectionId(mFolder.id());
        KMailSettings::self()->save();
        KMailSettings::self()->requestSync();

        // store the kmail specific serialization of the search in an attribute on
        // the server, for easy retrieval when editing it again
        const QByteArray search = mSearchPattern.serialize();
        Q_ASSERT(!search.isEmpty());
        Akonadi::SearchDescriptionAttribute *searchDescription = mFolder.attribute<Akonadi::SearchDescriptionAttribute>(Akonadi::Collection::AddIfMissing);
        searchDescription->setDescription(search);
        if (mUi.mChkMultiFolders->isChecked()) {
            searchDescription->setBaseCollection(Akonadi::Collection());
            QList<Akonadi::Collection::Id> lst;
            lst.reserve(mCollectionId.count());
            for (const Akonadi::Collection &col : qAsConst(mCollectionId)) {
                lst << col.id();
            }
            searchDescription->setListCollection(lst);
        } else if (mUi.mChkbxSpecificFolders->isChecked()) {
            const Akonadi::Collection collection = mUi.mCbxFolders->collection();
            searchDescription->setBaseCollection(collection);
        } else {
            searchDescription->setBaseCollection(Akonadi::Collection());
        }
        searchDescription->setRecursive(mUi.mChkSubFolders->isChecked());
        new Akonadi::CollectionModifyJob(mFolder, this);
        Akonadi::CollectionFetchJob *fetch = new Akonadi::CollectionFetchJob(mFolder, Akonadi::CollectionFetchJob::Base, this);
        fetch->fetchScope().setIncludeStatistics(true);
        connect(fetch, &KJob::result, this, &SearchWindow::slotCollectionStatisticsRetrieved);

        mUi.mStatusLbl->setText(i18n("Search complete."));
        createSearchModel();

        if (mCloseRequested) {
            close();
        }

        mUi.mLbxMatches->setSortingEnabled(true);
        mUi.mLbxMatches->header()->setSortIndicator(mSortColumn, mSortOrder);

        mUi.mSearchFolderEdt->setEnabled(true);
    }
}

void SearchWindow::slotCollectionStatisticsRetrieved(KJob *job)
{
    Akonadi::CollectionFetchJob *fetch = qobject_cast<Akonadi::CollectionFetchJob *>(job);
    if (!fetch || fetch->error()) {
        return;
    }

    const Akonadi::Collection::List cols = fetch->collections();
    if (cols.isEmpty()) {
        mUi.mStatusLbl->clear();
        return;
    }

    const Akonadi::Collection col = cols.at(0);
    updateCollectionStatistic(col.id(), col.statistics());
}

void SearchWindow::slotStop()
{
    mUi.mProgressIndicator->stop();
    if (mSearchJob) {
        mSearchJob->kill(KJob::Quietly);
        mSearchJob->deleteLater();
        mSearchJob = nullptr;
        mUi.mStatusLbl->setText(i18n("Search stopped."));
    }

    enableGUI();
}

void SearchWindow::slotClose()
{
    accept();
}

void SearchWindow::closeEvent(QCloseEvent *event)
{
    if (mSearchJob) {
        mCloseRequested = true;
        //Cancel search in progress
        mSearchJob->kill(KJob::Quietly);
        mSearchJob->deleteLater();
        mSearchJob = nullptr;
        QTimer::singleShot(0, this, &SearchWindow::slotClose);
    } else {
        QDialog::closeEvent(event);
    }
}

void SearchWindow::scheduleRename(const QString &text)
{
    if (!text.isEmpty()) {
        mRenameTimer.setSingleShot(true);
        mRenameTimer.start(250);
        mUi.mSearchFolderOpenBtn->setEnabled(false);
    } else {
        mRenameTimer.stop();
        mUi.mSearchFolderOpenBtn->setEnabled(!text.isEmpty());
    }
}

void SearchWindow::renameSearchFolder()
{
    const QString name = mUi.mSearchFolderEdt->text();
    if (mFolder.isValid()) {
        const QString oldFolderName = mFolder.name();
        if (oldFolderName != name) {
            mFolder.setName(name);
            Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob(mFolder, this);
            job->setProperty("oldfoldername", oldFolderName);
            connect(job, &Akonadi::CollectionModifyJob::result, this, &SearchWindow::slotSearchFolderRenameDone);
        }
        mUi.mSearchFolderOpenBtn->setEnabled(true);
    }
}

void SearchWindow::slotSearchFolderRenameDone(KJob *job)
{
    Q_ASSERT(job);
    if (job->error()) {
        qCWarning(KMAIL_LOG) << "Job failed:" << job->errorText();
        KMessageBox::information(this, i18n("There was a problem renaming your search folder. "
                                            "A common reason for this is that another search folder "
                                            "with the same name already exists. Error returned \"%1\".", job->errorText()));
        mUi.mSearchFolderEdt->blockSignals(true);
        mUi.mSearchFolderEdt->setText(job->property("oldfoldername").toString());
        mUi.mSearchFolderEdt->blockSignals(false);
    }
}

void SearchWindow::openSearchFolder()
{
    Q_ASSERT(mFolder.isValid());
    renameSearchFolder();
    mKMMainWidget->slotSelectCollectionFolder(mFolder);
    slotClose();
}

void SearchWindow::slotViewSelectedMsg()
{
    mKMMainWidget->slotMessageActivated(selectedMessage());
}

void SearchWindow::slotViewMsg(const Akonadi::Item &item)
{
    if (item.isValid()) {
        mKMMainWidget->slotMessageActivated(item);
    }
}

void SearchWindow::slotCurrentChanged(const Akonadi::Item &item)
{
    mUi.mSearchResultOpenBtn->setEnabled(item.isValid());
}

void SearchWindow::enableGUI()
{
    const bool searching = (mSearchJob != nullptr);

    KGuiItem::assign(mSearchButton, searching ? mStopSearchGuiItem : mStartSearchGuiItem);
    if (searching) {
        disconnect(mSearchButton, &QPushButton::clicked, this, &SearchWindow::slotSearch);
        connect(mSearchButton, &QPushButton::clicked, this, &SearchWindow::slotStop);
    } else {
        disconnect(mSearchButton, &QPushButton::clicked, this, &SearchWindow::slotStop);
        connect(mSearchButton, &QPushButton::clicked, this, &SearchWindow::slotSearch);
    }
}

Akonadi::Item::List SearchWindow::selectedMessages() const
{
    Akonadi::Item::List messages;

    const QModelIndexList lst = mUi.mLbxMatches->selectionModel()->selectedRows();
    for (const QModelIndex &index : lst) {
        const Akonadi::Item item = index.data(Akonadi::ItemModel::ItemRole).value<Akonadi::Item>();
        if (item.isValid()) {
            messages.append(item);
        }
    }

    return messages;
}

Akonadi::Item SearchWindow::selectedMessage() const
{
    return mUi.mLbxMatches->currentIndex().data(Akonadi::ItemModel::ItemRole).value<Akonadi::Item>();
}

void SearchWindow::updateContextMenuActions()
{
    const int count = selectedMessages().count();
    const bool singleActions = (count == 1);
    const bool notEmpty = (count > 0);

    mJumpToFolderAction->setEnabled(singleActions);

    mReplyAction->setEnabled(singleActions);
    mReplyAllAction->setEnabled(singleActions);
    mReplyListAction->setEnabled(singleActions);
    mPrintAction->setEnabled(singleActions);
    mSaveAtchAction->setEnabled(notEmpty);
    mSaveAsAction->setEnabled(notEmpty);
    mClearAction->setEnabled(notEmpty);
}

void SearchWindow::slotContextMenuRequested(const QPoint &)
{
    if (!selectedMessage().isValid() || selectedMessages().isEmpty()) {
        return;
    }

    updateContextMenuActions();
    QMenu menu(this);

    // show most used actions
    menu.addAction(mReplyAction);
    menu.addAction(mReplyAllAction);
    menu.addAction(mReplyListAction);
    menu.addAction(mForwardActionMenu);
    menu.addSeparator();
    menu.addAction(mJumpToFolderAction);
    menu.addSeparator();
    QAction *act = mAkonadiStandardAction->createAction(Akonadi::StandardActionManager::CopyItems);
    mAkonadiStandardAction->setActionText(Akonadi::StandardActionManager::CopyItems, ki18np("Copy Message", "Copy %1 Messages"));
    menu.addAction(act);
    act = mAkonadiStandardAction->createAction(Akonadi::StandardActionManager::CutItems);
    mAkonadiStandardAction->setActionText(Akonadi::StandardActionManager::CutItems, ki18np("Cut Message", "Cut %1 Messages"));
    menu.addAction(act);
    menu.addAction(mAkonadiStandardAction->createAction(Akonadi::StandardActionManager::CopyItemToMenu));
    menu.addAction(mAkonadiStandardAction->createAction(Akonadi::StandardActionManager::MoveItemToMenu));
    menu.addSeparator();
    menu.addAction(mSaveAsAction);
    menu.addAction(mSaveAtchAction);
    menu.addAction(mPrintAction);
    menu.addSeparator();
    menu.addAction(mClearAction);
    menu.exec(QCursor::pos(), nullptr);
}

void SearchWindow::slotClearSelection()
{
    mUi.mLbxMatches->clearSelection();
}

void SearchWindow::slotReplyToMsg()
{
    KMCommand *command = new KMReplyCommand(this, selectedMessage(), MessageComposer::ReplySmart);
    command->start();
}

void SearchWindow::slotReplyAllToMsg()
{
    KMCommand *command = new KMReplyCommand(this, selectedMessage(), MessageComposer::ReplyAll);
    command->start();
}

void SearchWindow::slotReplyListToMsg()
{
    KMCommand *command = new KMReplyCommand(this, selectedMessage(), MessageComposer::ReplyList);
    command->start();
}

void SearchWindow::slotForwardMsg()
{
    KMCommand *command = new KMForwardCommand(this, selectedMessages());
    command->start();
}

void SearchWindow::slotForwardAttachedMsg()
{
    KMCommand *command = new KMForwardAttachedCommand(this, selectedMessages());
    command->start();
}

void SearchWindow::slotSaveMsg()
{
    KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand(this, selectedMessages());
    saveCommand->start();
}

void SearchWindow::slotSaveAttachments()
{
    KMSaveAttachmentsCommand *saveCommand = new KMSaveAttachmentsCommand(this, selectedMessages());
    saveCommand->start();
}

void SearchWindow::slotPrintMsg()
{
    KMPrintCommandInfo info;
    info.mMsg = selectedMessage();
    KMCommand *command = new KMPrintCommand(this, info);
    command->start();
}

void SearchWindow::addRulesToSearchPattern(const SearchPattern &pattern)
{
    SearchPattern p(mSearchPattern);
    p.purify();

    QList<SearchRule::Ptr>::const_iterator it;
    QList<SearchRule::Ptr>::const_iterator end(pattern.constEnd());
    p.reserve(pattern.count());

    for (it = pattern.constBegin(); it != end; ++it) {
        p.append(SearchRule::createInstance(**it));
    }

    mSearchPattern = p;
    mUi.mPatternEdit->setSearchPattern(&mSearchPattern);
}

void SearchWindow::slotSelectMultipleFolders()
{
    mUi.mChkMultiFolders->setChecked(true);
    if (!mSelectMultiCollectionDialog) {
        QList<Akonadi::Collection::Id> lst;
        lst.reserve(mCollectionId.count());
        for (const Akonadi::Collection &col : qAsConst(mCollectionId)) {
            lst << col.id();
        }
        mSelectMultiCollectionDialog = new PimCommon::SelectMultiCollectionDialog(KMime::Message::mimeType(), lst, this);
    }
    mSelectMultiCollectionDialog->show();
}

void SearchWindow::slotJumpToFolder()
{
    if (selectedMessage().isValid()) {
        mKMMainWidget->slotSelectCollectionFolder(selectedMessage().parentCollection());
    }
}

QVector<qint64> SearchWindow::checkIncompleteIndex(const Akonadi::Collection::List &searchCols, bool recursive)
{
    QVector<qint64> results;
    Akonadi::Collection::List cols;
    if (recursive) {
        cols = searchCollectionsRecursive(searchCols);
    } else {
        for (const Akonadi::Collection &col : searchCols) {
            QAbstractItemModel *etm = KMKernel::self()->collectionModel();
            const QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection(etm, col);
            const Akonadi::Collection modelCol = etm->data(idx, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
            // Only index offline IMAP collections
            if (PimCommon::Util::isImapResource(modelCol.resource()) && !modelCol.cachePolicy().localParts().contains(QLatin1String("RFC822"))) {
                continue;
            } else {
                cols.push_back(modelCol);
            }
        }
    }

    enableGUI();
    mUi.mProgressIndicator->start();
    mUi.mStatusLbl->setText(i18n("Checking index status..."));
    //Fetch collection ?
    for (const Akonadi::Collection &col : qAsConst(cols)) {
        const qlonglong num = KMKernel::self()->indexedItems()->indexedItems((qlonglong)col.id());
        if (col.statistics().count() != num) {
            results.push_back(col.id());
        }
    }
    return results;
}

Akonadi::Collection::List SearchWindow::searchCollectionsRecursive(const Akonadi::Collection::List &cols) const
{
    QAbstractItemModel *etm = KMKernel::self()->collectionModel();
    Akonadi::Collection::List result;

    for (const Akonadi::Collection &col : cols) {
        const QModelIndex colIdx = Akonadi::EntityTreeModel::modelIndexForCollection(etm, col);
        if (col.statistics().count() > -1) {
            if (col.cachePolicy().localParts().contains(QLatin1String("RFC822"))) {
                result.push_back(col);
            }
        } else {
            const Akonadi::Collection collection = etm->data(colIdx, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
            if (!collection.hasAttribute<Akonadi::EntityHiddenAttribute>()
                && collection.cachePolicy().localParts().contains(QLatin1String("RFC822"))) {
                result.push_back(collection);
            }
        }

        const int childrenCount = etm->rowCount(colIdx);
        if (childrenCount > 0) {
            Akonadi::Collection::List subCols;
            subCols.reserve(childrenCount);
            for (int i = 0; i < childrenCount; ++i) {
                const QModelIndex idx = etm->index(i, 0, colIdx);
                const Akonadi::Collection child = etm->data(idx, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
                if (child.cachePolicy().localParts().contains(QLatin1String("RFC822"))) {
                    subCols.push_back(child);
                }
            }

            result += searchCollectionsRecursive(subCols);
        }
    }

    return result;
}
