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

class KAction;
class KMComposeWin;

namespace KMail {

/**
  * Holds information about an embedded HTML image.
  * A list with all images can be retrieved with KMComposerEditor::embeddedImages().
  */
struct EmbeddedImage
{
  QByteArray image;   ///< The image, encoded as PNG with base64 encoding
  QString contentID;  ///< The content id of the embedded image
  QString imageName;  ///< Name of the image as it is available as a resource in the editor
};

}

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
     * Reimplemented from KMEditor, to support more actions.
     *
     * The additional action XML names are:
     * - add_image
     */
    virtual void createActions( KActionCollection *actionCollection );

    /**
     * Reimplemented from KMEditor.
     *
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

    /**
     * Reimplemented from KMEditor.
     */
    virtual QString smartQuote( const QString & msg );

    /**
     * Reimplemented from KMEditor.
     */
    virtual void changeHighlighterColors(KPIM::KEMailQuotingHighlighter * highlighter);

    /**
     * Adds an image. The image is loaded from file and then pasted to the current
     * cursor position.
     *
     * @param url The URL of the file which contains the image
     */
     void addImage( const KUrl &url );

     /**
      * Get a list with all embedded HTML images.
      * If the same image is contained twice or more in the editor, it will have only
      * one entry in this list.
      *
      * The ownership of the EmbeddedImage pointers is transferred to the caller, the
      * caller is responsible for deleting them again.
      *
      * @return a list of embedded HTML images of the editor.
      */
     QList<KMail::EmbeddedImage*> embeddedImages() const;

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

    /**
     * Helper function to get the list of all QTextImageFormats in the document.
     */
    QList<QTextImageFormat> embeddedImageFormats() const;

  public slots:

    /**
     * Pastes the content of the clipboard into the editor, if the
     * mime type of the clipboard's contents in supported.
     */
     void paste();

  private:

     KMComposeWin *m_composerWin;
     QString m_quotePrefix;
     KAction *actionAddImage;

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

  protected slots:

    void slotAddImage();

  signals:
     void insertSnippet();

};

#endif

