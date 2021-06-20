/*
  This file is part of Kontact.

  SPDX-FileCopyrightText: 2003 Tobias Koenig <tokoe@kde.org>
  SPDX-FileCopyrightText: 2013-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later WITH Qt-Commercial-exception-1.0
*/

#pragma once

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

    Q_REQUIRED_RESULT int summaryHeight() const override;

protected:
    bool eventFilter(QObject *obj, QEvent *e) override;

public Q_SLOTS:
    void updateSummary(bool force) override;

private:
    void selectFolder(const QString &);
    void slotCollectionChanged();
    void slotUpdateFolderList();
    void displayModel(const QModelIndex &, int &, const bool, QStringList);

    QList<QLabel *> mLabels;
    QGridLayout *mLayout = nullptr;
    KontactInterface::Plugin *mPlugin = nullptr;
    Akonadi::ChangeRecorder *mChangeRecorder = nullptr;
    Akonadi::EntityTreeModel *mModel = nullptr;
    KViewStateMaintainer<Akonadi::ETMViewStateSaver> *mModelState = nullptr;
    KCheckableProxyModel *mModelProxy = nullptr;
    QItemSelectionModel *mSelectionModel = nullptr;
};

