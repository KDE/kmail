/*
   SPDX-FileCopyrightText: 2016-2025 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

namespace MessageComposer
{
class PluginEditorCheckBeforeSendInterface;
class PluginEditorCheckBeforeSendParams;
}

class KMailPluginEditorCheckBeforeSendManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit KMailPluginEditorCheckBeforeSendManagerInterface(QObject *parent = nullptr);
    ~KMailPluginEditorCheckBeforeSendManagerInterface() override;

    QWidget *parentWidget() const;
    void setParentWidget(QWidget *parentWidget);

    // TODO add Identity
    // TODO add Emails
    // TODO add body ? or editor

    void initializePlugins();
    [[nodiscard]] bool execute(const MessageComposer::PluginEditorCheckBeforeSendParams &params) const;

private:
    QList<MessageComposer::PluginEditorCheckBeforeSendInterface *> mListPluginInterface;
    QWidget *mParentWidget = nullptr;
};
