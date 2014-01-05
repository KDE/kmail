/*
  Copyright (c) 2013, 2014 Montel Laurent <montel.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "searchdebugwidget.h"
#include "pimcommon/nepomukdebug/sparqlsyntaxhighlighter.h"
#include "pimcommon/nepomukdebug/searchdebugnepomukshowdialog.h"
#include "pimcommon/nepomukdebug/akonadiresultlistview.h"
#include "pimcommon/nepomukdebug/nepomukdebugutils.h"
#include "util.h"

#include "pimcommon/texteditor/plaintexteditor/plaintexteditorwidget.h"
#include "pimcommon/texteditor/plaintexteditor/plaintexteditor.h"


#include <KPIMUtils/ProgressIndicatorLabel>

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemsearchjob.h>

#include <KMessageBox>
#include <KLocalizedString>

#include <QGridLayout>
#include <QLabel>
#include <QStringListModel>
#include <QPushButton>
#include <QShortcut>

SearchDebugWidget::SearchDebugWidget(const QString &query, QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *layout = new QGridLayout;

    mTextEdit = new PimCommon::PlainTextEditorWidget( this );
    // we install an event filter to catch Ctrl+Return for quick query execution
    mTextEdit->installEventFilter( this );

    mTextEdit->editor()->setPlainText(PimCommon::NepomukDebugUtils::indentQuery(query));
    new PimCommon::SparqlSyntaxHighlighter( mTextEdit->editor()->document() );

    mResultView = new PimCommon::AkonadiResultListView;

    mItemView = new PimCommon::PlainTextEditorWidget;
    mItemView->setReadOnly(true);

    layout->addWidget( mTextEdit, 0, 0, 1, 2);
    layout->addWidget( new QLabel( i18n("Akonadi Id:") ), 1, 0 );
    layout->addWidget( new QLabel( i18n("Messages:") ), 1, 1 );

    layout->addWidget( mResultView, 2, 0, 1, 1 );
    layout->addWidget( mItemView, 2, 1, 1, 1 );

    mReduceQuery = new QPushButton( i18n("Reduce query") );
    connect(mReduceQuery, SIGNAL(clicked()), SLOT(slotReduceQuery()));

    mSearchButton = new QPushButton( i18n("Search") );
    mSearchButton->setEnabled(false);
    connect( mSearchButton, SIGNAL(clicked()), this, SLOT(slotSearch()) );
    mProgressIndicator = new KPIMUtils::ProgressIndicatorLabel(i18n("Searching..."));

    mResultLabel = new QLabel;
    layout->addWidget( mResultLabel, 3, 0, Qt::AlignLeft );

    layout->addWidget( mProgressIndicator, 4, 0, Qt::AlignLeft );
    layout->addWidget( mSearchButton, 4, 1, Qt::AlignRight );
    layout->addWidget( mReduceQuery, 5, 1, Qt::AlignRight );
    setLayout(layout);

    connect( mResultView, SIGNAL(activated(QModelIndex)), this, SLOT(slotFetchItem(QModelIndex)) );
    connect(mTextEdit->editor(), SIGNAL(textChanged()), SLOT(slotUpdateSearchButton()));
    mResultModel = new QStringListModel( this );
    mResultView->setModel( mResultModel );

    slotSearch();
}

SearchDebugWidget::~SearchDebugWidget()
{
}

bool SearchDebugWidget::eventFilter( QObject *watched, QEvent *event )
{
    if( watched == mTextEdit && event->type() == QEvent::KeyPress ) {
        QKeyEvent* kev = static_cast<QKeyEvent*>(event);
        if( kev->key() == Qt::Key_Return &&
                kev->modifiers() == Qt::ControlModifier ) {
            slotSearch();
            return true;
        }
    }

    return QWidget::eventFilter( watched, event );
}

void SearchDebugWidget::slotUpdateSearchButton()
{
    mSearchButton->setEnabled(!mTextEdit->editor()->toPlainText().isEmpty());
}

QString SearchDebugWidget::queryStr() const
{
    return mTextEdit->editor()->toPlainText();
}

void SearchDebugWidget::slotSearch()
{
    const QString query = mTextEdit->editor()->toPlainText();

    if (query.isEmpty()) {
        mResultLabel->setText(i18n("Query is empty."));
        mSearchButton->setEnabled(false);
        mReduceQuery->setEnabled(false);
        return;
    }

    mResultModel->setStringList( QStringList() );
    mItemView->editor()->clear();
    mResultLabel->clear();
    mProgressIndicator->start();
    mSearchButton->setEnabled(false);
    mReduceQuery->setEnabled(false);

    Akonadi::ItemSearchJob *job = new Akonadi::ItemSearchJob( query );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotSearchFinished(KJob*)) );
}

void SearchDebugWidget::slotReduceQuery()
{
    QString query = mTextEdit->editor()->toPlainText();
    KMail::Util::reduceQuery(query);
    mTextEdit->editor()->setPlainText(PimCommon::NepomukDebugUtils::indentQuery(query));
}

void SearchDebugWidget::slotSearchFinished(KJob *job)
{
    mProgressIndicator->stop();
    mSearchButton->setEnabled(true);
    mReduceQuery->setEnabled(true);

    if ( job->error() ) {
        KMessageBox::error( this, job->errorString() );
        return;
    }

    QStringList uidList;
    Akonadi::ItemSearchJob *searchJob = qobject_cast<Akonadi::ItemSearchJob*>( job );
    const Akonadi::Item::List items = searchJob->items();
    Q_FOREACH ( const Akonadi::Item &item, items ) {
        uidList << QString::number( item.id() );
    }

    mResultModel->setStringList( uidList );
    if (uidList.isEmpty()) {
        mResultLabel->setText(i18n("No message found"));
    } else {
        mResultLabel->setText(i18np("1 message found", "%1 messages found", uidList.count()));
    }
}

void SearchDebugWidget::slotFetchItem( const QModelIndex &index )
{
    if ( !index.isValid() )
        return;

    const QString uid = index.data( Qt::DisplayRole ).toString();
    Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob( Akonadi::Item( uid.toLongLong() ) );
    fetchJob->fetchScope().fetchFullPayload();
    connect( fetchJob, SIGNAL(result(KJob*)), this, SLOT(slotItemFetched(KJob*)) );
}

void SearchDebugWidget::slotItemFetched( KJob *job )
{
    mItemView->editor()->clear();

    if ( job->error() ) {
        KMessageBox::error( this, i18n("Error on fetching item") );
        return;
    }

    Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
    if ( !fetchJob->items().isEmpty() ) {
        const Akonadi::Item item = fetchJob->items().first();
        mItemView->editor()->setPlainText( QString::fromUtf8( item.payloadData() ) );
    }
}


