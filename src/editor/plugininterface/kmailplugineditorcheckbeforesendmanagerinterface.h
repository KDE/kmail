/*
   Copyright (C) 2016 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KMAILPLUGINEDITORCHECKBEFORESENDMANAGERINTERFACE_H
#define KMAILPLUGINEDITORCHECKBEFORESENDMANAGERINTERFACE_H

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
    explicit KMailPluginEditorCheckBeforeSendManagerInterface(QObject *parent = Q_NULLPTR);
    ~KMailPluginEditorCheckBeforeSendManagerInterface();

    QWidget *parentWidget() const;
    void setParentWidget(QWidget *parentWidget);

    //TODO add Identity
    //TODO add Emails
    //TODO add body ? or editor

    void initializePlugins();
    bool execute(const MessageComposer::PluginEditorCheckBeforeSendParams &params) const;
private:
    QList<MessageComposer::PluginEditorCheckBeforeSendInterface *> mListPluginInterface;
    QWidget *mParentWidget;
};

#endif // KMAILPLUGINEDITORCHECKBEFORESENDMANAGERINTERFACE_H
