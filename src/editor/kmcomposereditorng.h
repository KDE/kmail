/*
  SPDX-FileCopyrightText: 2015-2021 Laurent Montel <montel@kde.org>

  SPDX-License-Identifier: GPL-2.0-only
*/

#pragma once

#include <MessageComposer/RichTextComposerNg>
class KMComposerWin;
namespace KPIMTextEdit
{
class RichTextComposerEmailQuoteHighlighter;
}
class KMComposerEditorNg : public MessageComposer::RichTextComposerNg
{
    Q_OBJECT
public:
    explicit KMComposerEditorNg(KMComposerWin *win, QWidget *parent);
    ~KMComposerEditorNg() override;

    Q_REQUIRED_RESULT QString smartQuote(const QString &msg) override;

    void setHighlighterColors(KPIMTextEdit::RichTextComposerEmailQuoteHighlighter *highlighter) override;

    void showSpellConfigDialog(const QString &configFileName);

    Q_REQUIRED_RESULT MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus convertPlainText(MessageComposer::TextPart *textPart) override;
Q_SIGNALS:
    void insertSnippet();

protected:
    bool processModifyText(QKeyEvent *event) override;
    void addExtraMenuEntry(QMenu *menu, QPoint pos) override;
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;

private:
    KMComposerWin *const mComposerWin;
};

