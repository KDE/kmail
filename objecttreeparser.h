/*  -*- c++ -*-
    objecttreeparser.h

    KMail, the KDE mail client.
    Copyright (c) 2002-2003 Karl-Heinz Zimmer <khz@kde.org>
    Copyright (c) 2003      Marc Mutz <mutz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License,
    version 2.0, as published by the Free Software Foundation.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, US
*/


#ifndef _KMAIL_OBJECTTREEPARSER_H_
#define _KMAIL_OBJECTTREEPARSER_H_

#include "kmmsgbase.h"

#include <cryptplugwrapper.h>
#include <qcstring.h>

class KMReaderWin;
class QString;
class QWidget;
class partNode;

namespace KMail {

  class AttachmentStrategy;
  class HtmlWriter;

  class ProcessResult {
  public:
    ProcessResult( KMMsgSignatureState  inlineSignatureState  = KMMsgNotSigned,
                   KMMsgEncryptionState inlineEncryptionState = KMMsgNotEncrypted,
		   bool neverDisplayInline = false,
                   bool isImage = false )
      : mInlineSignatureState( inlineSignatureState ),
	mInlineEncryptionState( inlineEncryptionState ),
	mNeverDisplayInline( neverDisplayInline ),
	mIsImage( isImage ) {}

    KMMsgSignatureState inlineSignatureState() const {
      return mInlineSignatureState;
    }
    void setInlineSignatureState( KMMsgSignatureState state ) {
      mInlineSignatureState = state;
    }

    KMMsgEncryptionState inlineEncryptionState() const {
      return mInlineEncryptionState;
    }
    void setInlineEncryptionState( KMMsgEncryptionState state ) {
      mInlineEncryptionState = state;
    }

    bool neverDisplayInline() const { return mNeverDisplayInline; }
    void setNeverDisplayInline( bool display ) {
      mNeverDisplayInline = display;
    }

    bool isImage() const { return mIsImage; }
    void setIsImage( bool image ) {
      mIsImage = image;
    }
    
  private:
    KMMsgSignatureState mInlineSignatureState;
    KMMsgEncryptionState mInlineEncryptionState;
    bool mNeverDisplayInline : 1;
    bool mIsImage : 1;
  };

  class ObjectTreeParser {
    /** Internal. Copies the context of @p other, but not it's @ref
	resultString() */
    ObjectTreeParser( const ObjectTreeParser & other );
  public:
    ObjectTreeParser( KMReaderWin * reader=0, CryptPlugWrapper * wrapper=0,
		      bool showOneMimePart=false, bool keepEncryptions=false,
		      bool includeSignatures=true,
		      const KMail::AttachmentStrategy * attachmentStrategy=0,
		      KMail::HtmlWriter * htmlWriter=0 );
    virtual ~ObjectTreeParser();

    QCString resultString() const { return mResultString; }

    void setCryptPlugWrapper( CryptPlugWrapper * wrapper ) {
      mCryptPlugWrapper = wrapper;
    }
    CryptPlugWrapper * cryptPlugWrapper() const {
      return mCryptPlugWrapper;
    }

    bool showOnlyOneMimePart() const { return mShowOnlyOneMimePart; }
    void setShowOnlyOneMimePart( bool show ) {
      mShowOnlyOneMimePart = show;
    }

    bool keepEncryptions() const { return mKeepEncryptions; }
    void setKeepEncryptions( bool keep ) {
      mKeepEncryptions = keep;
    }

    bool includeSignatures() const { return mIncludeSignatures; }
    void setIncludeSignatures( bool include ) {
      mIncludeSignatures = include;
    }

    const KMail::AttachmentStrategy * attachmentStrategy() const {
      return mAttachmentStrategy;
    }

    KMail::HtmlWriter * htmlWriter() const { return mHtmlWriter; }

    /** Parse beginning at a given node and recursively parsing
        the children of that node and it's next sibling. */
    //  Function is called internally by "parseMsg(KMMessage* msg)"
    //  and it will be replaced once KMime is alive.
    void parseObjectTree( partNode * node );

    /** Save a QByteArray into a new temp. file using the extention
        given in dirExt to compose the directory name and
        the name given in fileName as file name
        and return the path+filename.
        If parameter reader is valid the directory and file names are added
        to the reader's temp directories and temp files lists. */
    static QString byteArrayToTempFile( KMReaderWin* reader,
                                        const QString& dirExt,
                                        const QString& fileName,
                                        const QByteArray& theBody );

  private:
    /** 1. Create a new partNode using 'content' data and Content-Description
            found in 'cntDesc'.
        2. Make this node the child of 'node'.
        3. Insert the respective entries in the Mime Tree Viewer.
        3. Parse the 'node' to display the content. */
    //  Function will be replaced once KMime is alive.
    void insertAndParseNewChildNode( partNode & node,
				     const char * content,
				     const char * cntDesc,
				     bool append=false );
    /** if data is 0:
	Feeds the HTML widget with the contents of the opaque signed
            data found in partNode 'sign'.
        if data is set:
            Feeds the HTML widget with the contents of the given
            multipart/signed object.
        Signature is tested.  May contain body parts.

        Returns whether a signature was found or not: use this to
        find out if opaque data is signed or not. */
    bool writeOpaqueOrMultipartSignedData( partNode * data,
					   partNode & sign,
					   const QString & fromAddress,
					   bool doCheck=true,
					   QCString * cleartextData=0,
					   struct CryptPlugWrapper::SignatureMetaData * paramSigMeta=0,
					   bool hideErrors=false );

    /** Returns the contents of the given multipart/encrypted
        object. Data is decypted.  May contain body parts. */
    bool okDecryptMIME( partNode& data,
			QCString& decryptedData,
			bool& signatureFound,
			struct CryptPlugWrapper::SignatureMetaData& sigMeta,
			bool showWarning,
			bool& passphraseError,
			QString& aErrorText );

    bool processTextType( int subtype, partNode * node, ProcessResult & result );

    bool processMultiPartType( int subtype, partNode * node, ProcessResult & result );

    bool processMessageType( int subtype, partNode * node, ProcessResult & result );

    bool processApplicationType( int subtype, partNode * node, ProcessResult & result );

    bool processImageType( int subtype, partNode * node, ProcessResult & result );

    bool processAudioType( int subtype, partNode * node, ProcessResult & result );

    bool processVideoType( int subtype, partNode * node, ProcessResult & result );

    bool processModelType( int subtype, partNode * node, ProcessResult & result );


    void writeBodyString( const QCString & bodyString,
			  const QString & fromAddress,
			  const QTextCodec * codec,
			  ProcessResult & result );

    const QTextCodec * codecFor( partNode * node ) const;

#ifndef NDEBUG
    void dumpToFile( const char * filename, const char * dataStart, size_t dataLen );
#else
    void dumpToFile( const char *, const char *, size_t ) {}
#endif

  private:
    KMReaderWin * mReader;
    QCString mResultString;
    CryptPlugWrapper * mCryptPlugWrapper;
    bool mShowOnlyOneMimePart;
    bool mKeepEncryptions;
    bool mIncludeSignatures;
    const KMail::AttachmentStrategy * mAttachmentStrategy;
    KMail::HtmlWriter * mHtmlWriter;
  };

}; // namespace KMail

#endif // _KMAIL_OBJECTTREEPARSER_H_

