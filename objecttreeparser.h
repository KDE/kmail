/*  -*- mode: C++; c-file-style: "gnu" -*-
    objecttreeparser.h

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002-2003 Klarälvdalens Datakonsult AB
    Copyright (c) 2003      Marc Mutz <mutz@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef _KMAIL_OBJECTTREEPARSER_H_
#define _KMAIL_OBJECTTREEPARSER_H_

#include "kmmsgbase.h"

#include <kleo/cryptobackend.h>
#include <gpgme++/verificationresult.h>

#include <cassert>

class KMReaderWin;
class KMMessagePart;
class QString;
class partNode;

namespace GpgME {
  class Error;
}

namespace MessageViewer {
  class CSSHelper;
  class AttachmentStrategy;
  class HtmlWriter;
}
class PartMetaData;
namespace KMail {

  class ProcessResult {
  public:
    explicit ProcessResult( KMMsgSignatureState  inlineSignatureState  = KMMsgNotSigned,
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

    void adjustCryptoStatesOfNode( partNode * node ) const;

  private:
    KMMsgSignatureState mInlineSignatureState;
    KMMsgEncryptionState mInlineEncryptionState;
    bool mNeverDisplayInline : 1;
    bool mIsImage : 1;
  };

  class ObjectTreeParser {
    class CryptoProtocolSaver;
    /** Internal. Copies the context of @p other, but not it's rawReplyString() */
    ObjectTreeParser( const ObjectTreeParser & other );
  public:
    explicit ObjectTreeParser( KMReaderWin * reader=0, const Kleo::CryptoBackend::Protocol * protocol=0,
                      bool showOneMimePart=false, bool keepEncryptions=false,
                      bool includeSignatures=true,
                      const MessageViewer::AttachmentStrategy * attachmentStrategy=0,
                      MessageViewer::HtmlWriter * htmlWriter=0,
                      MessageViewer::CSSHelper * cssHelper=0 );
    virtual ~ObjectTreeParser();

    void setAllowAsync( bool allow ) { assert( !mHasPendingAsyncJobs ); mAllowAsync = allow; }
    bool allowAsync() const { return mAllowAsync; }

    bool hasPendingAsyncJobs() const { return mHasPendingAsyncJobs; }

    QByteArray rawReplyString() const { return mRawReplyString; }

    /*! @return the text of the message, ie. what would appear in the
        composer's text editor if this was edited. */
    QString textualContent() const { return mTextualContent; }

    QByteArray textualContentCharset() const { return mTextualContentCharset; }

    void setCryptoProtocol( const Kleo::CryptoBackend::Protocol * protocol ) {
      mCryptoProtocol = protocol;
    }
    const Kleo::CryptoBackend::Protocol* cryptoProtocol() const {
      return mCryptoProtocol;
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

    const MessageViewer::AttachmentStrategy * attachmentStrategy() const {
      return mAttachmentStrategy;
    }

    MessageViewer::HtmlWriter * htmlWriter() const { return mHtmlWriter; }

    MessageViewer::CSSHelper * cssHelper() const { return mCSSHelper; }

    /** Parse beginning at a given node and recursively parsing
        the children of that node and it's next sibling. */
    //  Function is called internally by "parseMsg(KMMessage* msg)"
    //  and it will be replaced once KMime is alive.
    void parseObjectTree( partNode * node );

  private:
    /** Standard children handling a.k.a. multipart/mixed (w/o
        kroupware hacks) */
    void stdChildHandling( partNode * child );

    void defaultHandling( partNode * node, ProcessResult & result );

    /**
     *  1. Create a new partNode using 'content' data and Content-Description
     *      found in 'cntDesc'.
     *  2. Make this node the child of 'node'.
     *  3. Insert the respective entries in the Mime Tree Viewer.
     *  3. Parse the 'node' to display the content.
     *
     * @param addToTextualContent If true, this will add the textual content of the parsed node
     *                            to the textual content of the current object tree parser.
     *                            Setting this to false is useful for encapsulated messages, as we
     *                            do not want the text in those to appear in the editor
     */
    //  Function will be replaced once KMime is alive.
    void insertAndParseNewChildNode( partNode & node,
                                     const char * content,
                                     const char * cntDesc,
                                     bool append=false,
                                     bool addToTextualContent = true );
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
                                           QByteArray * cleartextData=0,
                                           const std::vector<GpgME::Signature> & paramSignatures = std::vector<GpgME::Signature>(),
                                           bool hideErrors=false );

    /** Writes out the block that we use when the node is encrypted,
        but we're deferring decryption for later. */
    void writeDeferredDecryptionBlock();

    /** Writes out the block that we use when the node is encrypted,
        but we've just kicked off async decryption. */
    void writeDecryptionInProgressBlock();

    /** Returns the contents of the given multipart/encrypted
        object. Data is decypted.  May contain body parts. */
    bool okDecryptMIME( partNode& data,
                        QByteArray& decryptedData,
                        bool& signatureFound,
                        std::vector<GpgME::Signature> &signatures,
                        bool showWarning,
                        bool& passphraseError,
                        bool& actuallyEncrypted,
                        bool& decryptionStarted,
                        QString& aErrorText,
                        GpgME::Error & auditLogError,
                        QString& auditLog );

    bool processMailmanMessage( partNode * node );

    /** Checks whether @p str contains external references. To be precise,
        we only check whether @p str contains 'xxx="http[s]:' where xxx is
        not href. Obfuscated external references are ignored on purpose.
    */
    static bool containsExternalReferences( const QString & str );

  public:// (during refactoring)

    bool processTextHtmlSubtype( partNode * node, ProcessResult & result );
    bool processTextPlainSubtype( partNode * node, ProcessResult & result );

    bool processMultiPartMixedSubtype( partNode * node, ProcessResult & result );
    bool processMultiPartAlternativeSubtype( partNode * node, ProcessResult & result );
    bool processMultiPartDigestSubtype( partNode * node, ProcessResult & result );
    bool processMultiPartParallelSubtype( partNode * node, ProcessResult & result );
    bool processMultiPartSignedSubtype( partNode * node, ProcessResult & result );
    bool processMultiPartEncryptedSubtype( partNode * node, ProcessResult & result );

    bool processMessageRfc822Subtype( partNode * node, ProcessResult & result );

    bool processApplicationOctetStreamSubtype( partNode * node, ProcessResult & result );
    bool processApplicationPkcs7MimeSubtype( partNode * node, ProcessResult & result );
    bool processApplicationChiasmusTextSubtype( partNode * node, ProcessResult & result );
    bool processApplicationMsTnefSubtype( partNode *node, ProcessResult &result );

  private:
    bool decryptChiasmus( const QByteArray& data, QByteArray& bodyDecoded, QString& errorText );
    void writeBodyString( const QByteArray & bodyString,
                          const QString & fromAddress,
                          const QTextCodec * codec,
                          ProcessResult & result, bool decorate );

    void writePartIcon( KMMessagePart * msgPart, int partNumber, bool inlineImage = false );

    QString sigStatusToString( const Kleo::CryptoBackend::Protocol * cryptProto,
                               int status_code,
                               GpgME::Signature::Summary summary,
                               int & frameColor,
                               bool & showKeyInfos );
    QString writeSigstatHeader( PartMetaData & part,
                                const Kleo::CryptoBackend::Protocol * cryptProto,
                                const QString & fromAddress,
                                partNode *node = 0 );
    QString writeSigstatFooter( PartMetaData & part );

    // The attachment mark is a div that is placed around the attchment. It is used for drawing
    // a yellow border around the attachment when scrolling to it. When scrolling to it, the border
    // color of the div is changed, see KMReaderWin::scrollToAttachment().
    void writeAttachmentMarkHeader( partNode *node );
    void writeAttachmentMarkFooter();

    void writeBodyStr( const QByteArray & bodyString,
                       const QTextCodec * aCodec,
                       const QString & fromAddress,
                       KMMsgSignatureState &  inlineSignatureState,
                       KMMsgEncryptionState & inlineEncryptionState,
                       bool decorate );
  public: // KMReaderWin still needs this...
    void writeBodyStr( const QByteArray & bodyString,
                       const QTextCodec * aCodec,
                       const QString & fromAddress );

  private:
    /** Change the string to `quoted' html (meaning, that the quoted
        part of the message get italized */
    QString quotedHTML(const QString& pos, bool decorate);

    const QTextCodec * codecFor( partNode * node ) const;
    /** Check if the newline at position @p newLinePos in string @p s
        seems to separate two paragraphs (important for correct BiDi
        behavior, but is heuristic because paragraphs are not
        well-defined) */
    bool looksLikeParaBreak(const QString& s, unsigned int newLinePos) const;

#ifdef MARCS_DEBUG
    void dumpToFile( const char * filename, const char * dataStart, size_t dataLen );
#else
    void dumpToFile( const char *, const char *, size_t ) {}
#endif

  private:
    KMReaderWin * mReader;
    QByteArray mRawReplyString;
    QByteArray mTextualContentCharset;
    QString mTextualContent;
    const Kleo::CryptoBackend::Protocol * mCryptoProtocol;

    /// Show only one mime part means that the user has selected some node in the message structure
    /// viewer that is not the root, which means the user wants to only see the selected node and its
    /// children. If that is the case, this variable is set to true.
    /// The code needs to behave differently if this is set. For example, it should not process the
    /// siblings. Also, consider inline images: Normally, those nodes are completely hidden, as the
    /// HTML node embedds them. However, when showing only the node of the image, one has to show them,
    /// as their is no HTML node in which they are displayed. There are many more cases where this
    /// variable needs to be obeyed.
    /// This variable is set to false again when processing the children in stdChildHandling(), as
    /// the children can be completely displayed again.
    bool mShowOnlyOneMimePart;

    bool mKeepEncryptions;
    bool mIncludeSignatures;
    bool mHasPendingAsyncJobs;
    bool mAllowAsync;
    const MessageViewer::AttachmentStrategy * mAttachmentStrategy;
    MessageViewer::HtmlWriter * mHtmlWriter;
    MessageViewer::CSSHelper * mCSSHelper;
    // DataUrl Icons cache
    QString mCollapseIcon;
    QString mExpandIcon;
  };

} // namespace KMail

#endif // _KMAIL_OBJECTTREEPARSER_H_

