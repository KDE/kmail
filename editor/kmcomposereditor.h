/**
 * composer/kmeditor.h
 *
 * Copyright (C)  2007-2013  Laurent Montel <montel@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef KMCOMPOSEREDITOR_H
#define KMCOMPOSEREDITOR_H

#include <messagecomposer/composer/kmeditor.h>

class KAction;
class KMComposeWin;

class KMComposerEditor : public MessageComposer::KMeditor
{
    Q_OBJECT

public:

    /**
     * Constructs a KMComposerEditor object.
     */
    explicit KMComposerEditor( KMComposeWin *win,QWidget *parent = 0 );

    ~KMComposerEditor();

    /**
     * Reimplemented from KMEditor, to support more actions.
     *
     * The additional action XML names are:
     * - paste_quoted
     * - tools_quote
     * - tools_unquote
     */
    void createActions( KActionCollection *actionCollection );

    /**
     * This replaces all characters not known to the specified codec with
     * questionmarks. HTML formatting is preserved.
     */
    void replaceUnknownChars( const QTextCodec *codec );

    /**
     * Reimplemented from KMEditor.
     */
    QString smartQuote( const QString & msg );

    /**
     * Reimplemented from KMEditor.
     */
    void setHighlighterColors(KPIMTextEdit::EMailQuoteHighlighter * highlighter);

    /**
     * Static override because we want to hide part of the dialog UI
     */
    void showSpellConfigDialog( const QString &configFileName );

private:
    KMComposeWin *mComposerWin;

protected:
    bool canInsertFromMimeData( const QMimeData *source ) const;
    void insertFromMimeData( const QMimeData *source );

protected slots:
    void mousePopupMenuImplementation(const QPoint& pos);

signals:
    void insertSnippet();

};

#endif

