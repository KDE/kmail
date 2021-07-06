/*
   SPDX-FileCopyrightText: 2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <AkonadiCore/Item>
#include <QObject>
#include <QVector>
namespace MessageViewer
{
class MessageViewerCheckBeforeDeletingInterface;
}
class KMailPluginCheckBeforeDeletingManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit KMailPluginCheckBeforeDeletingManagerInterface(QObject *parent = nullptr);
    ~KMailPluginCheckBeforeDeletingManagerInterface() override;
    void initializePlugins();

    Q_REQUIRED_RESULT QWidget *parentWidget() const;
    void setParentWidget(QWidget *newParentWidget);

    Q_REQUIRED_RESULT Akonadi::Item::List confirmBeforeDeleting(const Akonadi::Item::List &list);

private:
    QVector<MessageViewer::MessageViewerCheckBeforeDeletingInterface *> mListPluginInterface;
    QWidget *mParentWidget = nullptr;
    bool mWasInitialized = false;
};
