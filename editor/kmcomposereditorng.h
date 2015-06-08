/*
  Copyright (c) 2015 Montel Laurent <montel@kde.org>

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

#include "messagecomposer/composer-ng/richtextcomposer.h"
class KMComposeWin;
namespace KPIMTextEdit {
class EMailQuoteHighlighter;
}

class KMComposerEditorNg : public MessageComposer::RichTextComposer
{
    Q_OBJECT
public:
    explicit KMComposerEditorNg(KMComposeWin *win, QWidget *parent);
    ~KMComposerEditorNg();

    //TODO make it virtual.
    QString smartQuote(const QString &msg) Q_DECL_OVERRIDE;

    void setHighlighterColors(KPIMTextEdit::EMailQuoteHighlighter *highlighter) Q_DECL_OVERRIDE;

    void showSpellConfigDialog(const QString &configFileName);

Q_SIGNALS:
    void insertSnippet();

protected:
    void addExtraMenuEntry(QMenu *menu, const QPoint &pos) Q_DECL_OVERRIDE;
    bool canInsertFromMimeData(const QMimeData *source) const Q_DECL_OVERRIDE;
    void insertFromMimeData(const QMimeData *source) Q_DECL_OVERRIDE;

private:
    KMComposeWin *mComposerWin;
};

#endif // KMCOMPOSEREDITORNG_H
