/*
  Copyright (c) 2015-2020 Laurent Montel <montel@kde.org>

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

#ifndef KMCOMPOSEREDITORNG_H
#define KMCOMPOSEREDITORNG_H

#include "messagecomposer/richtextcomposerng.h"
class KMComposerWin;
namespace KPIMTextEdit {
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

    MessageComposer::PluginEditorConvertTextInterface::ConvertTextStatus convertPlainText(MessageComposer::TextPart *textPart) override;
Q_SIGNALS:
    void insertSnippet();

protected:
    bool processModifyText(QKeyEvent *event) override;
    void addExtraMenuEntry(QMenu *menu, QPoint pos) override;
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;

private:
    KMComposerWin *mComposerWin = nullptr;
};

#endif // KMCOMPOSEREDITORNG_H
