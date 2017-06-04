/*
  This file is part of Kontact.

  Copyright (c) 2003 Tobias Koenig <tokoe@kde.org>
  Copyright (C) 2013-2017 Laurent Montel <montel@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#ifndef SUMMARYWIDGET_H
#define SUMMARYWIDGET_H

#include <KontactInterface/Summary>

#include <KViewStateMaintainer>

namespace Akonadi
{
class ChangeRecorder;
class Collection;
class EntityTreeModel;
class ETMViewStateSaver;
}

namespace KontactInterface
{
class Plugin;
}

class KCheckableProxyModel;

class QGridLayout;
class QItemSelectionModel;
class QLabel;
class QModelIndex;

class SummaryWidget : public KontactInterface::Summary
{
    Q_OBJECT

public:
    SummaryWidget(KontactInterface::Plugin *plugin, QWidget *parent);

    int summaryHeight() const override
    {
        return 1;
    }
    QStringList configModules() const override;

protected:
    bool eventFilter(QObject *obj, QEvent *e) override;

public Q_SLOTS:
    void updateSummary(bool force) override;

private Q_SLOTS:
    void selectFolder(const QString &);
    void slotCollectionChanged();
    void slotUpdateFolderList();

private:
    void displayModel(const QModelIndex &, int &, const bool, QStringList);

    QList<QLabel *> mLabels;
    QGridLayout *mLayout;
    KontactInterface::Plugin *mPlugin;
    Akonadi::ChangeRecorder *mChangeRecorder;
    Akonadi::EntityTreeModel *mModel;
    KViewStateMaintainer<Akonadi::ETMViewStateSaver> *mModelState;
    KCheckableProxyModel *mModelProxy;
    QItemSelectionModel *mSelectionModel;
};

#endif
