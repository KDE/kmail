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

#ifndef SEARCHDEBUGWIDGET_H
#define SEARCHDEBUGWIDGET_H

#include <QWidget>
#include <QStyledItemDelegate>
#include <QListView>

class QStringListModel;
class KTextEdit;
class KJob;
class QModelIndex;
class QPushButton;
class QLabel;
class QPlainTextEdit;
class QContextMenuEvent;

namespace PimCommon {
class PlainTextEditFindBar;
}

namespace KPIMUtils {
class ProgressIndicatorWidget;
}

class SearchResultListView : public QListView
{
    Q_OBJECT
public:
    explicit SearchResultListView(QWidget *parent=0);
    ~SearchResultListView();

protected:
    void contextMenuEvent( QContextMenuEvent *event );
};

class SearchDebugListDelegate : public QStyledItemDelegate
{
public:
    SearchDebugListDelegate(QObject *parent = 0);
    ~SearchDebugListDelegate();
    QWidget *createEditor( QWidget *, const QStyleOptionViewItem &, const QModelIndex & ) const;
};

class SearchDebugWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SearchDebugWidget(const QString &query, QWidget *parent=0);
    ~SearchDebugWidget();

    QString queryStr() const;

protected:
    bool eventFilter( QObject* watched, QEvent* event );

private Q_SLOTS:
    void slotSearchFinished(KJob*);
    void slotFetchItem( const QModelIndex &index );
    void slotItemFetched(KJob*);
    void slotSearch();
    void slotUpdateSearchButton();
    void slotFind();
    void slotReduceQuery();

private:
    void indentQuery(QString query);
    QStringListModel *mResultModel;
    SearchResultListView *mResultView;
    QPlainTextEdit *mItemView;
    KTextEdit *mTextEdit;
    KPIMUtils::ProgressIndicatorWidget *mProgressIndicator;
    QPushButton *mSearchButton;
    QPushButton *mReduceQuery;
    QLabel *mResultLabel;
    PimCommon::PlainTextEditFindBar *mFindBar;
};

#endif // SEARCHDEBUGWIDGET_H
