/**
 * kmeditor.h
 *
 * Copyright (C)  2007  Laurent Montel <montel@kde.org>
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

#include <messagecomposer/kmeditor.h>
#include <KJob>

class KAction;
class KMComposeWin;

class KMComposerEditor : public Message::KMeditor
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
    virtual void createActions( KActionCollection *actionCollection );

    virtual int quoteLength( const QString& line ) const;
    virtual const QString defaultQuoteSign() const;

    /**
     * Sets a quote prefix. Lines starting with the passed quote prefix will
     * be highlighted as quotes (in addition to lines that are starting with
     * '>' and '|').
     */
    void setQuotePrefixName( const QString &quotePrefix );

    /**
     * @return the quote prefix set before with setQuotePrefixName(), or an empty
     *         string if that was never called.
     */
    virtual QString quotePrefixName() const;

    /**
     * This replaces all characters not known to the specified codec with
     * questionmarks. HTML formatting is preserved.
     */
    void replaceUnknownChars( const QTextCodec *codec );

    /**
     * Reimplemented from KMEditor.
     */
    virtual QString smartQuote( const QString & msg );

    /**
     * Reimplemented from KMEditor.
     */
    virtual void setHighlighterColors(KPIMTextEdit::EMailQuoteHighlighter * highlighter);

  private:

     KMComposeWin *m_composerWin;
     QString m_quotePrefix;
     KAction *mPasteQuotation, *mAddQuoteChars, *mRemQuoteChars;

protected:

  virtual bool canInsertFromMimeData( const QMimeData *source ) const;
  virtual void insertFromMimeData( const QMimeData *source );

protected slots:
  void slotFetchJob( KJob * job );

  signals:
     void insertSnippet();

};

#endif

