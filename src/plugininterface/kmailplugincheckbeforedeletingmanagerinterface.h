/*
   SPDX-FileCopyrightText: 2021-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <Akonadi/Item>
#include <QList>
#include <QObject>
namespace MessageViewer
{
class MessageViewerCheckBeforeDeletingInterface;
}
class QAction;
class KActionCollection;
class KMailPluginCheckBeforeDeletingManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit KMailPluginCheckBeforeDeletingManagerInterface(QObject *parent = nullptr);
    ~KMailPluginCheckBeforeDeletingManagerInterface() override;
    void initializePlugins();

    [[nodiscard]] QWidget *parentWidget() const;
    void setParentWidget(QWidget *newParentWidget);

    [[nodiscard]] Akonadi::Item::List confirmBeforeDeleting(const Akonadi::Item::List &list);

    void setActionCollection(KActionCollection *ac);

    const QList<QAction *> actions() const;

private:
    QList<QAction *> mActions;
    QList<MessageViewer::MessageViewerCheckBeforeDeletingInterface *> mListPluginInterface;
    QWidget *mParentWidget = nullptr;
    KActionCollection *mActionCollection = nullptr;
    bool mWasInitialized = false;
};
