/*
   Copyright (C) 2019 Montel Laurent <montel@kde.org>

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

#ifndef KMAILPLUGINGrammarEDITORMANAGERINTERFACE_H
#define KMAILPLUGINGrammarEDITORMANAGERINTERFACE_H

#include <QObject>
namespace KPIMTextEdit {
class RichTextComposer;
}
namespace PimCommon {
class CustomToolsWidgetNg;
}
class KActionCollection;
class KMailPluginGrammarEditorManagerInterface : public QObject
{
    Q_OBJECT
public:
    explicit KMailPluginGrammarEditorManagerInterface(QObject *parent = nullptr);
    ~KMailPluginGrammarEditorManagerInterface() = default;

    KPIMTextEdit::RichTextComposer *richTextEditor() const;
    void setRichTextEditor(KPIMTextEdit::RichTextComposer *richTextEditor);

    QWidget *parentWidget() const;
    void setParentWidget(QWidget *parentWidget);

    void initializePlugins();
    KActionCollection *actionCollection() const;
    void setActionCollection(KActionCollection *actionCollection);

    PimCommon::CustomToolsWidgetNg *customToolsWidget() const;
    void setCustomToolsWidget(PimCommon::CustomToolsWidgetNg *customToolsWidget);

private:
    Q_DISABLE_COPY(KMailPluginGrammarEditorManagerInterface)
    KPIMTextEdit::RichTextComposer *mRichTextEditor = nullptr;
    PimCommon::CustomToolsWidgetNg *mCustomToolsWidget = nullptr;
    QWidget *mParentWidget = nullptr;
    KActionCollection *mActionCollection = nullptr;
    bool mWasInitialized = false;
};

#endif // KMAILPLUGINGrammarEDITORMANAGERINTERFACE_H
