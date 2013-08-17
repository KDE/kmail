/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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
#include "sparqlsyntaxhighlighter.h"

#include <KPIMUtils/ProgressIndicatorWidget>

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemsearchjob.h>

#include <KTextBrowser>
#include <KTextEdit>
#include <KMessageBox>
#include <KLocale>

#include <QGridLayout>
#include <QLabel>
#include <QListView>
#include <QStringListModel>
#include <QPushButton>


SearchDebugWidget::SearchDebugWidget(const QString &query, QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *layout = new QGridLayout;

    mTextEdit = new KTextEdit( this );
    mTextEdit->setAcceptRichText(false);
    indentQuery(query);
    new Nepomuk2::SparqlSyntaxHighlighter( mTextEdit->document() );

    mResultView = new QListView;
    mItemView = new KTextBrowser;
    layout->addWidget( mTextEdit, 0, 0, 1, 2);
    layout->addWidget( new QLabel( i18n("UIDS:") ), 1, 0 );
    layout->addWidget( new QLabel( i18n("Messages:") ), 1, 1 );

    layout->addWidget( mResultView, 2, 0, 1, 1 );
    layout->addWidget( mItemView, 2, 1, 1, 1 );
    mSearchButton = new QPushButton( i18n("Search") );
    connect( mSearchButton, SIGNAL(clicked()), this, SLOT(slotSearch()) );
    mProgressIndicator = new KPIMUtils::ProgressIndicatorWidget;

    mResultLabel = new QLabel;
    layout->addWidget( mResultLabel, 3, 0, Qt::AlignLeft );

    layout->addWidget( mProgressIndicator, 4, 0, Qt::AlignLeft );
    layout->addWidget( mSearchButton, 4, 1, Qt::AlignRight );
    setLayout(layout);

    connect( mResultView, SIGNAL(activated(QModelIndex)), this, SLOT(slotFetchItem(QModelIndex)) );

    mResultModel = new QStringListModel( this );
    mResultView->setModel( mResultModel );
    slotSearch();
}

SearchDebugWidget::~SearchDebugWidget()
{
}

QString SearchDebugWidget::queryStr() const
{
    return mTextEdit->toPlainText();
}

void SearchDebugWidget::slotSearch()
{
    const QString query = mTextEdit->toPlainText();

    if (query.isEmpty()) {
        mResultLabel->setText(i18n("Query is empty."));
        return;
    }
    mProgressIndicator->start();
    mSearchButton->setEnabled(false);

    Akonadi::ItemSearchJob *job = new Akonadi::ItemSearchJob( query );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotSearchFinished(KJob*)) );
}

void SearchDebugWidget::indentQuery(QString query)
{
    query= query.simplified();
    QString newQuery;
    int i = 0;
    int indent = 0;
    int space = 4;

    while(i < query.size()) {
        newQuery.append(query[i]);
        if (query[i] != QLatin1Char('"') && query[i] != QLatin1Char('<') && query[i] != QLatin1Char('\'')) {
            if (query[i] == QLatin1Char('{')) {
                indent++;
                newQuery.append(QLatin1Char('\n'));
                newQuery.append(QString().fill(QLatin1Char(' '), indent*space));
            } else if (query[i] == QLatin1Char('.')) {
                if(i+2<query.size()) {
                    if(query[i+1] == QLatin1Char('}')||query[i+2] == QLatin1Char('}')) {
                        newQuery.append(QLatin1Char('\n'));
                        newQuery.append(QString().fill(QLatin1Char(' '), (indent-1)*space));
                    } else {
                        newQuery.append(QLatin1Char('\n'));
                        newQuery.append(QString().fill(QLatin1Char(' '), indent*space));
                    }
                } else {
                    newQuery.append(QLatin1Char('\n'));
                    newQuery.append(QString().fill(QLatin1Char(' '), indent*space));
                }
            } else if (query[i] == QLatin1Char('}')) {
                indent--;
                if (i+2<query.size()) {
                    if (query[i+2] == QLatin1Char('.')||query[i+1] == QLatin1Char('.')) {
                        newQuery.append(QString().fill(QLatin1Char(' '), 1));
                    } else {
                        newQuery.append(QLatin1Char('\n'));
                        newQuery.append(QString().fill(QLatin1Char(' '), indent*space));
                    }
                } else {
                    newQuery.append(QLatin1Char('\n'));
                    newQuery.append(QString().fill(QLatin1Char(' '), indent*space));
                }
            }
        } else {
            i++;
            while(i < query.size()) {
                if (query[i] == QLatin1Char('"') || query[i] == QLatin1Char('>') || query[i] == QLatin1Char('\'')) {
                    newQuery.append(query[i]);
                    break;
                }
                newQuery.append(query[i]);
                i++;
            }
        }
        i++;
    }
    mTextEdit->setPlainText( newQuery );
}


void SearchDebugWidget::slotSearchFinished(KJob *job)
{
    mResultModel->setStringList( QStringList() );
    mItemView->clear();
    mProgressIndicator->stop();
    mSearchButton->setEnabled(true);
    mResultLabel->clear();

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
    mResultLabel->setText(i18np("1 message found", "%1 messages found", uidList.count()));
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
    mItemView->clear();

    if ( job->error() ) {
        KMessageBox::error( this, i18n("Error on fetching item") );
        return;
    }

    Akonadi::ItemFetchJob *fetchJob = qobject_cast<Akonadi::ItemFetchJob*>( job );
    if ( !fetchJob->items().isEmpty() ) {
        const Akonadi::Item item = fetchJob->items().first();
        mItemView->setPlainText( QString::fromUtf8( item.payloadData() ) );
    }
}


#include "searchdebugwidget.moc"
