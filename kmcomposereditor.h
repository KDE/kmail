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
     * Adds an image. The image is loaded from file and then pasted to the current
     * cursor position.
     *
     * @param url The URL of the file which contains the image
     */
     void addImage( const KUrl &url );

  protected:

    /**
     * Helper function for addImage(), which does the actual work of adding the QImage as a
     * resource to the document, pasting it and adding it to the image name list.
     *
     * @param imageName the desired image name. If it is already taken, a number will
     *                  be appended to it
     * @param image the actual image to add
     */
    void addImageHelper( const QString &imageName, const QImage &image );

  public slots:

    /**
     * Pastes the content of the clipboard into the editor, if the
     * mime type of the clipboard's contents in supported.
     */
     void paste();

  private:
     KMComposeWin *m_composerWin;
     QString m_quotePrefix;

     /**
      * The names of embedded images.
      * Used to easily obtain the names of the images.
      * New images are compared to the the list and not added as resource if already present.
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

