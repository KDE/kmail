/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 * Copyright (c) 2001 Aaron J. Seigo <aseigo@kde.org>
 * Copyright (c) 2010 Till Adam <adam@kde.org>
 * Copyright (c) 2011, 2012 Laurent Montel <montel@kde.org>
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

#include "folderrequester.h"
#if !defined(NDEBUG)
#include "debug/searchdebugdialog.h"
#endif
#include "kmcommands.h"
#include "kmmainwidget.h"
#include "mailcommon/kernel/mailkernel.h"
#include "mailcommon/search/searchpatternedit.h"
#include "mailcommon/widgets/regexplineedit.h"
#include "searchdescriptionattribute.h"
#include "foldertreeview.h"
#include "kmsearchmessagemodel.h"
#include "kmsearchfilterproxymodel.h"
#include "searchpatternwarning.h"
#include "mailcommon/folderdialog/selectmulticollectiondialog.h"

#include <Akonadi/CollectionModifyJob>
#include <Akonadi/EntityTreeView>
#include <akonadi/persistentsearchattribute.h>
#include <Akonadi/SearchCreateJob>
#include <Akonadi/ChangeRecorder>
#include <akonadi/standardactionmanager.h>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <KActionMenu>
#include <KDebug>
#include <KDialogButtonBox>
#include <KIcon>
#include <KIconLoader>
#include <KLineEdit>
#include <kmime/kmime_message.h>
#include <KStandardAction>
#include <KStandardGuiItem>
#include <KWindowSystem>
#include <KMessageBox>

#include <QCheckBox>
#include <QCloseEvent>
#include <QCursor>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QRadioButton>
#include <QVBoxLayout>

using namespace KPIM;
using namespace MailCommon;

namespace KMail {

SearchWindow::SearchWindow( KMMainWidget *widget, const Akonadi::Collection &collection )
    : KDialog( 0 ),
      mCloseRequested( false ),
      mSortColumn( 0 ),
      mSortOrder( Qt::AscendingOrder ),
      mSearchJob( 0 ),
      mResultModel( 0 ),
      mKMMainWidget( widget ),
      mAkonadiStandardAction( 0 )
{
    setCaption( i18n( "Find Messages" ) );

    KWindowSystem::setIcons( winId(), qApp->windowIcon().pixmap( IconSize( KIconLoader::Desktop ),
                                                                 IconSize( KIconLoader::Desktop ) ),
                             qApp->windowIcon().pixmap( IconSize( KIconLoader::Small ),
                                                        IconSize( KIconLoader::Small ) ) );

    QWidget *topWidget = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout;
    lay->setMargin(0);
    topWidget->setLayout(lay);
    mSearchPatternWidget = new SearchPatternWarning;
    lay->addWidget(mSearchPatternWidget);
    setMainWidget( topWidget );

    QWidget *searchWidget = new QWidget( this );
    mUi.setupUi( searchWidget );

    lay->addWidget(searchWidget);


    setButtons( None );
    mStartSearchGuiItem = KGuiItem( i18nc( "@action:button Search for messages", "&Search" ), QLatin1String("edit-find") );
    mStopSearchGuiItem = KStandardGuiItem::stop();
    mSearchButton =  mUi.mButtonBox->addButton( mStartSearchGuiItem, QDialogButtonBox::ActionRole );
    connect( mUi.mButtonBox, SIGNAL(rejected()), SLOT(slotClose()) );
#if !defined(NDEBUG)
    QPushButton *debugButton = mUi.mButtonBox->addButton( i18n("Debug query"), QDialogButtonBox::ActionRole );
    connect(debugButton, SIGNAL(clicked(bool)), SLOT(slotDebugQuery()));
#endif
    searchWidget->layout()->setMargin( 0 );

    mUi.mCbxFolders->setMustBeReadWrite( false );
    mUi.mCbxFolders->setNotAllowToCreateNewFolder( true );

    bool currentFolderIsSearchFolder = false;

    if ( !collection.hasAttribute<Akonadi::PersistentSearchAttribute>() ) {
        // it's not a search folder, make a new search
        mSearchPattern.append( SearchRule::createInstance( "Subject" ) );
        mUi.mCbxFolders->setCollection( collection );
    } else {
        // it's a search folder
        if ( collection.hasAttribute<Akonadi::SearchDescriptionAttribute>() ) {
            currentFolderIsSearchFolder = true; // FIXME is there a better way to tell?

            const Akonadi::SearchDescriptionAttribute* searchDescription = collection.attribute<Akonadi::SearchDescriptionAttribute>();
            mSearchPattern.deserialize( searchDescription->description() );

            const QList<Akonadi::Collection::Id> lst = searchDescription->listCollection();
            if (!lst.isEmpty()) {
                mUi.mChkMultiFolders->setChecked(true);
                mCollectionId.clear();
                Q_FOREACH (Akonadi::Collection::Id col, lst) {
                    mCollectionId.append(Akonadi::Collection(col));
                }
            } else {
                const Akonadi::Collection col = searchDescription->baseCollection();
                if ( col.isValid() ) {
                    mUi.mChkbxSpecificFolders->setChecked( true );
                    mUi.mCbxFolders->setCollection( col );
                    mUi.mChkSubFolders->setChecked( searchDescription->recursive() );
                } else {
                    mUi.mChkbxAllFolders->setChecked( true );
                    mUi.mChkSubFolders->setChecked( searchDescription->recursive() );
                }
            }
        } else {
            // it's a search folder, but not one of ours, warn the user that we can't edit it
            // FIXME show results, but disable edit GUI
            kWarning() << "This search was not created with KMail. It cannot be edited within it.";
            mSearchPattern.clear();
        }
    }

    mUi.mPatternEdit->setSearchPattern( &mSearchPattern );
    connect( mUi.mPatternEdit, SIGNAL(returnPressed()),
             this, SLOT(slotSearch()) );

    // enable/disable widgets depending on radio buttons:
    connect( mUi.mChkbxAllFolders, SIGNAL(toggled(bool)),
             this, SLOT(setEnabledSearchButton(bool)) );

    mUi.mLbxMatches->setXmlGuiClient( mKMMainWidget->guiClient() );

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
    mUi.mLbxMatches->setSortingEnabled( true );

    connect( mUi.mLbxMatches, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(slotContextMenuRequested(QPoint)) );
    connect( mUi.mLbxMatches, SIGNAL(clicked(Akonadi::Item)),
             this, SLOT(slotShowMsg(Akonadi::Item)) );
    connect( mUi.mLbxMatches, SIGNAL(doubleClicked(Akonadi::Item)),
             this, SLOT(slotViewMsg(Akonadi::Item)) );
    connect( mUi.mLbxMatches, SIGNAL(currentChanged(Akonadi::Item)),
             this, SLOT(slotCurrentChanged(Akonadi::Item)) );
    connect( mUi.selectMultipleFolders, SIGNAL(clicked()),
             this, SLOT(slotSelectMultipleFolders()));

    connect( KMKernel::self()->folderCollectionMonitor(), SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)), this, SLOT(updateCollectionStatistic(Akonadi::Collection::Id,Akonadi::CollectionStatistics)) );

    if ( currentFolderIsSearchFolder ) {
        mFolder = collection;
        mUi.mSearchFolderEdt->setText( collection.name() );
        Q_ASSERT ( !mResultModel );
        createSearchModel();
    } else {
        mUi.mSearchFolderEdt->setText( i18n( "Last Search" ) );
        // find last search and reuse it if possible
        mFolder = CommonKernel->collectionFromId( GlobalSettings::lastSearchCollectionId() );
        // when the last folder got renamed, create a new one
        if ( mFolder.isValid() && mFolder.name() != mUi.mSearchFolderEdt->text() ) {
            mFolder = Akonadi::Collection();
        }
    }

    connect( mUi.mSearchFolderEdt, SIGNAL(textChanged(QString)),
             this, SLOT(scheduleRename(QString)) );
    connect( &mRenameTimer, SIGNAL(timeout()),
             this, SLOT(renameSearchFolder()) );
    connect( mUi.mSearchFolderOpenBtn, SIGNAL(clicked()),
             this, SLOT(openSearchFolder()) );

    connect( mUi.mSearchResultOpenBtn, SIGNAL(clicked()),
             this, SLOT(slotViewSelectedMsg()) );

    const int mainWidth = GlobalSettings::self()->searchWidgetWidth();
    const int mainHeight = GlobalSettings::self()->searchWidgetHeight();

    if ( mainWidth || mainHeight )
        resize( mainWidth, mainHeight );

    connect( mSearchButton, SIGNAL(clicked()), SLOT(slotSearch()) );
    connect( this, SIGNAL(finished()), this, SLOT(deleteLater()) );
    connect( this, SIGNAL(closeClicked()),this,SLOT(slotClose()) );

    // give focus to the value field of the first search rule
    RegExpLineEdit* r = mUi.mPatternEdit->findChild<RegExpLineEdit*>( QLatin1String("regExpLineEdit") );
    if ( r )
        r->setFocus();
    else
        kDebug() << "SearchWindow: regExpLineEdit not found";

    //set up actions
    KActionCollection *ac = actionCollection();
    mReplyAction = new KAction( KIcon( QLatin1String("mail-reply-sender") ), i18n( "&Reply..." ), this );
    actionCollection()->addAction( QLatin1String("search_reply"), mReplyAction );
    connect( mReplyAction, SIGNAL(triggered(bool)), SLOT(slotReplyToMsg()) );

    mReplyAllAction = new KAction( KIcon( QLatin1String("mail-reply-all") ), i18n( "Reply to &All..." ), this );
    actionCollection()->addAction( QLatin1String("search_reply_all"), mReplyAllAction );
    connect( mReplyAllAction, SIGNAL(triggered(bool)), SLOT(slotReplyAllToMsg()) );

    mReplyListAction = new KAction( KIcon( QLatin1String("mail-reply-list") ), i18n( "Reply to Mailing-&List..." ), this );
    actionCollection()->addAction(QLatin1String( "search_reply_list"), mReplyListAction );
    connect( mReplyListAction, SIGNAL(triggered(bool)), SLOT(slotReplyListToMsg()) );

    mForwardActionMenu = new KActionMenu( KIcon( QLatin1String("mail-forward") ), i18nc( "Message->", "&Forward" ), this );
    actionCollection()->addAction( QLatin1String("search_message_forward"), mForwardActionMenu );
    connect( mForwardActionMenu, SIGNAL(triggered(bool)), this, SLOT(slotForwardMsg()) );

    mForwardInlineAction = new KAction( KIcon( QLatin1String("mail-forward") ),
                                        i18nc( "@action:inmenu Forward message inline.", "&Inline..." ),
                                        this );
    actionCollection()->addAction( QLatin1String("search_message_forward_inline"), mForwardInlineAction );
    connect( mForwardInlineAction, SIGNAL(triggered(bool)), SLOT(slotForwardMsg()) );

    mForwardAttachedAction = new KAction( KIcon( QLatin1String("mail-forward") ), i18nc( "Message->Forward->", "As &Attachment..." ), this );
    actionCollection()->addAction( QLatin1String("search_message_forward_as_attachment"), mForwardAttachedAction );
    connect( mForwardAttachedAction, SIGNAL(triggered(bool)), SLOT(slotForwardAttachedMsg()) );

    if ( GlobalSettings::self()->forwardingInlineByDefault() ) {
        mForwardActionMenu->addAction( mForwardInlineAction );
        mForwardActionMenu->addAction( mForwardAttachedAction );
    } else {
        mForwardActionMenu->addAction( mForwardAttachedAction );
        mForwardActionMenu->addAction( mForwardInlineAction );
    }

    mSaveAsAction = actionCollection()->addAction( KStandardAction::SaveAs, QLatin1String("search_file_save_as"), this, SLOT(slotSaveMsg()) );

    mSaveAtchAction = new KAction( KIcon( QLatin1String("mail-attachment") ), i18n( "Save Attachments..." ), this );
    actionCollection()->addAction( QLatin1String("search_save_attachments"), mSaveAtchAction );
    connect( mSaveAtchAction, SIGNAL(triggered(bool)), SLOT(slotSaveAttachments()) );

    mPrintAction = actionCollection()->addAction( KStandardAction::Print, QLatin1String("search_print"), this, SLOT(slotPrintMsg()) );

    mClearAction = new KAction( i18n( "Clear Selection" ), this );
    actionCollection()->addAction( QLatin1String("search_clear_selection"), mClearAction );
    connect( mClearAction, SIGNAL(triggered(bool)), SLOT(slotClearSelection()) );

    connect( mUi.mCbxFolders, SIGNAL(folderChanged(Akonadi::Collection)),
             this, SLOT(slotFolderActivated()) );

    ac->addAssociatedWidget( this );
    foreach ( QAction* action, ac->actions() )
        action->setShortcutContext( Qt::WidgetWithChildrenShortcut );
}

SearchWindow::~SearchWindow()
{
    if ( mResultModel ) {
        if ( mUi.mLbxMatches->columnWidth( 0 ) > 0 ) {
            GlobalSettings::self()->setCollectionWidth( mUi.mLbxMatches->columnWidth( 0 )  );
        }
        if ( mUi.mLbxMatches->columnWidth( 1 ) > 0 ) {
            GlobalSettings::self()->setSubjectWidth( mUi.mLbxMatches->columnWidth( 1 )  );
        }
        if ( mUi.mLbxMatches->columnWidth( 2 ) > 0 ) {
            GlobalSettings::self()->setSenderWidth( mUi.mLbxMatches->columnWidth( 2 ) );
        }
        if ( mUi.mLbxMatches->columnWidth( 3 ) > 0 ) {
            GlobalSettings::self()->setReceiverWidth( mUi.mLbxMatches->columnWidth( 3 ) );
        }
        if ( mUi.mLbxMatches->columnWidth( 4 ) > 0 ) {
            GlobalSettings::self()->setDateWidth( mUi.mLbxMatches->columnWidth( 4 ) );
        }
        if ( mUi.mLbxMatches->columnWidth( 5 ) > 0 ) {
            GlobalSettings::self()->setFolderWidth( mUi.mLbxMatches->columnWidth( 5 ) );
        }
        GlobalSettings::self()->setSearchWidgetWidth( width() );
        GlobalSettings::self()->setSearchWidgetHeight( height() );
        GlobalSettings::self()->requestSync();
        mResultModel->deleteLater();
    }
}

void SearchWindow::createSearchModel()
{
    if ( mResultModel ) {
        mResultModel->deleteLater();
    }
    mResultModel = new KMSearchMessageModel( this );
    mResultModel->setCollection( mFolder );
    KMSearchFilterProxyModel *sortproxy = new KMSearchFilterProxyModel( mResultModel );
    sortproxy->setSourceModel( mResultModel );
    mUi.mLbxMatches->setModel( sortproxy );

    mUi.mLbxMatches->setColumnWidth( 0, GlobalSettings::self()->collectionWidth() );
    mUi.mLbxMatches->setColumnWidth( 1, GlobalSettings::self()->subjectWidth() );
    mUi.mLbxMatches->setColumnWidth( 2, GlobalSettings::self()->senderWidth() );
    mUi.mLbxMatches->setColumnWidth( 3, GlobalSettings::self()->receiverWidth() );
    mUi.mLbxMatches->setColumnWidth( 4, GlobalSettings::self()->dateWidth() );
    mUi.mLbxMatches->setColumnWidth( 5, GlobalSettings::self()->folderWidth() );
    mUi.mLbxMatches->setColumnHidden( 6, true );
    mUi.mLbxMatches->setColumnHidden( 7, true );
    mUi.mLbxMatches->header()->setSortIndicator( 2, Qt::DescendingOrder );
    mUi.mLbxMatches->header()->setStretchLastSection( false );
    mUi.mLbxMatches->header()->restoreState( mHeaderState );
    //mUi.mLbxMatches->header()->setResizeMode( 3, QHeaderView::Stretch );
    if(!mAkonadiStandardAction)
        mAkonadiStandardAction = new Akonadi::StandardMailActionManager( actionCollection(), this );
    mAkonadiStandardAction->setItemSelectionModel( mUi.mLbxMatches->selectionModel() );
    mAkonadiStandardAction->setCollectionSelectionModel( mKMMainWidget->folderTreeView()->selectionModel() );

}


void SearchWindow::setEnabledSearchButton( bool )
{
    //Make sure that button is enable
    //Before when we selected a folder == "Local Folder" as that it was not a folder
    //search button was disable, and when we select "Search in all local folder"
    //Search button was never enabled :(
    mSearchButton->setEnabled( true );
}

void SearchWindow::updateCollectionStatistic(Akonadi::Collection::Id id,Akonadi::CollectionStatistics statistic)
{
    QString genMsg;
    if ( id == mFolder.id() ) {
        genMsg = i18np( "%1 match", "%1 matches", statistic.count() );
    }
    mUi.mStatusLbl->setText( genMsg );
}

void SearchWindow::keyPressEvent( QKeyEvent *event )
{
    if ( event->key() == Qt::Key_Escape && mSearchJob ) {
        slotStop();
        return;
    }

    KDialog::keyPressEvent( event );
}

void SearchWindow::slotFolderActivated()
{
    mUi.mChkbxSpecificFolders->setChecked( true );
}

void SearchWindow::activateFolder( const Akonadi::Collection &collection )
{
    mUi.mChkbxSpecificFolders->setChecked( true );
    mUi.mCbxFolders->setCollection( collection );
}

void SearchWindow::slotSearch()
{
    if ( mUi.mSearchFolderEdt->text().isEmpty() ) {
        mUi.mSearchFolderEdt->setText( i18n( "Last Search" ) );
    }

    if ( mResultModel )
        mHeaderState = mUi.mLbxMatches->header()->saveState();

    mUi.mLbxMatches->setModel( 0 );

    mSortColumn = mUi.mLbxMatches->header()->sortIndicatorSection();
    mSortOrder = mUi.mLbxMatches->header()->sortIndicatorOrder();
    mUi.mLbxMatches->setSortingEnabled( false );

    if ( mSearchJob ) {
        mSearchJob->kill( KJob::Quietly );
        mSearchJob->deleteLater();
        mSearchJob = 0;
    }

    mUi.mSearchFolderEdt->setEnabled( false );

    KUrl::List urls;
    if ( mUi.mChkbxSpecificFolders->isChecked() ) {
        const Akonadi::Collection col = mUi.mCbxFolders->collection();
        if (!col.isValid()) {
            mSearchPatternWidget->showWarningPattern(QStringList()<<i18n("You did not selected a valid folder."));
            mUi.mSearchFolderEdt->setEnabled( true );
            return;
        }
        urls << col.url( Akonadi::Collection::UrlShort );
        if ( mUi.mChkSubFolders->isChecked() ) {
            childCollectionsFromSelectedCollection( col, urls );
        }
    } else if (mUi.mChkMultiFolders->isChecked()) {
        if (!mSelectMultiCollectionDialog) {
            return;
        }
        mCollectionId = mSelectMultiCollectionDialog->selectedCollection();
        Q_FOREACH(const Akonadi::Collection &col, mCollectionId) {
            urls << col.url( Akonadi::Collection::UrlShort );
        }
        if (urls.isEmpty()) {
            mUi.mSearchFolderEdt->setEnabled( true );
            mSearchPatternWidget->showWarningPattern(QStringList()<<i18n("You forgot to select collections."));
            mQuery.clear();
            return;
        }
    }

    mUi.mPatternEdit->updateSearchPattern();

    SearchPattern searchPattern( mSearchPattern );
    searchPattern.purify();

    MailCommon::SearchPattern::SparqlQueryError queryError = MailCommon::SearchPattern::NoError;
    queryError = searchPattern.asSparqlQuery(mQuery, urls);
    const QString queryLanguage = QLatin1String("SPARQL");

    switch(queryError) {
    case MailCommon::SearchPattern::NoError:
        break;
    case MailCommon::SearchPattern::MissingCheck:
        mUi.mSearchFolderEdt->setEnabled( true );
        mSearchPatternWidget->showWarningPattern(QStringList()<<i18n("You forgot to define condition."));
        mQuery.clear();
        return;
    case MailCommon::SearchPattern::FolderEmptyOrNotIndexed:
        mUi.mSearchFolderEdt->setEnabled( true );
        mSearchPatternWidget->showWarningPattern(QStringList()<<i18n("All folders selected are empty or were not indexed."));
        mQuery.clear();
        return;
    case MailCommon::SearchPattern::EmptyResult:
        mUi.mSearchFolderEdt->setEnabled( true );
        mQuery.clear();
        mUi.mStatusLbl->setText( i18n("No message found.") );
        createSearchModel();
        return;
    case MailCommon::SearchPattern::NotEnoughCharacters:
        mUi.mSearchFolderEdt->setEnabled( true );
        mSearchPatternWidget->showWarningPattern(QStringList()<<i18n("Contains condition cannot be used with a number of characters inferior to 4."));
        mQuery.clear();
        return;
    }
    mSearchPatternWidget->hideWarningPattern();
    qDebug() << queryLanguage;
    qDebug() << mQuery;
    mUi.mSearchFolderOpenBtn->setEnabled( true );

    if ( !mFolder.isValid() ) {
        qDebug()<<" create new folder";
        // FIXME if another app created a virtual 'Last Search' folder without
        // out custom attributes it will result in problems
        mSearchJob = new Akonadi::SearchCreateJob( mUi.mSearchFolderEdt->text(), mQuery, this );
    } else {
        qDebug()<<" use existing folder";
        Akonadi::PersistentSearchAttribute *attribute = mFolder.attribute<Akonadi::PersistentSearchAttribute>();
        attribute->setQueryLanguage( queryLanguage );
        attribute->setQueryString( mQuery );
        mSearchJob = new Akonadi::CollectionModifyJob( mFolder, this );
    }

    connect( mSearchJob, SIGNAL(result(KJob*)), SLOT(searchDone(KJob*)) );
    enableGUI();
    mUi.mStatusLbl->setText( i18n( "Searching..." ) );
}

void SearchWindow::searchDone( KJob* job )
{
    Q_ASSERT( job == mSearchJob );
    QMetaObject::invokeMethod( this, "enableGUI", Qt::QueuedConnection );
    if ( job->error() ) {
        KMessageBox::sorry( this, i18n("Cannot get search result. %1", job->errorString() ) );
        if ( mSearchJob ) {
            mSearchJob = 0;
        }
        enableGUI();
        mUi.mSearchFolderEdt->setEnabled( true );
    }
    else
    {
        if ( Akonadi::SearchCreateJob *searchJob = qobject_cast<Akonadi::SearchCreateJob*>( mSearchJob ) ) {
            mFolder = searchJob->createdCollection();
        } else if ( Akonadi::CollectionModifyJob *modifyJob = qobject_cast<Akonadi::CollectionModifyJob*>( mSearchJob ) ) {
            mFolder = modifyJob->collection();
        }
        /// TODO: cope better with cases where this fails
        Q_ASSERT( mFolder.isValid() );
        Q_ASSERT( mFolder.hasAttribute<Akonadi::PersistentSearchAttribute>() );

        GlobalSettings::setLastSearchCollectionId( mFolder.id() );
        GlobalSettings::self()->writeConfig();
        GlobalSettings::self()->requestSync();

        // store the kmail specific serialization of the search in an attribute on
        // the server, for easy retrieval when editing it again
        const QByteArray search = mSearchPattern.serialize();
        Q_ASSERT( !search.isEmpty() );
        Akonadi::SearchDescriptionAttribute *searchDescription = mFolder.attribute<Akonadi::SearchDescriptionAttribute>( Akonadi::Entity::AddIfMissing );
        searchDescription->setDescription( search );
        if ( mUi.mChkMultiFolders->isChecked()) {
            searchDescription->setBaseCollection( Akonadi::Collection() );
            QList<Akonadi::Collection::Id> lst;
            Q_FOREACH (const Akonadi::Collection &col, mCollectionId) {
                lst << col.id();
            }
            searchDescription->setListCollection(lst);
        } else if (mUi.mChkbxSpecificFolders->isChecked()) {
            const Akonadi::Collection collection = mUi.mCbxFolders->collection();
            searchDescription->setBaseCollection( collection );
        } else {
            searchDescription->setBaseCollection( Akonadi::Collection() );
        }
        searchDescription->setRecursive( mUi.mChkSubFolders->isChecked() );
        new Akonadi::CollectionModifyJob( mFolder, this );
        mSearchJob = 0;

        mUi.mStatusLbl->setText( QString() );
        createSearchModel();

        if ( mCloseRequested )
            close();

        mUi.mLbxMatches->setSortingEnabled( true );
        mUi.mLbxMatches->header()->setSortIndicator( mSortColumn, mSortOrder );

        mUi.mSearchFolderEdt->setEnabled( true );
    }
}

void SearchWindow::slotStop()
{
    if ( mSearchJob ) {
        mSearchJob->kill( KJob::Quietly );
        mSearchJob->deleteLater();
        mSearchJob = 0;
    }

    enableGUI();
}

void SearchWindow::slotClose()
{
    accept();
}

void SearchWindow::closeEvent( QCloseEvent *event )
{
    if ( mSearchJob ) {
        mCloseRequested = true;
        //Cancel search in progress
        mSearchJob->kill( KJob::Quietly );
        mSearchJob->deleteLater();
        mSearchJob = 0;
        QTimer::singleShot( 0, this, SLOT(slotClose()) );
    } else {
        KDialog::closeEvent( event );
    }
}

void SearchWindow::scheduleRename( const QString &text )
{
    if ( !text.isEmpty() ) {
        mRenameTimer.setSingleShot( true );
        mRenameTimer.start( 250 );
        mUi.mSearchFolderOpenBtn->setEnabled( false );
    } else {
        mRenameTimer.stop();
        mUi.mSearchFolderOpenBtn->setEnabled( !text.isEmpty() );
    }
}

void SearchWindow::renameSearchFolder()
{
    const QString name = mUi.mSearchFolderEdt->text();
    if ( mFolder.isValid() ) {
        const QString oldFolderName = mFolder.name();
        if ( oldFolderName != name ) {
            mFolder.setName( name );
            Akonadi::CollectionModifyJob *job = new Akonadi::CollectionModifyJob( mFolder, this );
            job->setProperty("oldfoldername", oldFolderName);
            connect( job, SIGNAL(result(KJob*)),
                     this, SLOT(slotSearchFolderRenameDone(KJob*)) );
        }
        mUi.mSearchFolderOpenBtn->setEnabled( true );
    }
}

void SearchWindow::slotSearchFolderRenameDone( KJob *job )
{
    Q_ASSERT( job );
    if ( job->error() ) {
        kWarning() << "Job failed:" << job->errorText();
        KMessageBox::information( this, i18n( "There was a problem renaming your search folder. "
                                              "A common reason for this is that another search folder "
                                              "with the same name already exists. Error returned \"%1\".", job->errorText() ) );
        mUi.mSearchFolderEdt->blockSignals(true);
        mUi.mSearchFolderEdt->setText(job->property("oldfoldername").toString());
        mUi.mSearchFolderEdt->blockSignals(false);
    }
}

void SearchWindow::openSearchFolder()
{
    Q_ASSERT( mFolder.isValid() );
    renameSearchFolder();
    mKMMainWidget->slotSelectCollectionFolder( mFolder );
    slotClose();
}

bool SearchWindow::slotShowMsg( const Akonadi::Item &item )
{
    if ( item.isValid() ) {
        mKMMainWidget->slotMessageSelected( item );
        return true;
    }
    return false;
}

void SearchWindow::slotViewSelectedMsg()
{
    mKMMainWidget->slotMessageActivated( selectedMessage() );
}

bool SearchWindow::slotViewMsg( const Akonadi::Item &item )
{
    if ( item.isValid() ) {
        mKMMainWidget->slotMessageActivated( item );
        return true;
    }
    return false;
}

void SearchWindow::slotCurrentChanged( const Akonadi::Item &item )
{
    mUi.mSearchResultOpenBtn->setEnabled( item.isValid() );
}

void SearchWindow::enableGUI()
{
    const bool searching = (mSearchJob != 0);

    mSearchButton->setGuiItem( searching ? mStopSearchGuiItem : mStartSearchGuiItem );
    if ( searching ) {
        disconnect( mSearchButton, SIGNAL(clicked()), this, SLOT(slotSearch()) );
        connect( mSearchButton, SIGNAL(clicked()), SLOT(slotStop()) );
    } else {
        disconnect( mSearchButton, SIGNAL(clicked()), this, SLOT(slotStop()) );
        connect( mSearchButton, SIGNAL(clicked()), SLOT(slotSearch()) );
    }
}

Akonadi::Item::List SearchWindow::selectedMessages() const
{
    Akonadi::Item::List messages;

    foreach ( const QModelIndex &index, mUi.mLbxMatches->selectionModel()->selectedRows() ) {
        const Akonadi::Item item = index.data( Akonadi::ItemModel::ItemRole ).value<Akonadi::Item>();
        if ( item.isValid() )
            messages.append( item );
    }

    return messages;
}

Akonadi::Item SearchWindow::selectedMessage() const
{
    return mUi.mLbxMatches->currentIndex().data( Akonadi::ItemModel::ItemRole ).value<Akonadi::Item>();
}

void SearchWindow::updateContextMenuActions()
{
    const int count = selectedMessages().count();
    const bool singleActions = (count == 1);

    mReplyAction->setEnabled( singleActions );
    mReplyAllAction->setEnabled( singleActions );
    mReplyListAction->setEnabled( singleActions );
    mPrintAction->setEnabled( singleActions );
    mSaveAtchAction->setEnabled( count > 0 );
    mSaveAsAction->setEnabled( count > 0 );
    mClearAction->setEnabled( count > 0 );
}

void SearchWindow::slotContextMenuRequested( const QPoint& )
{
    if ( !selectedMessage().isValid() || selectedMessages().isEmpty() )
        return;

    QMenu *menu = new QMenu( this );
    updateContextMenuActions();

    // show most used actions
    menu->addAction( mReplyAction );
    menu->addAction( mReplyAllAction );
    menu->addAction( mReplyListAction );
    menu->addAction( mForwardActionMenu );
    menu->addSeparator();
    KAction *act = mAkonadiStandardAction->createAction( Akonadi::StandardActionManager::CopyItems );
    mAkonadiStandardAction->setActionText( Akonadi::StandardActionManager::CopyItems, ki18np( "Copy Message", "Copy %1 Messages" ) );
    menu->addAction( act );
    act = mAkonadiStandardAction->createAction( Akonadi::StandardActionManager::CutItems );
    mAkonadiStandardAction->setActionText( Akonadi::StandardActionManager::CutItems, ki18np( "Cut Message", "Cut %1 Messages" ) );
    menu->addAction( act );
    menu->addAction( mAkonadiStandardAction->createAction( Akonadi::StandardActionManager::CopyItemToMenu ) );
    menu->addAction( mAkonadiStandardAction->createAction( Akonadi::StandardActionManager::MoveItemToMenu ) );
    menu->addSeparator();
    menu->addAction( mSaveAsAction );
    menu->addAction( mSaveAtchAction );
    menu->addAction( mPrintAction );
    menu->addSeparator();
    menu->addAction( mClearAction );
    menu->exec( QCursor::pos(), 0 );

    delete menu;
}

void SearchWindow::slotClearSelection()
{
    mUi.mLbxMatches->clearSelection();
}

void SearchWindow::slotReplyToMsg()
{
    KMCommand *command = new KMReplyCommand( this, selectedMessage(), MessageComposer::ReplySmart );
    command->start();
}

void SearchWindow::slotReplyAllToMsg()
{
    KMCommand *command = new KMReplyCommand( this, selectedMessage(),MessageComposer::ReplyAll );
    command->start();
}

void SearchWindow::slotReplyListToMsg()
{
    KMCommand *command = new KMReplyCommand( this, selectedMessage(),MessageComposer::ReplyList );
    command->start();
}

void SearchWindow::slotForwardMsg()
{
    KMCommand *command = new KMForwardCommand( this, selectedMessages() );
    command->start();
}

void SearchWindow::slotForwardAttachedMsg()
{
    KMCommand *command = new KMForwardAttachedCommand( this, selectedMessages() );
    command->start();
}

void SearchWindow::slotSaveMsg()
{
    KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand( this, selectedMessages() );
    saveCommand->start();
}

void SearchWindow::slotSaveAttachments()
{
    KMSaveAttachmentsCommand *saveCommand = new KMSaveAttachmentsCommand( this, selectedMessages() );
    saveCommand->start();
}

void SearchWindow::slotPrintMsg()
{
    KMCommand *command = new KMPrintCommand( this, selectedMessage() );
    command->start();
}

void SearchWindow::addRulesToSearchPattern( const SearchPattern &pattern )
{
    SearchPattern p( mSearchPattern );
    p.purify();

    QList<SearchRule::Ptr>::const_iterator it;
    QList<SearchRule::Ptr>::const_iterator end(pattern.constEnd() ) ;

    for ( it = pattern.constBegin() ; it != end ; ++it ) {
        p.append( SearchRule::createInstance( **it ) );
    }

    mSearchPattern = p;
    mUi.mPatternEdit->setSearchPattern( &mSearchPattern );
}

void SearchWindow::childCollectionsFromSelectedCollection( const Akonadi::Collection& collection, KUrl::List&lstUrlCollection )
{  
    if ( collection.isValid() )  {
        QModelIndex idx = Akonadi::EntityTreeModel::modelIndexForCollection( KMKernel::self()->collectionModel(), collection );
        if ( idx.isValid() ) {
            getChildren( KMKernel::self()->collectionModel(), idx, lstUrlCollection );
        }
    }
}

void SearchWindow::getChildren( const QAbstractItemModel *model,
                                const QModelIndex &parentIndex,
                                KUrl::List &list )
{
    const int rowCount = model->rowCount( parentIndex );
    for ( int row = 0; row < rowCount; ++row ) {
        const QModelIndex index = model->index( row, 0, parentIndex );
        if ( model->rowCount( index ) > 0 ) {
            getChildren( model, index, list );
        }
        Akonadi::Collection c = model->data(index, Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
        if ( c.isValid() )
            list << c.url( Akonadi::Collection::UrlShort );
    }
}

void SearchWindow::slotDebugQuery()
{
#if !defined(NDEBUG)
    QPointer<SearchDebugDialog> dlg = new SearchDebugDialog(mQuery, this);
    dlg->exec();
    delete dlg;
#endif
}

void SearchWindow::slotSelectMultipleFolders()
{
    mUi.mChkMultiFolders->setChecked(true);
    if (!mSelectMultiCollectionDialog)  {
        QList<Akonadi::Collection::Id> lst;
        Q_FOREACH (const Akonadi::Collection &col, mCollectionId) {
            lst << col.id();
        }
        mSelectMultiCollectionDialog = new MailCommon::SelectMultiCollectionDialog(lst, this);
    }
    mSelectMultiCollectionDialog->show();
}

}

#include "searchwindow.moc"
