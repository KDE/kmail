/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#ifndef kmmsgpartdlg_h
#define kmmsgpartdlg_h

#include <kdialogbase.h>
#include <kio/global.h>

class KMMessagePart;
class QPushButton;
class KComboBox;
class QComboBox;
class QCheckBox;
class QLabel;
class QLineEdit;

#undef None

/** @short GUI for KMMsgPartDialog
    @author Marc Mutz <mutz@kde.org>
*/
class KMMsgPartDialog: public KDialogBase
{
  Q_OBJECT

public:
  KMMsgPartDialog( const QString & caption=QString::null,
		   QWidget * parent=0, const char * name=0 );
  virtual ~KMMsgPartDialog();

  /** Get the currently selected mimetype */
  QString mimeType() const;
  /** Sets the mime type to be displayed. */
  void setMimeType( const QString & type, const QString & subtype );
  /** This is an overloaded member function, provided for
      convenience. It behaves essentially like the above function.

      Sets the mime type to be displayed, but only if @p mimeType
      passes @see KMimeTypeValidator's test.  */
  void setMimeType( const QString & mimeType );
  /** Sets the initial list of mime types to be displayed in the
      combobox. The items are @em not validated. */
  void setMimeTypeList( const QStringList & mimeTypes );

  /** Sets the size of the file to be attached in bytes. This is
      strictly informational and thus can't be queried. If @p approx
      is true, the size is an estimation based on typical */
  void setSize( KIO::filesize_t size, bool estimated=false );

  /** Returns the current file name of the attachment. Note that this
      doesn't define which file is being attached. It only defines
      what the attachment's filename parameter should contain. */
  QString fileName() const;
  /** Sets the file name of the attachment. Note that this doesn't
      define which file is being attached. It only defines what the
      attachment's filename parameter should contain. */
  void setFileName( const QString & fileName );

  /** Returns the content of the Content-Description header
      field. This field is only informational. */
  QString description() const;
  /** Sets the description of the attachment, ie. the content of the
      Content-Description header field. */
  void setDescription( const QString & description );

  /** The list of supported encodings */
  enum Encoding {
    None     = 0x00,
    SevenBit = 0x01,
    EightBit = 0x02,
    QuotedPrintable = 0x04,
    Base64   = 0x08
  };

  /** Returns the current encoding */
  Encoding encoding() const;
  /** Sets the encoding to use */
  void setEncoding( Encoding encoding );
  /** Sets the list of encodings to be shown. @p encodings is the
      bitwise OR of @see Encoding flags */
  void setShownEncodings( int encodings );

  /** Returns true if the attchment has a content-disposition of
      "inline", false otherwise. */
  bool isInline() const;
  /** Sets whether this attachment has a content-disposition of
      "inline" */
  void setInline( bool inlined );

  /** Returns whether or not this attachment is or shall be encrypted */
  bool isEncrypted() const;
  /** Sets whether or not this attachment is or should be encrypted */
  void setEncrypted( bool encrypted );
  /** Sets whether or not this attachment can be encrypted */
  void setCanEncrypt( bool enable );

  /** Returns whether or not this attachment is or shall be signed */
  bool isSigned() const;
  /** Sets whether or not this attachment is or should be signed */
  void setSigned( bool sign );
  /** Sets whether or not this attachment can be signed */
  void setCanSign( bool enable );

protected slots:
  void slotMimeTypeChanged( const QString & mimeType );

protected:
  KComboBox  *mMimeType;
  QLabel     *mIcon;
  QLabel     *mSize;
  QLineEdit  *mFileName;
  QLineEdit  *mDescription;
  QComboBox  *mEncoding;
  QCheckBox  *mInline;
  QCheckBox  *mEncrypted;
  QCheckBox  *mSigned;
  QStringList mI18nizedEncodings;
  bool mReadOnly;
};

/** @short The attachment dialog with convenience backward compatible methods
    @author Marc Mutz <mutz@kde.org>
*/
class KMMsgPartDialogCompat : public KMMsgPartDialog {
  Q_OBJECT
public:
  KMMsgPartDialogCompat( const char * caption=0, bool=FALSE );
  virtual ~KMMsgPartDialogCompat();

  /** Display information about this message part. */
  void setMsgPart(KMMessagePart* msgPart);

  /** Returns the (possibly modified) message part. */
  KMMessagePart* msgPart(void) const { return mMsgPart; }

protected slots:
  void slotOk();

protected:
  /** Applies changes from the dialog to the message part. Called
      when the Ok button is pressed. */
  void applyChanges(void);

  KMMessagePart *mMsgPart;
};

#endif /*kmmsgpartdlg_h*/
