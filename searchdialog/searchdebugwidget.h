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

class QListView;
class QStringListModel;
class KTextBrowser;
class KTextEdit;
class KJob;
class QModelIndex;
class QPushButton;
class QLabel;

namespace KPIMUtils {
class ProgressIndicatorWidget;
}

class SearchDebugWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SearchDebugWidget(const QString &query, QWidget *parent=0);
    ~SearchDebugWidget();

    QString queryStr() const;

private Q_SLOTS:
    void slotSearchFinished(KJob*);
    void slotFetchItem( const QModelIndex &index );
    void slotItemFetched(KJob*);
    void slotSearch();
    void slotUpdateSearchButton();

private:
    void indentQuery(QString query);
    QStringListModel *mResultModel;
    QListView *mResultView;
    KTextBrowser *mItemView;
    KTextEdit *mTextEdit;
    KPIMUtils::ProgressIndicatorWidget *mProgressIndicator;
    QPushButton *mSearchButton;
    QLabel *mResultLabel;
};

#endif // SEARCHDEBUGWIDGET_H
