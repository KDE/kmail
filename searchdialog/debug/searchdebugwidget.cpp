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
#include "searchdebugnepomukshowdialog.h"

#include "util.h"

#include "pimcommon/widgets/plaintexteditfindbar.h"

#include <KPIMUtils/ProgressIndicatorWidget>

#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/itemsearchjob.h>

#include <KTextEdit>
#include <KMessageBox>
#include <KLocale>

#include <QPlainTextEdit>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QStringListModel>
#include <QPushButton>
#include <QShortcut>
#include <QContextMenuEvent>
#include <QMenu>
#include <QPointer>

SearchResultListView::SearchResultListView(QWidget *parent)
    : QListView(parent)
{

}

SearchResultListView::~SearchResultListView()
{

}

void SearchResultListView::contextMenuEvent( QContextMenuEvent * event )
{
    const QModelIndex index = indexAt( event->pos() );
    if (!index.isValid())
        return;

    QMenu *popup = new QMenu(this);
    QAction *searchNepomukShow = new QAction(i18n("Search with nepomuk show..."), popup);
    popup->addAction(searchNepomukShow);
    QAction *act = popup->exec( event->globalPos() );
    delete popup;
    if (act == searchNepomukShow) {
        const QString uid = index.data( Qt::DisplayRole ).toString();
        QPointer<SearchDebugNepomukShowDialog> dlg = new SearchDebugNepomukShowDialog(uid, this);
        dlg->exec();
        delete dlg;
    }
}

SearchDebugListDelegate::SearchDebugListDelegate( QObject *parent )
    : QStyledItemDelegate ( parent )
{
}

SearchDebugListDelegate::~SearchDebugListDelegate()
{
}

QWidget *SearchDebugListDelegate::createEditor( QWidget *, const QStyleOptionViewItem  &, const QModelIndex & ) const
{
    return 0;
}

SearchDebugWidget::SearchDebugWidget(const QString &query, QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *layout = new QGridLayout;

    mTextEdit = new KTextEdit( this );
    // we install an event filter to catch Ctrl+Return for quick query execution
    mTextEdit->installEventFilter( this );

    mTextEdit->setAcceptRichText(false);
    indentQuery(query);
    new Nepomuk2::SparqlSyntaxHighlighter( mTextEdit->document() );

    mResultView = new SearchResultListView;
    mResultView->setItemDelegate(new SearchDebugListDelegate(this));

    QWidget *w = new QWidget;
    QVBoxLayout *lay = new QVBoxLayout;
    mItemView = new QPlainTextEdit;
    mFindBar = new PimCommon::PlainTextEditFindBar( mItemView, this );
    lay->addWidget(mItemView);
    lay->addWidget(mFindBar);
    lay->setMargin(0);
    w->setLayout(lay);


    QShortcut *shortcut = new QShortcut( this );
    shortcut->setKey( Qt::Key_F+Qt::CTRL );
    connect( shortcut, SIGNAL(activated()), SLOT(slotFind()) );

    layout->addWidget( mTextEdit, 0, 0, 1, 2);
    layout->addWidget( new QLabel( i18n("Akonadi Id:") ), 1, 0 );
    layout->addWidget( new QLabel( i18n("Messages:") ), 1, 1 );

    layout->addWidget( mResultView, 2, 0, 1, 1 );
    layout->addWidget( w, 2, 1, 1, 1 );

    mReduceQuery = new QPushButton( i18n("Reduce query") );
    connect(mReduceQuery, SIGNAL(clicked()), SLOT(slotReduceQuery()));

    mSearchButton = new QPushButton( i18n("Search") );
    mSearchButton->setEnabled(false);
    connect( mSearchButton, SIGNAL(clicked()), this, SLOT(slotSearch()) );
    mProgressIndicator = new KPIMUtils::ProgressIndicatorWidget;

    mResultLabel = new QLabel;
    layout->addWidget( mResultLabel, 3, 0, Qt::AlignLeft );

    layout->addWidget( mProgressIndicator, 4, 0, Qt::AlignLeft );
    layout->addWidget( mSearchButton, 4, 1, Qt::AlignRight );
    layout->addWidget( mReduceQuery, 5, 1, Qt::AlignRight );
    setLayout(layout);

    connect( mResultView, SIGNAL(activated(QModelIndex)), this, SLOT(slotFetchItem(QModelIndex)) );
    connect(mTextEdit, SIGNAL(textChanged()), SLOT(slotUpdateSearchButton()));
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
    mSearchButton->setEnabled(!mTextEdit->toPlainText().isEmpty());
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
        mSearchButton->setEnabled(false);
        mReduceQuery->setEnabled(false);
        return;
    }

    mResultModel->setStringList( QStringList() );
    mItemView->clear();
    mResultLabel->clear();
    mProgressIndicator->start();
    mSearchButton->setEnabled(false);
    mReduceQuery->setEnabled(false);

    Akonadi::ItemSearchJob *job = new Akonadi::ItemSearchJob( query );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(slotSearchFinished(KJob*)) );
}

void SearchDebugWidget::indentQuery(QString query)
{
    query = query.simplified();
    QString newQuery;
    int i = 0;
    int indent = 0;
    int space = 4;

    while(i < query.size()) {
        newQuery.append(query[i]);
        if (query[i] != QLatin1Char('"') && query[i] != QLatin1Char('<') && query[i] != QLatin1Char('\'')) {
            if (query[i] == QLatin1Char('{')) {
                ++indent;
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
            ++i;
            while(i < query.size()) {
                if (query[i] == QLatin1Char('"') || query[i] == QLatin1Char('>') || query[i] == QLatin1Char('\'')) {
                    newQuery.append(query[i]);
                    break;
                }
                newQuery.append(query[i]);
                ++i;
            }
        }
        ++i;
    }
    mTextEdit->setPlainText( newQuery );
}

void SearchDebugWidget::slotReduceQuery()
{
    QString query = mTextEdit->toPlainText();
    KMail::Util::reduceQuery(query);
    indentQuery(query);
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

void SearchDebugWidget::slotFind()
{
    if ( mTextEdit->textCursor().hasSelection() )
        mFindBar->setText( mTextEdit->textCursor().selectedText() );
    mTextEdit->moveCursor(QTextCursor::Start);
    mFindBar->show();
    mFindBar->focusAndSetCursor();
}



#include "searchdebugwidget.moc"
