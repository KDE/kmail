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

#include <kmeditor.h>
using namespace KPIM;

class KMComposeWin;

class KMComposerEditor : public KMeditor
{
  Q_OBJECT

  public:
    /**
     * Constructs a KMComposerEditor object.
     */
    explicit KMComposerEditor( KMComposeWin *win,QWidget *parent = 0 );

    ~KMComposerEditor();

    /**
     * @return the quote prefix set before with setQuotePrefixName(), or an empty
     *         string if that was never called.
     */
    virtual QString quotePrefixName() const;

    /**
     * Sets a quote prefix. Lines starting with the passed quote prefix will
     * be highlighted as quotes (in addition to lines that are starting with
     * '>' and '|').
     */
    void setQuotePrefixName( const QString &quotePrefix );

    /**
     * This replaces all characters not known to the specified codec with
     * questionmarks. HTML formatting is preserved.
     */
    void replaceUnknownChars( const QTextCodec *codec );

    virtual QString smartQuote( const QString & msg );
    virtual void changeHighlighterColors(KPIM::KEMailQuotingHighlighter * highlighter);

    /**
     * Adds an image
     */
     void addImage( const KUrl &url );
     void addImage( QString &imagename, QImage &image );

  public slots:
     void paste();

  private:
     KMComposeWin *m_composerWin;
     QString m_quotePrefix;

     /**
      * The names of embedded images.
      * Used to easily obtain the names of the images. New images are compared to the the list and not added as resource if already present.
      */
     QStringList mImageNames;

  protected:
     void dropEvent( QDropEvent *e );
     bool canInsertFromMimeData( const QMimeData *source ) const;
     void insertFromMimeData( const QMimeData *source );

  signals:
     void attachPNGImageData( const QByteArray &image );
     void insertSnippet();

};

#endif

