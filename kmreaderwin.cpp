// kmreaderwin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>

//#define STRICT_RULES_OF_GERMAN_GOVERNMENT_02

#include <config.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <qclipboard.h>
#include <qhbox.h>
#include <qstyle.h>
#include <qtextcodec.h>
#include <qpaintdevicemetrics.h>

#include <kaction.h>
#include <kapplication.h>
#include <kcharsets.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpgp.h>
#include <kpgpblock.h>
#include <krun.h>
#include <ktempfile.h>
#include <kprocess.h>

// khtml headers
#include <khtml_part.h>
#include <khtmlview.h> // So that we can get rid of the frames
#include <dom/html_element.h>
#include <dom/html_block.h>


#include <mimelib/mimepp.h>
#include <mimelib/body.h>
#include <mimelib/utility.h>

#include "kmversion.h"
#include "kmglobal.h"
#include "kmmainwin.h"

#include "kbusyptr.h"
#include "kfileio.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmmsgpart.h"
#include "kmmsgpartdlg.h"
#include "kmreaderwin.h"
#include "partNode.h"
#include "linklocator.h"
#include "kmmsgdict.h"

// for the MIME structure viewer (khz):
#include "kmmimeparttree.h"


// for selection
//#include <X11/X.h>
//#include <X11/Xlib.h>
//#include <X11/Xatom.h>

// X headers...
#undef Never
#undef Always

//--- Sven's save attachments to /tmp start ---
#include <unistd.h>
#include <klocale.h>
#include <kstandarddirs.h>  // for access and getpid
#include <kglobalsettings.h>
//--- Sven's save attachments to /tmp end ---

// for the click on attachment stuff (dnaber):
#include <kuserprofile.h>

// Do the tmp stuff correctly - thanks to Harri Porten for
// reminding me (sven)

#include "vcard.h"
#include "kmdisplayvcard.h"
#include <kpopupmenu.h>
#include <qimage.h>

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

QPtrList<KMReaderWin> KMReaderWin::mStandaloneWindows;

class KMReaderWin::PartMetaData {
public:
    bool isSigned;
    bool isGoodSignature;
    CryptPlugWrapper::SigStatusFlags sigStatusFlags;
    QString signClass;
    QString signer;
    QStringList signerMailAddresses;
    QCString keyId;
    Kpgp::Validity keyTrust;
    QString status;  // to be used for unknown plug-ins
    int status_code; // to be used for i18n of OpenPGP and S/MIME CryptPlugs
    QString errorText;
    tm creationTime;
    bool isEncrypted;
    bool isDecryptable;
    QString decryptionError;
    bool isEncapsulatedRfc822Message;
    PartMetaData();
};
KMReaderWin::PartMetaData::PartMetaData()
{
    sigStatusFlags = CryptPlugWrapper::SigStatus_UNKNOWN;
    isSigned = false;
    isGoodSignature = false;
    isEncrypted = false;
    isDecryptable = false;
    isEncapsulatedRfc822Message = false;
}



class NewByteArray : public QByteArray
{
public:
    NewByteArray &appendNULL();
    NewByteArray &operator+=( const char * );
    NewByteArray &operator+=( const QByteArray & );
    NewByteArray &operator+=( const QCString & );
    QByteArray& qByteArray();
};

NewByteArray& NewByteArray::appendNULL()
{
    QByteArray::detach();
    uint len1 = size();
    if ( !QByteArray::resize( len1 + 1 ) )
        return *this;
    *(data() + len1) = '\0';
    return *this;
}
NewByteArray& NewByteArray::operator+=( const char * newData )
{
    if ( !newData )
        return *this;
    QByteArray::detach();
    uint len1 = size();
    uint len2 = qstrlen( newData );
    if ( !QByteArray::resize( len1 + len2 ) )
        return *this;
    memcpy( data() + len1, newData, len2 );
    return *this;
}
NewByteArray& NewByteArray::operator+=( const QByteArray & newData )
{
    if ( newData.isNull() )
        return *this;
    QByteArray::detach();
    uint len1 = size();
    uint len2 = newData.size();
    if ( !QByteArray::resize( len1 + len2 ) )
        return *this;
    memcpy( data() + len1, newData.data(), len2 );
    return *this;
}
NewByteArray& NewByteArray::operator+=( const QCString & newData )
{
    if ( newData.isEmpty() )
        return *this;
    QByteArray::detach();
    uint len1 = size();
    uint len2 = newData.length(); // forget about the trailing 0x00 !
    if ( !QByteArray::resize( len1 + len2 ) )
        return *this;
    memcpy( data() + len1, newData.data(), len2 );
    return *this;
}
QByteArray& NewByteArray::qByteArray()
{
    return *((QByteArray*)this);
}



//
// THIS IS AN INTERIM SOLUTION
// TO BE REMOVED ONCE AUTOMATIC PLUG-IN DETECTION IS FULLY WORKING
//
// STATIC:
bool KMReaderWin::foundMatchingCryptPlug( CryptPlugWrapperList* plugins,
                                          QString libName,
                                          CryptPlugWrapper** useThisCryptPlug_ref,
                                          QWidget* parent,
                                          QString verboseName )
{
  if( plugins && useThisCryptPlug_ref ) {
    *useThisCryptPlug_ref = 0;
    CryptPlugWrapper* current;
    QPtrListIterator<CryptPlugWrapper> it( *plugins );
    while( ( current = it.current() ) ) {
        ++it;
        if( 0 <= current->libName().find( libName, 0, false ) ) {
        *useThisCryptPlug_ref = current;
        return true;
        }
    }
  }
  if( parent )
    KMessageBox::information(parent,
      i18n("Problem: %1 Plug-In was not specified.\n"
           "Use the 'Settings/Configure KMail / Plugins' dialog to specify the"
           " Plug-In or ask your system administrator to do that for you.")
           .arg(verboseName),
           QString::null,
           "cryptoPluginBox");
  return false;
}


// this STATIC function will be replaced once KMime is alive (khz, 04.05.2001)
void KMReaderWin::insertAndParseNewChildNode( KMReaderWin* reader,
                                              QCString* resultStringPtr,
                                              CryptPlugWrapperList* cryptPlugList,
                                              CryptPlugWrapper*     useThisCryptPlug,
                                              partNode& node,
                                              const char* content,
                                              const char* cntDesc )
{
//  DwBodyPart* myBody = new DwBodyPart( DwString( content ), node.dwPart() );
  DwString cntStr( content );
  DwBodyPart* myBody = new DwBodyPart( cntStr, 0 );

  myBody->Parse();

  if( myBody->hasHeaders() ) {
    DwText& desc = myBody->Headers().ContentDescription();
    desc.FromString( cntDesc );
    desc.SetModified();
    //desc.Assemble();
    myBody->Headers().Parse();
  }

//  node.setFirstChild(new partNode(false, myBody))->buildObjectTree( false );
  partNode* newNode = new partNode(false, myBody);
  newNode = node.setFirstChild( newNode );
  newNode->buildObjectTree( false );

  if( node.mimePartTreeItem() ) {
kdDebug(5006) << "\n     ----->  Inserting items into MimePartTree\n" << endl;
    node.mChild->fillMimePartTree( node.mimePartTreeItem(),
                                    0,
                                    "",   // cntDesc,
                                    "",   // mainCntTypeStr,
                                    "",   // cntEnc,
                                    "" ); // cntSize );
kdDebug(5006) << "\n     <-----  Finished inserting items into MimePartTree\n" << endl;
  } else {
kdDebug(5006) << "\n     ------  Sorry, node.mimePartTreeItem() returns ZERO so"
              << "\n                    we cannot insert new lines into MimePartTree. :-(\n" << endl;
  }
kdDebug(5006) << "\n     ----->  Now parsing the MimePartTree\n" << endl;
  parseObjectTree( reader,
                   resultStringPtr,
                   cryptPlugList,
                   useThisCryptPlug,
                   node.mChild );// showOneMimePart, keepEncryptions, includeSignatures );
kdDebug(5006) << "\n     <-----  Finished parsing the MimePartTree in insertAndParseNewChildNode()\n" << endl;
}


// this STATIC function will be replaced once KMime is alive (khz, 29.11.2001)
void KMReaderWin::parseObjectTree( KMReaderWin* reader,
                                   QCString* resultStringPtr,
                                   CryptPlugWrapperList* cryptPlugList,
                                   CryptPlugWrapper*     useThisCryptPlug,
                                   partNode* node,
                                   bool showOneMimePart,
                                   bool keepEncryptions,
                                   bool includeSignatures/*,
                                   NewByteArray* resultingRawDataPtr*/ )
{
  kdDebug(5006) << "\n**\n** KMReaderWin::parseObjectTree( "
                << (node ? "node OK, " : "no node, ")
                << "showOneMimePart: " << (showOneMimePart ? "TRUE" : "FALSE")
                << " ) **\n**" << endl;

  // Use this string to return the first readable part's content.
  // - This is used to retrieve the quotable text for reply-to...
  QCString dummyStr;
  QCString& resultString( resultStringPtr ? *resultStringPtr : dummyStr);
/*
  // Use this array to return the complete data that were in this
  // message parts - *after* all encryption has been removed that
  // could be removed.
  // - This is used to store the message in decrypted form.
  NewByteArray dummyData;
  NewByteArray& resultingRawData( resultingRawDataPtr ? *resultingRawDataPtr
                                                      : dummyData );
*/

  // make sure we have a valid CryptPlugList
  bool tmpPlugList = !cryptPlugList;
  if( tmpPlugList ) {
    cryptPlugList = new CryptPlugWrapperList();
    KConfig *config = KGlobal::config();
    cryptPlugList->loadFromConfig( config );
  }


  if( showOneMimePart && reader ) {
    // clear the viewer
    reader->mViewer->view()->setUpdatesEnabled( false );
    reader->mViewer->view()->viewport()->setUpdatesEnabled( false );
    static_cast<QScrollView *>(reader->mViewer->widget())->ensureVisible(0,0);

    if (reader->mHtmlTimer.isActive())
    {
      reader->mHtmlTimer.stop();
      reader->mViewer->end();
    }
    reader->mHtmlQueue.clear();

    reader->mColorBar->hide();

    // start the new viewer content
    reader->mViewer->begin( KURL( "file:/" ) );
    reader->mViewer->write("<html><body" +
      (reader->mPrinting ? " bgcolor=\"#FFFFFF\""
                         : QString(" bgcolor=\"%1\"").arg(reader->c4.name())));
    if (reader->mBackingPixmapOn && !reader->mPrinting )
      reader->mViewer->write(" background=\"file://" + reader->mBackingPixmapStr + "\"");
    reader->mViewer->write(">");
  }
  if(node && (showOneMimePart || (reader && reader->mShowCompleteMessage && !node->mRoot ))) {
    if( showOneMimePart ) {
      // set this node and all it's children and their children to 'not yet processed'
      node->mWasProcessed = false;
      if( node->mChild )
        node->mChild->setProcessed( false );
    } else
      // set this node and all it's siblings and all it's childrens to 'not yet processed'
      node->setProcessed( false );
  }

  bool isImage = false;
  bool isInlineSigned = false;
  bool isInlineEncrypted = false;

  if( node ) {
    partNode* curNode = node;
    /*
    // process decrypting (or signature verifying, resp.)
    // via CRYPTPLUG
    if( mCryptPlugList && !curNode->mWasProcessed ) {
      partNode* signedDataPart    = 0;
      partNode* signaturePart     = 0;
      partNode* versionPart       = 0;
      partNode* encryptedDataPart = 0;
      CryptPlugWrapper* plugForSignatureVerifying
        = findPlugForSignatureVerifying( signedDataPart, signaturePart );
      CryptPlugWrapper* plugForDecrypting
        = findPlugForDecrypting( versionPart, encryptedDataPart );
      if( plugForSignatureVerifying ) {

        // Set the signature node to done to prevent it from being processed
        //   by parseObjectTree( data )  called from  writeOpaqueOrMultipartSignedData().
        signaturePart->setProcessed( true );
        writeOpaqueOrMultipartSignedData( *plugForSignatureVerifying,
                         *signedDataPart,
                         *signaturePart );
        signedDataPart->setProcessed( true );
        bDone = true;
      }
      if( plugForDecrypting ) {

        QCString decryptedData;
        if( okDecryptMIME( *plugForDecrypting,
                           versionPart,
                           *encryptedDataPart,
                           decryptedData,
                           true,
                           bool& isEncrypted,
                           QString& errCode ) ) {
          DwBodyPart* myBody = new DwBodyPart( DwString( decryptedData ),
                                               encryptedDataPart->dwPart() );
          myBody->Parse();
          partNode myBodyNode( true, myBody );
          myBodyNode.buildObjectTree( false );
          parseObjectTree( &myBodyNode );
        }
        else
        {
          if( versionPart )
            parseObjectTree( versionPart );
          writeHTMLStr("<hr>");
          writeHTMLStr(mCodec->toUnicode( decryptedData ));
        }
        if( versionPart )
          versionPart->setProcessed( true );
        encryptedDataPart->setProcessed( true );
        bDone = true;
      }
    }
    */

    // process all mime parts that are not covered by one of the CRYPTPLUGs
    if( !curNode->mWasProcessed ) {
      bool bDone = false;
      switch( curNode->type() ){
      case DwMime::kTypeText: {
kdDebug(5006) << "* text *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeHtml: {
            if( reader )
              kdDebug(5006) << "html, attachmentstyle = " << reader->mAttachmentStyle << endl;
            else
              kdDebug(5006) << "html" << endl;
            QCString cstr( curNode->msgPart().bodyDecoded() );
//            resultingRawData += cstr;
            resultString = cstr;
            if( !reader ) {
              bDone = true;
            } else if( reader->mAttachmentStyle == InlineAttmnt ||
                       (reader->mAttachmentStyle == SmartAttmnt &&
                        !curNode->isAttachment()) ||
                       (reader->mAttachmentStyle == IconicAttmnt &&
                        reader->mIsFirstTextPart) ||
                       showOneMimePart )
            {
              reader->mIsFirstTextPart = false;
              if( reader->htmlMail() ) {
                // ---Sven's strip </BODY> and </HTML> from end of attachment start-
                // We must fo this, or else we will see only 1st inlined html
                // attachment.  It is IMHO enough to search only for </BODY> and
                // put \0 there.
                int i = cstr.findRev("</body>", -1, false); //case insensitive
                if( 0 <= i )
                  cstr.truncate(i);
                else // just in case - search for </html>
                {
                  i = cstr.findRev("</html>", -1, false); //case insensitive
                  if( 0 <= i ) cstr.truncate(i);
                }
                // ---Sven's strip </BODY> and </HTML> from end of attachment end-
              } else {
                reader->writeHTMLStr(QString("<div style=\"margin:0px 5%;"
                                  "border:2px solid %1;padding:10px;"
                                  "text-align:left;font-size:90%\">")
                                  .arg( reader->cCBhtml.name() ) );
                reader->writeHTMLStr(i18n("<b>Note:</b> This is a HTML message. For "
                                  "security reasons, only the raw HTML code "
                                  "is shown. If you trust the sender of this "
                                  "message then you can activate formatted "
                                  //"HTML display by enabling <em>Prefer HTML "
                                  //"to Plain Text</em> in the <em>Folder</em> "
                                  //"menu."));
                                  "HTML display for this message by clicking "
                                  "<a href=\"kmail:showHTML\">here</a>."));
                reader->writeHTMLStr(     "</div><br /><br />");
              }
              reader->writeHTMLStr(reader->mCodec->toUnicode( reader->htmlMail() ? cstr : KMMessage::html2source( cstr )));
              bDone = true;
            }
            break;
          }
          case DwMime::kSubtypeXVCard: {
kdDebug(5006) << "v-card" << endl;
              // do nothing: X-VCard is handled in parseMsg(KMMessage* aMsg)
              //             _before_ calling parseObjectTree()
            }
            break;
          // Every 'Text' type that is not 'Html' or 'V-Card'
          // is processed like 'Plain' text:
          case DwMime::kSubtypeRichtext:
kdDebug(5006) << "rich text" << endl;
          case DwMime::kSubtypeEnriched:
kdDebug(5006) << "enriched " << endl;
          case DwMime::kSubtypePlain:
kdDebug(5006) << "plain " << endl;
          default: {
kdDebug(5006) << "default " << endl;
              QCString cstr( curNode->msgPart().bodyDecoded() );
//              resultingRawData += cstr;
              if( !reader || reader->mAttachmentStyle == InlineAttmnt ||
                 (reader->mAttachmentStyle == SmartAttmnt &&
                  !curNode->isAttachment()) ||
                  (reader->mAttachmentStyle == IconicAttmnt &&
                   reader->mIsFirstTextPart) || showOneMimePart )
              {
                if (reader) reader->mIsFirstTextPart = false;
                if( reader && curNode->isAttachment() && !showOneMimePart )
                  reader->queueHtml("<br><hr><br>");
                if( reader )
                  reader->writeBodyStr( cstr.data(),
                                        reader->mCodec,
                                        curNode->trueFromAddress(),
                                        &isInlineSigned, &isInlineEncrypted);
                resultString = cstr;
                bDone = true;
              }
            }
            break;
          }
        }
        break;
      case DwMime::kTypeMultipart: {
kdDebug(5006) << "* multipart *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeMixed: {
kdDebug(5006) << "mixed" << endl;
              if( curNode->mChild )
                parseObjectTree( reader,
                                 &resultString,
                                 cryptPlugList,
                                 useThisCryptPlug,
                                 curNode->mChild,
                                 false,
                                 keepEncryptions,
                                 includeSignatures );
              bDone = true;
            }
            break;
          case DwMime::kSubtypeAlternative: {
kdDebug(5006) << "alternative" << endl;
              if( curNode->mChild ) {
                partNode* dataHtml =
                  curNode->mChild->findType( DwMime::kTypeText, DwMime::kSubtypeHtml, false, true );
                partNode* dataPlain =
                  curNode->mChild->findType( DwMime::kTypeText, DwMime::kSubtypePlain, false, true );

                if( !reader || (reader->htmlMail() && dataHtml) ) {
                    if( dataPlain )
                        dataPlain->mWasProcessed = true;
                    parseObjectTree( reader,
                                     &resultString,
                                     cryptPlugList,
                                     useThisCryptPlug,
                                     dataHtml,
                                     false,
                                     keepEncryptions,
                                     includeSignatures );
                }
                else if( !reader || (!reader->htmlMail() && dataPlain) ) {
                    if( dataHtml )
                        dataHtml->mWasProcessed = true;
                    parseObjectTree( reader,
                                     &resultString,
                                     cryptPlugList,
                                     useThisCryptPlug,
                                     dataPlain,
                                     false,
                                     keepEncryptions,
                                     includeSignatures );
                }
                else
                    parseObjectTree( reader,
                                     &resultString,
                                     cryptPlugList,
                                     useThisCryptPlug,
                                     curNode->mChild,
                                     true,
                                     keepEncryptions,
                                     includeSignatures );
                bDone = true;
              }
            }
            break;
          case DwMime::kSubtypeDigest: {
kdDebug(5006) << "digest" << endl;
            }
            break;
          case DwMime::kSubtypeParallel: {
kdDebug(5006) << "parallel" << endl;
            }
            break;
          case DwMime::kSubtypeSigned: {
kdDebug(5006) << "signed" << endl;
              CryptPlugWrapper* oldUseThisCryptPlug = useThisCryptPlug;

              /*
                ATTENTION: We currently do _not_ support "multipart/signed" with _multiple_ signatures.
                          Instead we expect to find two objects: one object containing the signed data
                          and another object containing exactly one signature, this is determined by
                          looking for an "application/pgp-signature" object.
              */
              if( !curNode->mChild )
kdDebug(5006) << "       SORRY, signed has NO children" << endl;
              else {
kdDebug(5006) << "       signed has children" << endl;

                bool plugFound = false;

                /*
                  ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
                */
                partNode* data = 0;
                partNode* sign;
                sign = curNode->mChild->findType(      DwMime::kTypeApplication, DwMime::kSubtypePgpSignature, false, true );
                if( sign ) {
kdDebug(5006) << "       OpenPGP signature found" << endl;
                  data = curNode->mChild->findTypeNot( DwMime::kTypeApplication, DwMime::kSubtypePgpSignature, false, true );
                  if( data ){
                    curNode->setCryptoType( partNode::CryptoTypeOpenPgpMIME );
                    plugFound = foundMatchingCryptPlug( cryptPlugList, "openpgp", &useThisCryptPlug, reader, "OpenPGP" );
                  }
                }
                else {
                  sign = curNode->mChild->findType(      DwMime::kTypeApplication, DwMime::kSubtypePkcs7Signature, false, true );
                  if( sign ) {
kdDebug(5006) << "       S/MIME signature found" << endl;
                    data = curNode->mChild->findTypeNot( DwMime::kTypeApplication, DwMime::kSubtypePkcs7Signature, false, true );
                    if( data ){
                      curNode->setCryptoType( partNode::CryptoTypeSMIME );
                      plugFound = foundMatchingCryptPlug( cryptPlugList, "smime", &useThisCryptPlug, reader, "S/MIME" );
                    }
                  }
                  else
                  {
kdDebug(5006) << "       Sorry, *neither* OpenPGP *nor* S/MIME signature could be found!\n\n" << endl;
                  }
                }

                /*
                  ---------------------------------------------------------------------------------------------------------------
                */

                if( sign && data ) {
kdDebug(5006) << "       signed has data + signature" << endl;
                  curNode->setSigned( true );
                }

                if( !includeSignatures ) {
                  if( !data )
                    data = curNode->mChild;
                  QCString cstr( data->msgPart().bodyDecoded() );
                  if( reader )
                    reader->writeBodyStr(cstr,
                                         reader->mCodec,
                                         curNode->trueFromAddress(),
                                         &isInlineSigned, &isInlineEncrypted);
                  resultString += cstr;
                  bDone = true;
                } else if( sign && data && plugFound ) {
                  // Set the signature node to done to prevent it from being processed
                  // by parseObjectTree( data ) called from writeOpaqueOrMultipartSignedData().
                  sign->mWasProcessed = true;
                  writeOpaqueOrMultipartSignedData( reader,
                                                    &resultString,
                                                    cryptPlugList,
                                                    useThisCryptPlug,
                                                    data,
                                                    *sign,
                                                    curNode->trueFromAddress() );
                  bDone = true;
                }
              }
              useThisCryptPlug = oldUseThisCryptPlug;
            }
            break;
          case DwMime::kSubtypeEncrypted: {
kdDebug(5006) << "encrypted" << endl;
              CryptPlugWrapper* oldUseThisCryptPlug = useThisCryptPlug;
              if( keepEncryptions ) {
                curNode->setEncrypted( true );
                QCString cstr( curNode->msgPart().bodyDecoded() );
                if( reader )
                  reader->writeBodyStr(cstr,
                                       reader->mCodec,
                                       curNode->trueFromAddress(),
                                       &isInlineSigned, &isInlineEncrypted);
                resultString += cstr;
                bDone = true;
              } else if( curNode->mChild ) {

                  bool plugFound = false;

                  /*
                    ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
                  */
                  partNode* data =
                    curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypeOctetStream, false, true );
                  if( data ){
                    curNode->setCryptoType( partNode::CryptoTypeOpenPgpMIME );
                    plugFound = foundMatchingCryptPlug( cryptPlugList, "openpgp", &useThisCryptPlug, reader, "OpenPGP" );
                  }
                  if( !data ) {
                    data = curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypePkcs7Mime, false, true );
                  if( data ){
                    curNode->setCryptoType( partNode::CryptoTypeSMIME );
                    plugFound = foundMatchingCryptPlug( cryptPlugList, "smime", &useThisCryptPlug, reader, "S/MIME" );
                  }
                }
                /*
                  ---------------------------------------------------------------------------------------------------------------
                */

                if( data ) {
                  if( data->mChild ) {
kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
                    parseObjectTree( reader,
                                     &resultString,
                                     cryptPlugList,
                                     useThisCryptPlug,
                                     data->mChild,
                                     false,
                                     keepEncryptions,
                                     includeSignatures );
                    bDone = true;
kdDebug(5006) << "\n----->  Returning from parseObjectTree( curNode->mChild )\n" << endl;
                  }
                  else if( plugFound ) {
kdDebug(5006) << "\n----->  Initially processing encrypted data\n" << endl;
                    PartMetaData messagePart;
                    curNode->setEncrypted( true );
                    QCString decryptedData;
                    bool passphraseError;
                    if( okDecryptMIME( reader, cryptPlugList, useThisCryptPlug,
                                       *data,
                                       decryptedData,
                                       true,
                                       passphraseError,
                                       messagePart.errorText ) ) {
                      DwBodyPart* myBody = new DwBodyPart(DwString( decryptedData ), data->dwPart());
                      myBody->Parse();
                      partNode myBodyNode( true, myBody );
                      myBodyNode.buildObjectTree( false );

                      // paint the frame
                      if( reader ) {
                        messagePart.isDecryptable = true;
                        messagePart.isEncrypted = true;
                        messagePart.isSigned = false;
                        reader->queueHtml( reader->writeSigstatHeader( messagePart,
                                                                       useThisCryptPlug,
                                                                       curNode->trueFromAddress() ) );
                      }
                      insertAndParseNewChildNode( reader,
                                                  &resultString,
                                                  cryptPlugList,
                                                  useThisCryptPlug,
                                                  *curNode,
                                                  &*decryptedData,
                                                  "encrypted data" );
                      if( reader )
                        reader->queueHtml( reader->writeSigstatFooter( messagePart ) );
                    }
                    else
                    {
                      if( reader ) {
                        if( passphraseError ) {
                          messagePart.isDecryptable = false;
                          messagePart.isEncrypted = true;
                          messagePart.isSigned = false;
                          reader->queueHtml( reader->writeSigstatHeader( messagePart,
                                                                         useThisCryptPlug,
                                                                         curNode->trueFromAddress() ) );
                        }
                        reader->writeHTMLStr(reader->mCodec->toUnicode( decryptedData ));
                        if( passphraseError )
                          reader->queueHtml( reader->writeSigstatFooter( messagePart ) );
                      }
                      resultString += decryptedData;
                    }
                    data->mWasProcessed = true; // Set the data node to done to prevent it from being processed
                    bDone = true;
                  }
                }
              }
              useThisCryptPlug = oldUseThisCryptPlug;
            }
            break;
          default : {
kdDebug(5006) << "(  unknown subtype  )" << endl;
            }
            break;
          }
          //  Multipart object not processed yet?  Just parse the children!
          if( !bDone ){
            if( curNode && curNode->mChild ) {
              parseObjectTree( reader,
                               &resultString,
                               cryptPlugList,
                               useThisCryptPlug,
                               curNode->mChild,
                               false,
                               keepEncryptions,
                               includeSignatures );
              bDone = true;
            }
          }
        }
        break;
      case DwMime::kTypeMessage: {
kdDebug(5006) << "* message *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeRfc822: {
kdDebug(5006) << "RfC 822" << endl;
              if( reader->mAttachmentStyle != InlineAttmnt &&
                  (reader->mAttachmentStyle != SmartAttmnt ||
                   curNode->isAttachment()) && !showOneMimePart)
                 break;

              if( curNode->mChild ) {
kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
                parseObjectTree( reader,
                                 &resultString,
                                 cryptPlugList,
                                 useThisCryptPlug,
                                 curNode->mChild );
                bDone = true;
kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
              } else {
kdDebug(5006) << "\n----->  Initially processing data of embedded RfC 822 message\n" << endl;
                // paint the frame
                PartMetaData messagePart;
                if( reader ) {
                  messagePart.isEncrypted = false;
                  messagePart.isSigned = false;
                  messagePart.isEncapsulatedRfc822Message = true;
                  reader->queueHtml( reader->writeSigstatHeader( messagePart,
                                                                 useThisCryptPlug,
                                                                 curNode->trueFromAddress() ) );
                }
                QCString rfc822messageStr( curNode->msgPart().bodyDecoded() );
                // display the headers of the encapsulated message
                DwMessage* rfc822DwMessage = new DwMessage(); // will be deleted by c'tor of rfc822headers
                rfc822DwMessage->FromString( rfc822messageStr );
                rfc822DwMessage->Parse();
                KMMessage rfc822message( rfc822DwMessage );
                curNode->setFromAddress( rfc822message.from() );
kdDebug(5006) << "\n----->  Store RfC 822 message header \"From: " << rfc822message.from() << "\"\n" << endl;
                if( reader )
                  reader->parseMsg( &rfc822message, true );
                // display the body of the encapsulated message
                insertAndParseNewChildNode( reader,
                                            &resultString,
                                            cryptPlugList,
                                            useThisCryptPlug,
                                            *curNode,
                                            &*rfc822messageStr,
                                            "encapsulated message" );
                if( reader )
                  reader->queueHtml( reader->writeSigstatFooter( messagePart ) );
                bDone = true;
              }
            }
            break;
          }
        }
        break;
      /*
      case DwMime::kType..WhatTheHellIsThis: {
          switch( curNode->subType() ){
          case DwMime::kSubtypePartial: {
            }
            break;
          case DwMime::kSubtypeExternalBody: {
            }
            break;
          }
        }
        break;
      */
      case DwMime::kTypeApplication: {
kdDebug(5006) << "* application *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypePostscript: {
kdDebug(5006) << "postscript" << endl;
              isImage = true;
            }
            break;
          case DwMime::kSubtypeOctetStream: {
kdDebug(5006) << "octet stream" << endl;
              if( curNode->mChild ) {
kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
                parseObjectTree( reader,
                                 &resultString,
                                 cryptPlugList,
                                 useThisCryptPlug,
                                 curNode->mChild );
                bDone = true;
kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
              } else {
kdDebug(5006) << "\n----->  Initially processing encrypted data\n" << endl;
                CryptPlugWrapper* oldUseThisCryptPlug = useThisCryptPlug;
                if(    curNode->mRoot
                    && DwMime::kTypeMultipart    == curNode->mRoot->type()
                    && DwMime::kSubtypeEncrypted == curNode->mRoot->subType() ) {
                    curNode->setEncrypted( true );
                    curNode->setCryptoType( partNode::CryptoTypeOpenPgpMIME );
                    if( keepEncryptions ) {
                        QCString cstr( curNode->msgPart().bodyDecoded() );
                    if( reader )
                        reader->writeBodyStr(cstr,
                                             reader->mCodec,
                                             curNode->trueFromAddress(),
                                             &isInlineSigned, &isInlineEncrypted);
                    resultString += cstr;
                    bDone = true;
                    } else {
                    /*
                        ATTENTION: This code is to be replaced by the planned 'auto-detect' feature.
                    */
                    PartMetaData messagePart;
                    if( foundMatchingCryptPlug( cryptPlugList, "openpgp", &useThisCryptPlug, reader, "OpenPGP" ) ) {
                      QCString decryptedData;
                      bool passphraseError;
                      if( okDecryptMIME( reader, cryptPlugList, useThisCryptPlug,
                                         *curNode,
                                         decryptedData,
                                         true,
                                         passphraseError,
                                         messagePart.errorText ) ) {

                        // paint the frame
                        if( reader ) {
                          messagePart.isDecryptable = true;
                          messagePart.isEncrypted = true;
                          messagePart.isSigned = false;
                          reader->queueHtml( reader->writeSigstatHeader( messagePart,
                                                                         useThisCryptPlug,
                                                                         curNode->trueFromAddress() ) );
                        }
                        // fixing the missing attachments bug #1090-b
                        insertAndParseNewChildNode( reader,
                                                    &resultString,
                                                    cryptPlugList,
                                                    useThisCryptPlug,
                                                    *curNode,
                                                    &*decryptedData,
                                                    "encrypted data" );
                        if( reader )
                          reader->queueHtml( reader->writeSigstatFooter( messagePart ) );
                      }
                      else
                      {
                        if( reader ) {
                          if( passphraseError ) {
                            messagePart.isDecryptable = false;
                            messagePart.isEncrypted = true;
                            messagePart.isSigned = false;
                            reader->queueHtml( reader->writeSigstatHeader( messagePart,
                                                                           useThisCryptPlug,
                                                                           curNode->trueFromAddress() ) );
                          }
                          reader->writeHTMLStr(reader->mCodec->toUnicode( decryptedData ));
                          if( passphraseError )
                            reader->queueHtml( reader->writeSigstatFooter( messagePart ) );
                        }
                        resultString += decryptedData;
                      }
                    }
                    bDone = true;
                  }
                }
                useThisCryptPlug = oldUseThisCryptPlug;
              }
            }
            break;
          case DwMime::kSubtypePgpEncrypted: {
kdDebug(5006) << "pgp encrypted" << endl;
            }
            break;
          case DwMime::kSubtypePgpSignature: {
kdDebug(5006) << "pgp signed" << endl;
            }
            break;
          case DwMime::kSubtypePkcs7Mime: {
kdDebug(5006) << "pkcs7 mime" << endl;
              if( curNode->mChild ) {
kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
                parseObjectTree( reader,
                                 &resultString,
                                 cryptPlugList,
                                 useThisCryptPlug,
                                 curNode->mChild );
                bDone = true;
kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
              } else {
kdDebug(5006) << "\n----->  Initially processing signed and/or encrypted data\n" << endl;
                curNode->setCryptoType( partNode::CryptoTypeSMIME );
                if( curNode->dwPart() && curNode->dwPart()->hasHeaders() ) {
                  CryptPlugWrapper* oldUseThisCryptPlug = useThisCryptPlug;

                  if( foundMatchingCryptPlug( cryptPlugList, "smime", &useThisCryptPlug, reader, "S/MIME" ) ) {

                    DwHeaders& headers( curNode->dwPart()->Headers() );
                    QCString ctypStr( headers.ContentType().AsString().c_str() );
                    ctypStr.replace( QRegExp("\""), "" );
                    bool isSigned    = 0 <= ctypStr.find("smime-type=signed-data",    0, false);
                    bool isEncrypted = 0 <= ctypStr.find("smime-type=enveloped-data", 0, false);


                    // Analyze "signTestNode" node to find/verify a signature.
                    // If zero this verification was sucessfully done after
                    // decrypting via recursion by insertAndParseNewChildNode().
                    partNode* signTestNode = isEncrypted ? 0 : curNode;


                    // We try decrypting the content
                    // if we either *know* that it is an encrypted message part
                    // or there is neither signed nor encrypted parameter.
                    if( !isSigned ) {
                      if( isEncrypted )
                        kdDebug(5006) << "pkcs7 mime     ==      S/MIME TYPE: enveloped (encrypted) data" << endl;
                      else
                        kdDebug(5006) << "pkcs7 mime  -  type unknown  -  enveloped (encrypted) data ?" << endl;
                      QCString decryptedData;
                      PartMetaData messagePart;
                      messagePart.isEncrypted = true;
                      messagePart.isSigned = false;
                      bool passphraseError;
                      if( okDecryptMIME( reader, cryptPlugList, useThisCryptPlug,
                                         *curNode,
                                         decryptedData,
                                         false,
                                         passphraseError,
                                         messagePart.errorText ) ) {
                        kdDebug(5006) << "pkcs7 mime  -  encryption found  -  enveloped (encrypted) data !" << endl;
                        isEncrypted = true;
                        curNode->setEncrypted( true );
                        signTestNode = 0;
                        // paint the frame
                        messagePart.isDecryptable = true;
                        if( reader )
                          reader->queueHtml( reader->writeSigstatHeader( messagePart,
                                                                         useThisCryptPlug,
                                                                         curNode->trueFromAddress() ) );
                        insertAndParseNewChildNode( reader,
                                                    &resultString,
                                                    cryptPlugList,
                                                    useThisCryptPlug,
                                                    *curNode,
                                                    &*decryptedData,
                                                    "encrypted data" );
                        if( reader )
                            reader->queueHtml( reader->writeSigstatFooter( messagePart ) );
                      } else {

                        if( passphraseError ) {
                          isEncrypted = true;
                          signTestNode = 0;
                        }

                        if( isEncrypted ) {
                          kdDebug(5006) << "pkcs7 mime  -  ERROR: COULD NOT DECRYPT enveloped data !" << endl;
                          // paint the frame
                          messagePart.isDecryptable = false;
                          if( reader ) {
                            reader->queueHtml( reader->writeSigstatHeader( messagePart,
                                                                           useThisCryptPlug,
                                                                           curNode->trueFromAddress() ) );
                            reader->writePartIcon(&curNode->msgPart(), curNode->nodeId());
                            reader->queueHtml( reader->writeSigstatFooter( messagePart ) );
                          }
                        } else {
                          kdDebug(5006) << "pkcs7 mime  -  NO encryption found" << endl;
                        }
                      }
                      if( isEncrypted )
                        curNode->setEncrypted( true );
                    }

                    // We now try signature verification if necessarry.
                    if( signTestNode ) {
                      if( isSigned )
                        kdDebug(5006) << "pkcs7 mime     ==      S/MIME TYPE: opaque signed data" << endl;
                      else
                        kdDebug(5006) << "pkcs7 mime  -  type unknown  -  opaque signed data ?" << endl;

                      bool sigFound = writeOpaqueOrMultipartSignedData( reader,
                                        &resultString,
                                        cryptPlugList,
                                        useThisCryptPlug,
                                        0,
                                        *signTestNode,
                                        curNode->trueFromAddress(),
                                        isEncrypted );
                      if( sigFound ) {
                        if( !isSigned ) {
                          kdDebug(5006) << "pkcs7 mime  -  signature found  -  opaque signed data !" << endl;
                          isSigned = true;
                        }
                        signTestNode->setSigned( true );
                        if( signTestNode != curNode )
                          curNode->setSigned( true );
                      } else {
                        kdDebug(5006) << "pkcs7 mime  -  NO signature found   :-(" << endl;
                      }
                    }

                    if( isSigned || isEncrypted )
                      bDone = true;
                  }
                  useThisCryptPlug = oldUseThisCryptPlug;
                }
              }
            }
            break;
          }
        }
        break;
      case DwMime::kTypeImage: {
kdDebug(5006) << "* image *" << endl;

          switch( curNode->subType() ){
          case DwMime::kSubtypeJpeg: {
kdDebug(5006) << "JPEG" << endl;
            }
            break;
          case DwMime::kSubtypeGif: {
kdDebug(5006) << "GIF" << endl;
            }
            break;
          }
          isImage = true;
        }
        break;
      case DwMime::kTypeAudio: {
kdDebug(5006) << "* audio *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeBasic: {
kdDebug(5006) << "basic" << endl;
            }
            break;
          }
          // We allways show audio as icon.
          if( reader && ( reader->mAttachmentStyle != HideAttmnt || showOneMimePart ) )
            reader->writePartIcon(&curNode->msgPart(), curNode->nodeId());
          bDone = true;
        }
        break;
      case DwMime::kTypeVideo: {
kdDebug(5006) << "* video *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeMpeg: {
kdDebug(5006) << "mpeg" << endl;
            }
            break;
          }
        }
        break;
      case DwMime::kTypeModel: {
kdDebug(5006) << "* model *" << endl;
        // what the hell is "Content-Type: model/.." ?
        }
        break;
      }

      if( !bDone && reader &&
          ( reader->mAttachmentStyle != HideAttmnt || showOneMimePart) ) {
        bool asIcon = true;
        if (showOneMimePart)
        {
          asIcon = ( curNode->msgPart().contentDisposition().find("inline") < 0 );
        }
        else
        {
          switch (reader->mAttachmentStyle)
          {
            case IconicAttmnt:
              asIcon = TRUE;
              break;
            case InlineAttmnt:
              asIcon = FALSE;
              break;
            case SmartAttmnt:
              asIcon = ( curNode->msgPart().contentDisposition().find("inline") < 0 );
            case HideAttmnt: {
              // NOOP
            }
          }
        }
        if( asIcon ) {
          reader->writePartIcon(&curNode->msgPart(), curNode->nodeId());
        } else if (isImage) {
          reader->mInlineImage = true;
          reader->writePartIcon(&curNode->msgPart(), curNode->nodeId());
          reader->mInlineImage = false;
        } else {
          QCString cstr( curNode->msgPart().bodyDecoded() );
          reader->writeBodyStr(cstr,
                               reader->mCodec,
                               curNode->trueFromAddress(),
                               &isInlineSigned, &isInlineEncrypted);
        }
      }
      curNode->mWasProcessed = true;
    }
    // parse the siblings (children are parsed in the 'multipart' case terms)
    if( !showOneMimePart && curNode && curNode->mNext )
      parseObjectTree( reader,
                       &resultString,
                       cryptPlugList,
                       useThisCryptPlug,
                       curNode->mNext,
                       showOneMimePart,
                       keepEncryptions,
                       includeSignatures );
    // adjust signed/encrypted flags if inline PGP was found
    if( isInlineSigned || isInlineEncrypted ){
      if(    partNode::CryptoTypeUnknown == curNode->cryptoType()
          || partNode::CryptoTypeNone    == curNode->cryptoType() ){
        curNode->setCryptoType( partNode::CryptoTypeInlinePGP );
      }
      if( isInlineSigned )
        curNode->setSigned( true );
      if( isInlineEncrypted )
        curNode->setEncrypted( true );
    }
    if( partNode::CryptoTypeUnknown == curNode->cryptoType() )
      curNode->setCryptoType( partNode::CryptoTypeNone );
  }

  if( reader && showOneMimePart ) {
    reader->mViewer->write("</body></html>");
    reader->sendNextHtmlChunk();
    /*reader->mViewer->view()->viewport()->setUpdatesEnabled( true );
    reader->mViewer->view()->setUpdatesEnabled( true );
    reader->mViewer->view()->viewport()->repaint( false );*/
  }
  // remove temp. CryptPlugList
  if( tmpPlugList )
    delete cryptPlugList;
}


// This function returns the complete data that were in this
// message parts - *after* all encryption has been removed that
// could be removed.
// - This is used to store the message in decrypted form.
void KMReaderWin::objectTreeToDecryptedMsg( partNode* node,
                                            NewByteArray& resultingData,
                                            KMMessage& theMessage,
                                            bool weAreReplacingTheRootNode,
                                            int recCount )
{
  kdDebug(5006) << QString("-------------------------------------------------" ) << endl;
  kdDebug(5006) << QString("KMReaderWin::objectTreeToDecryptedMsg( %1 )  START").arg( recCount ) << endl;
  if( node ) {
    partNode* curNode = node;
    partNode* dataNode = curNode;
    bool bIsMultipart = false;

    switch( curNode->type() ){
      case DwMime::kTypeText: {
kdDebug(5006) << "* text *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeHtml:
kdDebug(5006) << "html" << endl;
            break;
          case DwMime::kSubtypeXVCard:
kdDebug(5006) << "v-card" << endl;
            break;
          case DwMime::kSubtypeRichtext:
kdDebug(5006) << "rich text" << endl;
            break;
          case DwMime::kSubtypeEnriched:
kdDebug(5006) << "enriched " << endl;
            break;
          case DwMime::kSubtypePlain:
kdDebug(5006) << "plain " << endl;
            break;
          default:
kdDebug(5006) << "default " << endl;
            break;
          }
        }
        break;
      case DwMime::kTypeMultipart: {
kdDebug(5006) << "* multipart *" << endl;
          bIsMultipart = true;
          switch( curNode->subType() ){
          case DwMime::kSubtypeMixed:
kdDebug(5006) << "mixed" << endl;
            break;
          case DwMime::kSubtypeAlternative:
kdDebug(5006) << "alternative" << endl;
            break;
          case DwMime::kSubtypeDigest:
kdDebug(5006) << "digest" << endl;
            break;
          case DwMime::kSubtypeParallel:
kdDebug(5006) << "parallel" << endl;
            break;
          case DwMime::kSubtypeSigned:
kdDebug(5006) << "signed" << endl;
            break;
          case DwMime::kSubtypeEncrypted: {
kdDebug(5006) << "encrypted" << endl;
              if( curNode->mChild ) {
                /*
                    ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
                */
                partNode* data =
                  curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypeOctetStream, false, true );
                if( !data ) {
                  data = curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypePkcs7Mime, false, true );
                }
                if( data && data->mChild )
                  dataNode = data;
              }
            }
            break;
          default :
kdDebug(5006) << "(  unknown subtype  )" << endl;
            break;
          }
        }
        break;
      case DwMime::kTypeMessage: {
kdDebug(5006) << "* message *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeRfc822: {
kdDebug(5006) << "RfC 822" << endl;
              if( curNode->mChild )
                dataNode = curNode->mChild;
            }
            break;
          }
        }
        break;
      case DwMime::kTypeApplication: {
kdDebug(5006) << "* application *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypePostscript:
kdDebug(5006) << "postscript" << endl;
            break;
          case DwMime::kSubtypeOctetStream: {
kdDebug(5006) << "octet stream" << endl;
              if( curNode->mChild )
                dataNode = curNode->mChild;
            }
            break;
          case DwMime::kSubtypePgpEncrypted:
kdDebug(5006) << "pgp encrypted" << endl;
            break;
          case DwMime::kSubtypePgpSignature:
kdDebug(5006) << "pgp signed" << endl;
            break;
          case DwMime::kSubtypePkcs7Mime: {
kdDebug(5006) << "pkcs7 mime" << endl;
              // note: subtype Pkcs7Mime can also be signed
              //       and we do NOT want to remove the signature!
              if( curNode->isEncrypted() && curNode->mChild )
                dataNode = curNode->mChild;
            }
            break;
          }
        }
        break;
      case DwMime::kTypeImage: {
kdDebug(5006) << "* image *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeJpeg:
kdDebug(5006) << "JPEG" << endl;
            break;
          case DwMime::kSubtypeGif:
kdDebug(5006) << "GIF" << endl;
            break;
          }
        }
        break;
      case DwMime::kTypeAudio: {
kdDebug(5006) << "* audio *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeBasic:
kdDebug(5006) << "basic" << endl;
            break;
          }
        }
        break;
      case DwMime::kTypeVideo: {
kdDebug(5006) << "* video *" << endl;
          switch( curNode->subType() ){
          case DwMime::kSubtypeMpeg:
kdDebug(5006) << "mpeg" << endl;
            break;
          }
        }
        break;
      case DwMime::kTypeModel:
kdDebug(5006) << "* model *" << endl;
        break;
    }


    DwHeaders& rootHeaders( theMessage.headers() );
    DwBodyPart * part = dataNode->dwPart() ? dataNode->dwPart() : 0;
    DwHeaders * headers(
        (part && part->hasHeaders())
        ? &part->Headers()
        : (  (weAreReplacingTheRootNode || !dataNode->mRoot)
            ? &rootHeaders
            : 0 ) );
    if( dataNode == curNode ) {
kdDebug(5006) << "dataNode == curNode:  Save curNode without replacing it." << endl;

      // A) Store the headers of this part IF curNode is not the root node
      //    AND we are not replacing a node that allready *has* replaced
      //    the root node in previous recursion steps of this function...
      if( headers ) {
        if( dataNode->mRoot && !weAreReplacingTheRootNode ) {
kdDebug(5006) << "dataNode is NOT replacing the root node:  Store the headers." << endl;
          resultingData += headers->AsString().c_str();
        } else if( weAreReplacingTheRootNode && part->hasHeaders() ){
kdDebug(5006) << "dataNode replace the root node:  Do NOT store the headers but change" << endl;
kdDebug(5006) << "                                 the Message's headers accordingly." << endl;
kdDebug(5006) << "              old Content-Type = " << rootHeaders.ContentType().AsString().c_str() << endl;
kdDebug(5006) << "              new Content-Type = " << headers->ContentType(   ).AsString().c_str() << endl;
          rootHeaders.ContentType()             = headers->ContentType();
          theMessage.setContentTransferEncodingStr(
              headers->HasContentTransferEncoding()
            ? headers->ContentTransferEncoding().AsString().c_str()
            : "" );
          rootHeaders.ContentDescription() = headers->ContentDescription();
          rootHeaders.ContentDisposition() = headers->ContentDisposition();
          theMessage.setNeedsAssembly();
        }
      }

      // B) Store the body of this part.
      if( headers && bIsMultipart && dataNode->mChild )  {
kdDebug(5006) << "is valid Multipart, processing children:" << endl;
        QCString boundary = headers->ContentType().Boundary().c_str();
        curNode = dataNode->mChild;
        // store children of multipart
        while( curNode ) {
kdDebug(5006) << "--boundary" << endl;
          if( resultingData.size() &&
              ( '\n' != resultingData.at( resultingData.size()-1 ) ) )
            resultingData += QCString( "\n" );
          resultingData += QCString( "\n" );
          resultingData += "--";
          resultingData += boundary;
          resultingData += "\n";
          // note: We are processing a harmless multipart that is *not*
          //       to be replaced by one of it's children, therefor
          //       we set their doStoreHeaders to true.
          objectTreeToDecryptedMsg( curNode,
                                    resultingData,
                                    theMessage,
                                    false,
                                    recCount + 1 );
          curNode = curNode->mNext;
        }
kdDebug(5006) << "--boundary--" << endl;
        resultingData += "\n--";
        resultingData += boundary;
        resultingData += "--\n\n";
kdDebug(5006) << "Multipart processing children - DONE" << endl;
      } else if( part ){
        // store simple part
kdDebug(5006) << "is Simple part or invalid Multipart, storing body data .. DONE" << endl;
        resultingData += part->Body().AsString().c_str();
      }
    } else {
kdDebug(5006) << "dataNode != curNode:  Replace curNode by dataNode." << endl;
      bool rootNodeReplaceFlag = weAreReplacingTheRootNode || !curNode->mRoot;
      if( rootNodeReplaceFlag ) {
kdDebug(5006) << "                      Root node will be replaced." << endl;
      } else {
kdDebug(5006) << "                      Root node will NOT be replaced." << endl;
      }
      // store special data to replace the current part
      // (e.g. decrypted data or embedded RfC 822 data)
      objectTreeToDecryptedMsg( dataNode,
                                resultingData,
                                theMessage,
                                rootNodeReplaceFlag,
                                recCount + 1 );
    }
  }
  kdDebug(5006) << QString("\nKMReaderWin::objectTreeToDecryptedMsg( %1 )  END").arg( recCount ) << endl;
}


/*
 ===========================================================================


        E N D    O F     T E M P O R A R Y     M I M E     C O D E


 ===========================================================================
*/













const int KMReaderWin::delay = 150;

//-----------------------------------------------------------------------------
KMReaderWin::KMReaderWin(CryptPlugWrapperList *cryptPlugList,
                         KMMimePartTree* mimePartTree,
                         int* showMIMETreeMode,
                         QWidget *aParent,
                         const char *aName,
                         int aFlags)
  : KMReaderWinInherited(aParent, aName, aFlags | Qt::WDestructiveClose),
    mShowCompleteMessage( false ),
    mMimePartTree( mimePartTree ),
    mShowMIMETreeMode( showMIMETreeMode ),
    mCryptPlugList( cryptPlugList ),
    mRootNode( 0 ),
    mIdOfLastViewedMessage()
{
  mAutoDelete = false;
  mLastSerNum = 0;
  mMessage = 0;
  mLastStatus = KMMsgStatusUnknown;
  mMsgDisplay = true;
  mPrinting = false;
  mShowColorbar = false;
  mInlineImage = false;
  mIsFirstTextPart = true;

  if (!aParent)
     mStandaloneWindows.append(this);

  initHtmlWidget();
  readConfig();
  mHtmlOverride = false;
  mUseFixedFont = false;

  connect( &updateReaderWinTimer, SIGNAL(timeout()),
  	   this, SLOT(updateReaderWin()) );
  connect( &mResizeTimer, SIGNAL(timeout()),
  	   this, SLOT(slotDelayedResize()) );
  connect( &mHtmlTimer, SIGNAL(timeout()),
           this, SLOT(sendNextHtmlChunk()) );
  connect( &mDelayedMarkTimer, SIGNAL(timeout()),
           this, SLOT(slotTouchMessage()) );

  mCodec = 0;
  mAutoDetectEncoding = true;
}


//-----------------------------------------------------------------------------
KMReaderWin::~KMReaderWin()
{
  mStandaloneWindows.removeRef(this);
  delete mViewer;  //hack to prevent segfault on exit
  if (mAutoDelete) delete message();
  if (mRootNode) delete mRootNode;
  removeTempFiles();
}


//-----------------------------------------------------------------------------
void KMReaderWin::setMimePartTree( KMMimePartTree* mimePartTree )
{
  mMimePartTree = mimePartTree;
}
//-----------------------------------------------------------------------------
void KMReaderWin::removeTempFiles()
{
  for (QStringList::Iterator it = mTempFiles.begin(); it != mTempFiles.end();
    it++)
  {
    QFile::remove(*it);
  }
  mTempFiles.clear();
  for (QStringList::Iterator it = mTempDirs.begin(); it != mTempDirs.end();
    it++)
  {
    QDir(*it).rmdir(*it);
  }
  mTempDirs.clear();
}


//-----------------------------------------------------------------------------
bool KMReaderWin::event(QEvent *e)
{
  if (e->type() == QEvent::ApplicationPaletteChange)
  {
     readColorConfig();
     if (message())
	 message()->readConfig();
     update( true ); // Force update
     return true;
  }
  return KMReaderWinInherited::event(e);
}



//-----------------------------------------------------------------------------
void KMReaderWin::readColorConfig(void)
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "Reader");

  c1 = QColor(kapp->palette().active().text());
  c2 = KGlobalSettings::linkColor();
  c3 = KGlobalSettings::visitedLinkColor();
  c4 = QColor(kapp->palette().active().base());

  // The default colors are also defined in configuredialog.cpp
  cPgpEncrH = QColor( 0x00, 0x80, 0xFF ); // light blue
  cPgpOk1H  = QColor( 0x40, 0xFF, 0x40 ); // light green
  cPgpOk0H  = QColor( 0xFF, 0xFF, 0x40 ); // light yellow
  cPgpWarnH = QColor( 0xFF, 0xFF, 0x40 ); // light yellow
  cPgpErrH  = QColor( 0xFF, 0x00, 0x00 ); // red
  cCBpgp   = QColor( 0x80, 0xFF, 0x80 ); // very light green
  cCBplain = QColor( 0xFF, 0xFF, 0x80 ); // very light yellow
  cCBhtml  = QColor( 0xFF, 0x40, 0x40 ); // light red

  if (!config->readBoolEntry("defaultColors",TRUE)) {
    c1 = config->readColorEntry("ForegroundColor",&c1);
    c2 = config->readColorEntry("LinkColor",&c2);
    c3 = config->readColorEntry("FollowedColor",&c3);
    c4 = config->readColorEntry("BackgroundColor",&c4);
    cPgpEncrH = config->readColorEntry( "PGPMessageEncr", &cPgpEncrH );
    cPgpOk1H  = config->readColorEntry( "PGPMessageOkKeyOk", &cPgpOk1H );
    cPgpOk0H  = config->readColorEntry( "PGPMessageOkKeyBad", &cPgpOk0H );
    cPgpWarnH = config->readColorEntry( "PGPMessageWarn", &cPgpWarnH );
    cPgpErrH  = config->readColorEntry( "PGPMessageErr", &cPgpErrH );
    cCBpgp   = config->readColorEntry( "ColorbarPGP", &cCBpgp );
    cCBplain = config->readColorEntry( "ColorbarPlain", &cCBplain );
    cCBhtml  = config->readColorEntry( "ColorbarHTML", &cCBhtml );
  }

  // determine the frame and body color for PGP messages from the header color
  // if the header color equals the background color then the other colors are
  // also set to the background color (-> old style PGP message viewing)
  // else
  // the brightness of the frame is set to 4/5 of the brightness of the header
  // the saturation of the body is set to 1/8 of the saturation of the header
  int h,s,v;
  if ( cPgpOk1H == c4 )
  { // header color == background color?
    cPgpOk1F = c4;
    cPgpOk1B = c4;
  }
  else
  {
    cPgpOk1H.hsv( &h, &s, &v );
    cPgpOk1F.setHsv( h, s, v*4/5 );
    cPgpOk1B.setHsv( h, s/8, v );
  }
  if ( cPgpOk0H == c4 )
  { // header color == background color?
    cPgpOk0F = c4;
    cPgpOk0B = c4;
  }
  else
  {
    cPgpOk0H.hsv( &h, &s, &v );
    cPgpOk0F.setHsv( h, s, v*4/5 );
    cPgpOk0B.setHsv( h, s/8, v );
  }
  if ( cPgpWarnH == c4 )
  { // header color == background color?
    cPgpWarnF = c4;
    cPgpWarnB = c4;
  }
  else
  {
    cPgpWarnH.hsv( &h, &s, &v );
    cPgpWarnF.setHsv( h, s, v*4/5 );
    cPgpWarnB.setHsv( h, s/8, v );
  }
  if ( cPgpErrH == c4 )
  { // header color == background color?
    cPgpErrF = c4;
    cPgpErrB = c4;
  }
  else
  {
    cPgpErrH.hsv( &h, &s, &v );
    cPgpErrF.setHsv( h, s, v*4/5 );
    cPgpErrB.setHsv( h, s/8, v );
  }

  if ( cPgpEncrH == c4 )
  { // header color == background color?
    cPgpEncrF = c4;
    cPgpEncrB = c4;
  }
  else
  {
    cPgpEncrH.hsv( &h, &s, &v );
    cPgpEncrF.setHsv( h, s, v*4/5 );
    cPgpEncrB.setHsv( h, s/8, v );
  }

  mRecyleQouteColors = config->readBoolEntry( "RecycleQuoteColors", false );

  //
  // Prepare the quoted fonts
  //
  mQuoteFontTag[0] = quoteFontTag(0);
  mQuoteFontTag[1] = quoteFontTag(1);
  mQuoteFontTag[2] = quoteFontTag(2);
}

//-----------------------------------------------------------------------------
void KMReaderWin::readConfig(void)
{
  KConfig *config = kapp->config();
  QString encoding;

  { // block defines the lifetime of KConfigGroupSaver
  KConfigGroupSaver saver(config, "Pixmaps");
  mBackingPixmapOn = FALSE;
  mBackingPixmapStr = config->readEntry("Readerwin","");
  if (mBackingPixmapStr != "")
    mBackingPixmapOn = TRUE;
  }

  {
  KConfigGroupSaver saver(config, "Reader");
  mHtmlMail = config->readBoolEntry( "htmlMail", false );
  mAtmInline = config->readNumEntry("attach-inline", 100);
  mHeaderStyle = (HeaderStyle)config->readNumEntry("hdr-style", HdrFancy);
  mAttachmentStyle = (AttachmentStyle)config->readNumEntry("attmnt-style",
							   SmartAttmnt);
  mLoadExternal = config->readBoolEntry( "htmlLoadExternal", false );
  mViewer->setOnlyLocalReferences( !mLoadExternal );

  // if the user uses OpenPGP then the color bar defaults to enabled
  // else it defaults to disabled
  if( Kpgp::Module::getKpgp()->usePGP() )
    mShowColorbar = true;
  else
    mShowColorbar = false;
  mShowColorbar = config->readBoolEntry( "showColorbar", mShowColorbar );
  // if the value defaults to enabled and KMail (with color bar) is used for
  // the first time the config dialog doesn't know this if we don't save the
  // value now
  config->writeEntry( "showColorbar", mShowColorbar );
  }

  {
  KConfigGroupSaver saver(config, "Fonts");
  mUnicodeFont = config->readBoolEntry("unicodeFont",FALSE);
  mBodyFont = KGlobalSettings::generalFont();
  mFixedFont = KGlobalSettings::fixedFont();
  if (!config->readBoolEntry("defaultFonts",TRUE)) {
    mBodyFont = config->readFontEntry((mPrinting) ? "print-font" : "body-font",
      &mBodyFont);
    mFixedFont = config->readFontEntry("fixed-font", &mFixedFont);
    fntSize = mBodyFont.pointSize();
    mBodyFamily = mBodyFont.family();
  }
  else {
    setFont(KGlobalSettings::generalFont());
    fntSize = KGlobalSettings::generalFont().pointSize();
    mBodyFamily = KGlobalSettings::generalFont().family();
  }
  mViewer->setStandardFont(mBodyFamily);
  }

  {
    KConfigGroup behaviour( kapp->config(), "Behaviour" );
    mDelayedMarkAsRead = behaviour.readBoolEntry( "DelayedMarkAsRead", true );
    mDelayedMarkTimeout = behaviour.readNumEntry( "DelayedMarkTime", 0 );
  }

  readColorConfig();

  if (message()) {
    update();
    message()->readConfig();
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeConfig(bool aWithSync)
{
  KConfig *config = kapp->config();
  KConfigGroupSaver saver(config, "Reader");
  config->writeEntry("attach-inline", mAtmInline);
  config->writeEntry("hdr-style", (int)mHeaderStyle);
  config->writeEntry("attmnt-style",(int)mAttachmentStyle);
  if (aWithSync) config->sync();
}


//-----------------------------------------------------------------------------
QString KMReaderWin::quoteFontTag( int quoteLevel )
{
  KConfig *config = kapp->config();

  QColor color;

  { // block defines the lifetime of KConfigGroupSaver
    KConfigGroupSaver saver(config, "Reader");
    if( config->readBoolEntry( "defaultColors", true ) == true )
    {
      color = QColor(kapp->palette().active().text());
    }
    else
    {
      if( quoteLevel == 0 ) {
	QColor defaultColor( 0x00, 0x80, 0x00 );
	color = config->readColorEntry( "QuotedText1", &defaultColor );
      } else if( quoteLevel == 1 ) {
	QColor defaultColor( 0x00, 0x70, 0x00 );
	color = config->readColorEntry( "QuotedText2", &defaultColor );
      } else if( quoteLevel == 2 ) {
	QColor defaultColor( 0x00, 0x60, 0x00 );
	color = config->readColorEntry( "QuotedText3", &defaultColor );
      } else
	color = QColor(kapp->palette().active().base());
    }
  }

  QFont font;
  {
    KConfigGroupSaver saver(config, "Fonts");
    if( config->readBoolEntry( "defaultFonts", true ) == true )
    {
      font = KGlobalSettings::generalFont();
      font.setItalic(true);
    }
    else
    {
      const QFont defaultFont = QFont("helvetica");
      if( quoteLevel == 0 )
	font  = config->readFontEntry( "quote1-font", &defaultFont );
      else if( quoteLevel == 1 )
	font  = config->readFontEntry( "quote2-font", &defaultFont );
      else if( quoteLevel == 2 )
	font  = config->readFontEntry( "quote3-font", &defaultFont );
      else
      {
	font = KGlobalSettings::generalFont();
	font.setItalic(true);
      }
    }
  }

  QString style;
  if( mPrinting )
    style = "color:#000000;";
  else
    style = QString( "color:%1;" ).arg( color.name() );
  if( font.italic() )
    style += "font-style:italic;";
  if( font.bold() )
    style += "font-weight:bold;";

  return QString( "<div style=\"%1\">" ).arg( style );
}


//-----------------------------------------------------------------------------
void KMReaderWin::initHtmlWidget(void)
{
  mBox = new QHBox(this);

  mColorBar = new QLabel(" ", mBox);
  mColorBar->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
  mColorBar->setEraseColor( mPrinting ? QColor( "white" ) : c4 );

  if ( !mShowColorbar )
    mColorBar->hide();


  mViewer = new KHTMLPart(mBox, "khtml");
  mViewer->widget()->setFocusPolicy(WheelFocus);
  // Let's better be paranoid and disable plugins (it defaults to enabled):
  mViewer->setPluginsEnabled(false);
  mViewer->setJScriptEnabled(false); // just make this explicit
  mViewer->setJavaEnabled(false);    // just make this explicit
  mViewer->setMetaRefreshEnabled(false);
  mViewer->setURLCursor(KCursor::handCursor());

  // Espen 2000-05-14: Getting rid of thick ugly frames
  mViewer->view()->setLineWidth(0);

  connect(mViewer->browserExtension(),
          SIGNAL(openURLRequest(const KURL &, const KParts::URLArgs &)),this,
          SLOT(slotUrlOpen(const KURL &, const KParts::URLArgs &)));
  connect(mViewer->browserExtension(),
          SIGNAL(createNewWindow(const KURL &, const KParts::URLArgs &)),this,
          SLOT(slotUrlOpen(const KURL &, const KParts::URLArgs &)));
  connect(mViewer,SIGNAL(onURL(const QString &)),this,
          SLOT(slotUrlOn(const QString &)));
  connect(mViewer,SIGNAL(popupMenu(const QString &, const QPoint &)),
          SLOT(slotUrlPopup(const QString &, const QPoint &)));
}


//-----------------------------------------------------------------------------
void KMReaderWin::setHeaderStyle(KMReaderWin::HeaderStyle aHeaderStyle)
{
  mHeaderStyle = aHeaderStyle;
  update(true);
  writeConfig(true);   // added this so we can forward w/ full headers
}


//-----------------------------------------------------------------------------
void KMReaderWin::setAttachmentStyle(int aAttachmentStyle)
{
  mAttachmentStyle = (AttachmentStyle)aAttachmentStyle;
  update(true);
}

//-----------------------------------------------------------------------------
void KMReaderWin::setCodec(QTextCodec *codec)
{
  mCodec = codec;
  if(!codec) {
    mAutoDetectEncoding = true;
    update(true);
    return;
  }
  mAutoDetectEncoding = false;
  update(true);
}

//-----------------------------------------------------------------------------
void KMReaderWin::setInlineAttach(int aAtmInline)
{
  mAtmInline = aAtmInline;
  update(true);
}


//-----------------------------------------------------------------------------
void KMReaderWin::setMsg(KMMessage* aMsg, bool force)
{
  if (aMsg)
      kdDebug(5006) << "(" << aMsg->getMsgSerNum() << ") " << aMsg->subject() << " " << aMsg->fromStrip() << endl;

  // If not forced and there is aMsg and aMsg is same as mMsg then return
  if (!force && aMsg && mLastSerNum != 0 && aMsg->getMsgSerNum() == mLastSerNum)
    return;

  kdDebug(5006) << "set Msg, force = " << force << endl;

  // connect to the updates if we have hancy headers

  mDelayedMarkTimer.stop();

  mLastSerNum = (aMsg) ? aMsg->getMsgSerNum() : 0;

  // assume if a serial number exists it can be used to find the assoc KMMessage
  if (!mLastSerNum)
    mMessage = aMsg;
  else
    mMessage = 0;
  if (message() != aMsg) {
    mMessage = aMsg;
    mLastSerNum = 0; // serial number was invalid
    Q_ASSERT(0);
  }

  mLastStatus = (aMsg) ? aMsg->status() : KMMsgStatusUnknown;
  if (aMsg)
  {
    aMsg->setCodec(mCodec);
    aMsg->setDecodeHTML(htmlMail());
  }

  // Avoid flicker, somewhat of a cludge
  if (force) {
    // stop the timer to avoid calling updateReaderWin twice
    updateReaderWinTimer.stop();
    updateReaderWin();
  }
  else if (updateReaderWinTimer.isActive())
    updateReaderWinTimer.changeInterval( delay );
  else
    updateReaderWinTimer.start( 0, TRUE );

  if (mDelayedMarkAsRead) {
    if ( mDelayedMarkTimeout == 0 )
    	slotTouchMessage();
    else
        mDelayedMarkTimer.start( mDelayedMarkTimeout * 1000, TRUE );
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::clearCache()
{
  updateReaderWinTimer.stop();
  mDelayedMarkTimer.stop();
  mLastSerNum = 0;
  mMessage = 0;
}

// enter items for the "new features" list here, so the main body of
// the welcome page can be left untouched (probably much easier for
// the translators). Note that the <li>...</li> tags are added
// automatically below:
static const char * const kmailNewFeatures[] = {
  I18N_NOOP("PGP/MIME (RFC 3156) support for GnuPG users"),
  I18N_NOOP("S/MIME support"),
  I18N_NOOP("Optional MIME tree viewer, allowing direct access to all "
	    "body parts (attachments)"),
  I18N_NOOP("Optional total/unread columns in the folder view"),
  I18N_NOOP("Custom folder icons"),
  I18N_NOOP("Custom date format"),
  I18N_NOOP("Reorganized menu bar looks more like other KDE applications"),
  I18N_NOOP("Default identity can now be renamed"),
  I18N_NOOP("Multiple OpenPGP keys per email address (useful for distribution lists)"),
};
static const int numKMailNewFeatures =
  sizeof kmailNewFeatures / sizeof *kmailNewFeatures;


//-----------------------------------------------------------------------------
void KMReaderWin::displayAboutPage()
{
  mColorBar->hide();
  mMsgDisplay = FALSE;
  QString location = locate("data", "kmail/about/main.html");
  QString content = kFileToString(location);
  mViewer->begin(location);
  QString info =
    i18n("%1: KMail version; %2: help:// URL; %3: homepage URL; "
	 "%4: prior KMail version; %5: prior KDE version; "
	 "%6: generated list of new features; "
	 "%7: First-time user text (only shown on first start)"
	 "--- end of comment ---",
	 "<h2>Welcome to KMail %1</h2><p>KMail is the email client for the K "
	 "Desktop Environment. It is designed to be fully compatible with "
	 "Internet mailing standards including MIME, SMTP, POP3 and IMAP."
	 "</p>\n"
	 "<ul><li>KMail has many powerful features which are described in the "
	 "<a href=\"%2\">documentation</a></li>\n"
	 "<li>The <a href=\"%3\">KMail homepage</A> offers information about "
	 "new versions of KMail</li></ul>\n"
	 "<p>Some of the new features in this release of KMail include "
	 "(compared to KMail %4, which is part of KDE %5):</p>\n"
	 "<ul>\n%6</ul>\n"
	 "%7\n"
	 "<p>We hope that you will enjoy KMail.</p>\n"
	 "<p>Thank you,</p>\n"
	 "<p>&nbsp; &nbsp; The KMail Team</p>")
    .arg(KMAIL_VERSION) // KMail version
    .arg("help:/kmail/index.html") // KMail help:// URL
    .arg("http://kmail.kde.org/") // KMail homepage URL
    .arg("1.4").arg("3.0"); // prior KMail and KDE version

  QString featureItems;
  for ( int i = 0 ; i < numKMailNewFeatures ; i++ )
    featureItems += i18n("<li>%1</li>\n").arg( i18n( kmailNewFeatures[i] ) );

  info = info.arg( featureItems );

  if( kernel->firstStart() ) {
    info = info.arg( i18n("<p>Please take a moment to fill in the KMail "
			  "configuration panel at Settings-&gt;Configure "
			  "KMail.\n"
			  "You need to create at least a default identity and "
			  "an incoming as well as outgoing mail account."
			  "</p>\n") );
  } else {
    info = info.arg( QString::null );
  }
  mViewer->write(content.arg(pointsToPixel(fntSize)).arg(info));
  mViewer->end();
}


//-----------------------------------------------------------------------------
void KMReaderWin::updateReaderWin()
{
  if (!mMsgDisplay) return;

  mViewer->view()->setUpdatesEnabled( false );
  mViewer->view()->viewport()->setUpdatesEnabled( false );
  static_cast<QScrollView *>(mViewer->widget())->ensureVisible(0,0);

  if (mHtmlTimer.isActive())
  {
    mHtmlTimer.stop();
    mViewer->end();
  }
  mHtmlQueue.clear();

  if (message())
  {
    if ( mShowColorbar )
      mColorBar->show();
    else
      mColorBar->hide();
    parseMsg();
  }
  else
  {
    mColorBar->hide();
    mViewer->begin( KURL( "file:/" ) );
    mViewer->write("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 "
		   "Transitional//EN\">\n<html><body" +
		   QString(" bgcolor=\"%1\"").arg(c4.name()));

    if (mBackingPixmapOn)
      mViewer->write(" background=\"file://" + mBackingPixmapStr + "\"");
    mViewer->write("></body></html>");
    mViewer->end();
    mViewer->view()->viewport()->setUpdatesEnabled( true );
    mViewer->view()->setUpdatesEnabled( true );
    mViewer->view()->viewport()->repaint( false );
    if( mMimePartTree )
      mMimePartTree->clear();
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::queueHtml(const QString &aStr)
{
  uint pos = 0;
  while (aStr.length() > pos)
  {
    mHtmlQueue += aStr.mid(pos, 16384);
    pos += 16384;
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::sendNextHtmlChunk()
{
  QStringList::Iterator it = mHtmlQueue.begin();
  if (it == mHtmlQueue.end())
  {
    mViewer->end();
    mViewer->view()->viewport()->setUpdatesEnabled( true );
    mViewer->view()->setUpdatesEnabled( true );
    mViewer->view()->viewport()->repaint( false );
    return;
  }
  mViewer->write(*it);
  mHtmlQueue.remove(it);
  mHtmlTimer.start(0, TRUE);
}

//-----------------------------------------------------------------------------
int KMReaderWin::pointsToPixel(int pointSize) const
{
  QPaintDeviceMetrics const pdm(mViewer->view());

  return (pointSize * pdm.logicalDpiY() + 36) / 72;
}

//-----------------------------------------------------------------------------
void KMReaderWin::showHideMimeTree( bool showIt )
{
  if( mMimePartTree && ( !mShowMIMETreeMode || (0 != *mShowMIMETreeMode) ) ){
    if( showIt )
      mMimePartTree->show();
    else
      mMimePartTree->hide();
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(void)
{
  KMMessage *msg = message();
  if ( msg == NULL )
    return;

  if( mMimePartTree )
    mMimePartTree->clear();

  int mainType = msg->type();
  bool isMultipart = ( DwMime::kTypeMultipart == mainType );

  showHideMimeTree( isMultipart );

  QString bkgrdStr = "";
  if (mBackingPixmapOn)
    bkgrdStr = " background=\"file://" + mBackingPixmapStr + "\"";

  mViewer->begin( KURL( "file:/" ) );

  if (mAutoDetectEncoding) {
    mCodec = 0;
    QCString encoding;
    if( DwMime::kTypeText == mainType )
      encoding = msg->charset();
    else if ( isMultipart ) {
      if (msg->numBodyParts() > 0) {
        KMMessagePart msgPart;
        msg->bodyPart(0, &msgPart);
        encoding = msgPart.charset();
      }
    }
    if (encoding.isEmpty())
      encoding = kernel->networkCodec()->name();
    mCodec = KMMsgBase::codecForName(encoding);
  }

  if (!mCodec)
    mCodec = QTextCodec::codecForName("iso8859-1");
  msg->setCodec(mCodec);


//      QString( "table.rfc822 { width: 100%; "
//                 "border-style:solid; border-width: 8px; }\n" )

/*

#links {
  border-left-width:1cm;
  border-left-style:solid;
  border-color:red;
  padding-left:1cm;
  text-align:justify; }
#linksrechts {
  border-left-width:1cm;
  border-left-style:solid;
  border-left-color:red;
  padding-left:1cm;
  border-right-width:1cm;
  border-right-style:solid;
  border-right-color:green;
  padding-right:1cm;
  text-align:justify; }
#rundrum {
  border-width:1px;
  border-style:solid;
  border-color:blue;
  padding:1cm;
  text-align:justify; }

*/


  QColorGroup cg = kapp->palette().active();
  queueHtml("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 "
	    "Transitional//EN\">\n<html><head><title></title>"
	    "<style type=\"text/css\">" +
    ((mPrinting) ? QString("body { font-family: \"%1\"; font-size: %2pt; "
                           "color: #000000; background-color: #FFFFFF; }\n")
        .arg( mBodyFamily ).arg( fntSize )
      : QString("body { font-family: \"%1\"; font-size: %2px; "
        "color: %3; background-color: %4; }\n")
        .arg( mBodyFamily ).arg( pointsToPixel(fntSize) )
        .arg( mPrinting ? "#000000" : c1.name() )
        .arg( mPrinting ? "#FFFFFF" : c4.name() ) ) +
    ((mPrinting) ? QString("a { color: #000000; text-decoration: none; }")
      : QString("a { color: %1; ").arg(c2.name()) +
        "text-decoration: none; }" + // just playing

        QString( "table.encr { width: 100%; background-color: %1; "
                 "border-width: 0px; }\n" )
        .arg( cPgpEncrF.name() ) +
        QString( "tr.encrH { background-color: %1; "
                 "font-weight: bold; }\n" )
        .arg( cPgpEncrH.name() ) +
        QString( "tr.encrB { background-color: %1; }\n" )
        .arg( cPgpEncrB.name() ) +

        QString( "table.rfc822 { width: 100%; "
                 "border-top-style: solid; "
                 "border-top-width: 1px; "
                 "border-top-color: black; "
                 "border-left-style: solid; "
                 "border-left-width: 1px; "
                 "border-left-color: black; "
                 "border-bottom-style: solid; "
                 "border-bottom-width: 1px; "
                 "border-bottom-color: black; "
                 "border-right-style: hidden; "
                 "border-right-width: 0px; "
                 "padding: 2px; } \n" ) +
        QString( "tr.rfc822H { font-weight: bold; }\n" ) +
        QString( "tr.rfc822B { font-weight: normal; }\n" ) +


        QString( "table.signOkKeyOk { width: 100%; background-color: %1; "
                 "border-width: 0px; }\n" )
        .arg( cPgpOk1F.name() ) +
        QString( "tr.signOkKeyOkH { background-color: %1; "
                 "font-weight: bold; }\n" )
        .arg( cPgpOk1H.name() ) +
        QString( "tr.signOkKeyOkB { background-color: %1; }\n" )
        .arg( cPgpOk1B.name() ) +

        QString( "table.signOkKeyBad { width: 100%; background-color: %1; "
                 "border-width: 0px; }\n" )
        .arg( cPgpOk0F.name() ) +
        QString( "tr.signOkKeyBadH { background-color: %1; "
                 "font-weight: bold; }\n" )
        .arg( cPgpOk0H.name() ) +
        QString( "tr.signOkKeyBadB { background-color: %1; }\n" )
        .arg( cPgpOk0B.name() ) +

        QString( "table.signWarn { width: 100%; background-color: %1; "
                 "border-width: 0px; }\n" )
        .arg( cPgpWarnF.name() ) +
        QString( "tr.signWarnH { background-color: %1; "
                 "font-weight: bold; }\n" )
        .arg( cPgpWarnH.name() ) +
        QString( "tr.signWarnB { background-color: %1; }\n" )
        .arg( cPgpWarnB.name() ) +

        QString( "table.signErr { width: 100%; background-color: %1; "
                 "border-width: 0px; }\n" )
        .arg( cPgpErrF.name() ) +
        QString( "tr.signErrH { background-color: %1; "
                 "font-weight: bold; }\n" )
        .arg( cPgpErrH.name() ) +
        QString( "tr.signErrB { background-color: %1; }\n" )
        .arg( cPgpErrB.name() )) +

        QString( "div.fancyHeaderSubj { background-color: %1; "
                                       "color: %2; padding: 4px; "
                                       "border: solid %3 1px; }\n"
		 "div.fancyHeaderSubj a[href] { color: %2; }"
		 "div.fancyHeaderSubj a[href]:hover { text-decoration: underline; }\n")
        .arg((mPrinting) ? cg.background().name() : cg.highlight().name())
        .arg((mPrinting) ? cg.foreground().name() : cg.highlightedText().name())
        .arg((mPrinting) ? cg.foreground().name() : cg.highlightedText().name())
        .arg(cg.foreground().name()) +
        QString( "div.fancyHeaderDtls { background-color: %1; color: %2; "
                                       "border-bottom: solid %3 1px; "
                                       "border-left: solid %4 1px; "
                                       "border-right: solid %5 1px; "
                                       "margin-bottom: 1em; "
                                       "padding: 2px; }\n" )
         .arg(cg.background().name())
         .arg(cg.foreground().name())
         .arg(cg.foreground().name())
         .arg(cg.foreground().name())
         .arg(cg.foreground().name()) +
         QString( "table.fancyHeaderDtls { width: 100%; "
                                          "border-width: 0px; "
                                          "align: left }\n"
                  "th.fancyHeaderDtls { padding: 0px; "
                                       "border-spacing: 0px; "
                                       "text-align: left; "
                                       "vertical-align: top; }\n"
                  "td.fancyHeaderDtls { padding: 0px; "
                                       "border-spacing: 0px; "
                                       "text-align: left; "
                                       "text-valign: top; "
                                       "width: 100%; }\n" ) +
         "</style></head>" +
		 // TODO: move these to stylesheet, too:
    ((mPrinting) ? QString("<body>") : QString("<body ") + bkgrdStr + ">" ));

  if (!parent())
    setCaption(msg->subject());

  parseMsg(msg);

  queueHtml("</body></html>");
  sendNextHtmlChunk();
}


void KMReaderWin::showMessageAndSetData( const QString& txt0,
                                         const QString& txt1,
                                         const QString& txt2a,
                                         const QString& txt2b,
                                         QCString& data )
{
  data  = "<hr><b><h2>";
  data += txt0.utf8();
  data += "</h2></b><br><b>";
  data += i18n("reason:").utf8();
  data += "</b><br><i>&nbsp; &nbsp; ";
  data += txt1.utf8();
  data += "</i><br><b>";
  data += i18n("proposal:").utf8();
  data += "</b><br><i>&nbsp; &nbsp; ";
  data += txt2a.utf8();
  data += "<br>&nbsp; &nbsp; ";
  data += txt2b.utf8();
  data += "</i>";
  KMessageBox::sorry(this, txt0+"\n\n"+txt1+"\n\n"+txt2a+"\n"+txt2b);
}


bool KMReaderWin::writeOpaqueOrMultipartSignedData( KMReaderWin* reader,
                                                    QCString* resultString,
                                                    CryptPlugWrapperList* cryptPlugList,
                                                    CryptPlugWrapper*     useThisCryptPlug,
                                                    partNode* data,
                                                    partNode& sign,
                                                    const QString& fromAddress,
                                                    bool hideErrors )
{
  bool bIsOpaqueSigned = false;

  CryptPlugWrapper* cryptPlug = useThisCryptPlug ? useThisCryptPlug : cryptPlugList->active();
  if( cryptPlug ) {
    if( data )
      kdDebug(5006) << "\nKMReaderWin::writeOpaqueOrMultipartSignedData: processing SMIME Signed data" << endl;
    else
      kdDebug(5006) << "\nKMReaderWin::writeOpaqueOrMultipartSignedData: processing Opaque Signed data" << endl;

    kdDebug(5006) << "\nKMReaderWin::writeOpaqueOrMultipartSignedData: going to call CRYPTPLUG "
                  << cryptPlug->libName() << endl;

    if( !cryptPlug->initStatus( 0 ) == CryptPlugWrapper::InitStatus_Ok ) {
        if( reader && !hideErrors )
          KMessageBox::sorry(reader, i18n("Crypto Plug-In \"%1\" is not initialized.\n"
            "Please specify the Plug-In using the 'Settings/Configure KMail / Plug-In' dialog.").arg(cryptPlug->libName()));
        return false;
    }

    QCString cleartext;
    char* new_cleartext;
    if( data )
      cleartext = data->dwPart()->AsString().c_str();
    else
      new_cleartext = 0;
    
// #define KHZ_TEST
#ifdef KHZ_TEST
    QFile fileD0( "testdat_xx-0" );
    if( fileD0.open( IO_WriteOnly ) ) {
        if( data ) {
          QDataStream ds( &fileD0 );
          ds.writeRawBytes( cleartext, cleartext.length() );
        }
        fileD0.close();  // If data is 0 we just create a zero length file.
    }
#endif
    if( data &&
        ( (0 <= cryptPlug->libName().find( "smime",   0, false )) ||
          (0 <= cryptPlug->libName().find( "openpgp", 0, false )) ) ) {
      // replace simple LFs by CRLSs
      // according to RfC 2633, 3.1.1 Canonicalization
      int posLF = cleartext.find( '\n' );
      if(    ( 0 < posLF )
          && ( '\r'  != cleartext[posLF - 1] ) ) {
        kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
        cleartext = KMMessage::lf2crlf( cleartext );
        kdDebug(5006) << "                                                       done." << endl;
      }
    }
// #define KHZ_TEST
#ifdef KHZ_TEST
    QFile fileD( "testdat_xx1" );
    if( fileD.open( IO_WriteOnly ) ) {
        if( data ) {
          QDataStream ds( &fileD );
          ds.writeRawBytes( cleartext, cleartext.length() );
        }
        fileD.close();  // If data is 0 we just create a zero length file.
    }
#endif

    QByteArray signaturetext( sign.msgPart().bodyDecodedBinary() );
    QCString signatureStr( signaturetext );
    bool signatureIsBinary = (-1 == signatureStr.find("BEGIN SIGNED MESSAGE", 0, false) ) &&
                             (-1 == signatureStr.find("BEGIN PGP SIGNED MESSAGE", 0, false) ) &&
                             (-1 == signatureStr.find("BEGIN PGP MESSAGE", 0, false) );
    int signatureLen = signaturetext.size();
#ifdef KHZ_TEST
    QFile fileS( "testdat_xx1.sig" );
    if( fileS.open( IO_WriteOnly ) ) {
        QDataStream ds( &fileS );
        ds.writeRawBytes( signaturetext, signaturetext.size() );
        fileS.close();
    }
#endif

    QCString deb;
    deb =  "\n\nS I G N A T U R E = ";
    if( signatureIsBinary )
      deb += "[binary data]";
    else {
      deb += "\"";
      deb += signaturetext;
      deb += "\"";
    }
    deb += "\n\nC O N T E N T = \"";
    deb += cleartext;
    deb += "\"  <--  E N D    O F    C O N T E N T\n\n";
    kdDebug(5006) << deb << endl;


    struct CryptPlugWrapper::SignatureMetaData sigMeta;
    sigMeta.status              = 0;
    sigMeta.extended_info       = 0;
    sigMeta.extended_info_count = 0;
    sigMeta.nota_xml            = 0;

    const char* cleartextP = cleartext;
    PartMetaData messagePart;
    messagePart.isSigned = true;
    messagePart.isGoodSignature = false;
    messagePart.isEncrypted = false;
    messagePart.isDecryptable = false;
    messagePart.keyTrust = Kpgp::KPGP_VALIDITY_UNKNOWN;
    messagePart.status = i18n("Wrong Crypto Plug-In!");

    if( cryptPlug->hasFeature( Feature_VerifySignatures ) ) {

      if( cryptPlug->checkMessageSignature(
                                data ? const_cast<char**>(&cleartextP)
                                    : &new_cleartext,
                                signaturetext,
                                signatureIsBinary,
                                signatureLen,
                                &sigMeta ) ) {
        messagePart.isGoodSignature = true;
      }

      kdDebug(5006) << "\nKMReaderWin::writeOpaqueOrMultipartSignedData: returned from CRYPTPLUG" << endl;

      if( sigMeta.status && *sigMeta.status )
        messagePart.status = QString::fromUtf8( sigMeta.status );
      messagePart.status_code = sigMeta.status_code;

      // only one signature supported
      if( sigMeta.extended_info_count != 0 ) {

        kdDebug(5006) << "\nKMReaderWin::writeOpaqueOrMultipartSignedData: found extended sigMeta info" << endl;

        CryptPlugWrapper::SignatureMetaDataExtendedInfo& ext = sigMeta.extended_info[0];

        // save extended signature status flags
        messagePart.sigStatusFlags = ext.sigStatusFlags;

        if( messagePart.status.isEmpty()
            && ext.status_text
            && *ext.status_text )
          messagePart.status = QString::fromUtf8( ext.status_text );
        if( ext.keyid && *ext.keyid )
            messagePart.keyId = ext.keyid;
        if( messagePart.keyId.isEmpty() )
            messagePart.keyId = ext.fingerprint; // take fingerprint if no id found (e.g. for S/MIME)
    // ### Ugh. We depend on two enums being in sync:
        messagePart.keyTrust = (Kpgp::Validity)ext.validity;
        if( ext.userid && *ext.userid )
            messagePart.signer = QString::fromUtf8( ext.userid );
        for( int iMail = 0; iMail < ext.emailCount; ++iMail )
            // The following if /should/ allways result in TRUE but we
            // won't trust implicitely the plugin that gave us these data.
            if( ext.emailList[ iMail ] && *ext.emailList[ iMail ] )
                messagePart.signerMailAddresses.append( QString::fromUtf8( ext.emailList[ iMail ] ) );
        if( ext.creation_time )
            messagePart.creationTime = *ext.creation_time;
        if(     70 > messagePart.creationTime.tm_year
            || 200 < messagePart.creationTime.tm_year
            ||   1 > messagePart.creationTime.tm_mon
            ||  12 < messagePart.creationTime.tm_mon
            ||   1 > messagePart.creationTime.tm_mday
            ||  31 < messagePart.creationTime.tm_mday ) {
            messagePart.creationTime.tm_year = 0;
            messagePart.creationTime.tm_mon  = 1;
            messagePart.creationTime.tm_mday = 1;
        }
        if( messagePart.signer.isEmpty() ) {
            if( ext.name && *ext.name )
                messagePart.signer = QString::fromUtf8( ext.name );
            if( messagePart.signerMailAddresses.count() ) {
                if( !messagePart.signer.isEmpty() )
                    messagePart.signer += " ";
                messagePart.signer += "<";
                messagePart.signer += messagePart.signerMailAddresses.first();
                messagePart.signer += ">";
            }
        }

        kdDebug(5006) << "\n  key id: " << messagePart.keyId << "\n  key trust: " << messagePart.keyTrust << "\n  signer: " << messagePart.signer << endl;

      } else {
        messagePart.creationTime.tm_year = 0;
        messagePart.creationTime.tm_mon  = 1;
        messagePart.creationTime.tm_mday = 1;
      }
    } else {
      KMessageBox::information(reader,
          i18n("Problem: This Crypto Plug-In cannot verify message signatures.\n"
               "Please specify an appropriate Plug-In using the 'Settings/Configure KMail / Plug-In' dialog."),
               QString::null,
               "cryptoPluginBox");
    }

    QString unknown( i18n("(unknown)") );
    if( !data ){
      if( new_cleartext ) {
        if( reader )
            reader->queueHtml( reader->writeSigstatHeader( messagePart,
                                                           useThisCryptPlug,
                                                           fromAddress ) );
        bIsOpaqueSigned = true;
        deb = "\n\nN E W    C O N T E N T = \"";
        deb += new_cleartext;
        deb += "\"  <--  E N D    O F    N E W    C O N T E N T\n\n";
        kdDebug(5006) << deb << endl;
        insertAndParseNewChildNode( reader,
                                    resultString,
                                    cryptPlugList,
                                    useThisCryptPlug,
                                    sign,
                                    new_cleartext,
                                    "opaqued signed data" );
        delete new_cleartext;

        if( reader )
          reader->queueHtml( reader->writeSigstatFooter( messagePart ) );

      }
      else if( !hideErrors )
      {
        QString txt;
        txt = "<hr><b><h2>";
        txt.append( i18n( "The crypto engine returned no cleartext data!" ) );
        txt.append( "</h2></b>" );
        txt.append( "<br>&nbsp;<br>" );
        txt.append( i18n( "Status: " ) );
        if( sigMeta.status && *sigMeta.status ) {
          txt.append( "<i>" );
          txt.append( sigMeta.status );
          txt.append( "</i>" );
        }
        else
          txt.append( unknown );
        if( reader )
          reader->queueHtml(txt);
      }
    }
    else
    {
      if( reader )
        reader->queueHtml( reader->writeSigstatHeader( messagePart,
                                                       useThisCryptPlug,
                                                       fromAddress ) );
      parseObjectTree( reader,
                       resultString,
                       cryptPlugList,
                       useThisCryptPlug,
                       data );
      if( reader )
        reader->queueHtml( reader->writeSigstatFooter( messagePart ) );
    }

    cryptPlug->freeSignatureMetaData( &sigMeta );

  } else {
    if( reader && !hideErrors ) {
      KMessageBox::information(reader,
        i18n("problem: No Crypto Plug-Ins found.\n"
             "Please specify a Plug-In using the 'Settings/Configure KMail / Plug-In' dialog."),
             QString::null,
             "cryptoPluginBox");
      reader->queueHtml(i18n("<hr><b><h2>Signature could *not* be verified !</h2></b><br>"
                   "reason:<br><i>&nbsp; &nbsp; No Crypto Plug-Ins found.</i><br>"
                   "proposal:<br><i>&nbsp; &nbsp; Please specify a Plug-In by invoking<br>&nbsp; &nbsp; the "
                   "'Settings/Configure KMail / Plug-In' dialog!</i>"));
    }
  }
  kdDebug(5006) << "\nKMReaderWin::writeOpaqueOrMultipartSignedData: done, returning "
                << ( bIsOpaqueSigned ? "TRUE" : "FALSE" ) << endl;
  return bIsOpaqueSigned;
}


//pending(khz): replace this and put it into CryptPlugWrapper class  (khz, 2002/06/27)
class tmpHelper {
public:
    static QString pluginErrorIdToErrorText( int errId, bool& passphraseError )
    {
        /* The error numbers used by GPGME.  */
    /*
        typedef enum
        {
            GPGME_EOF                = -1,
            GPGME_No_Error           = 0,
            GPGME_General_Error      = 1,
            GPGME_Out_Of_Core        = 2,
            GPGME_Invalid_Value      = 3,
            GPGME_Busy               = 4,
            GPGME_No_Request         = 5,
            GPGME_Exec_Error         = 6,
            GPGME_Too_Many_Procs     = 7,
            GPGME_Pipe_Error         = 8,
            GPGME_No_Recipients      = 9,
            GPGME_No_Data            = 10,
            GPGME_Conflict           = 11,
            GPGME_Not_Implemented    = 12,
            GPGME_Read_Error         = 13,
            GPGME_Write_Error        = 14,
            GPGME_Invalid_Type       = 15,
            GPGME_Invalid_Mode       = 16,
            GPGME_File_Error         = 17,  // errno is set in this case.
            GPGME_Decryption_Failed  = 18,
            GPGME_No_Passphrase      = 19,
            GPGME_Canceled           = 20,
            GPGME_Invalid_Key        = 21,
            GPGME_Invalid_Engine     = 22,
            GPGME_Invalid_Recipients = 23
        }
    */
        /*
        NOTE:
            The following hack *must* be changed into something
            using an extra enum specified in the CryptPlug API
            *and* the file error number (case 17) must be taken
            into account.                     (khz, 2002/27/06)
        */
        passphraseError = false;
        switch( errId ){
            case /*GPGME_EOF                = */-1:
                return(i18n("End of File reached during operation."));
                break;
            case /*GPGME_No_Error           = */0:
                return(i18n("No error."));
                break;
            case /*GPGME_General_Error      = */1:
                return(i18n("General error."));
                break;
            case /*GPGME_Out_Of_Core        = */2:
                return(i18n("Out of core!"));
                break;
            case /*GPGME_Invalid_Value      = */3:
                return(i18n("Invalid value."));
                break;
            case /*GPGME_Busy               = */4:
                return(i18n("Engine is busy."));
                break;
            case /*GPGME_No_Request         = */5:
                return(i18n("No request."));
                break;
            case /*GPGME_Exec_Error         = */6:
                return(i18n("Execution error."));
                break;
            case /*GPGME_Too_Many_Procs     = */7:
                return(i18n("Too many processes."));
                break;
            case /*GPGME_Pipe_Error         = */8:
                return(i18n("Pipe error."));
                break;
            case /*GPGME_No_Recipients      = */9:
                return(i18n("No recipients."));
                break;
            case /*GPGME_No_Data            = */10:
                return(i18n("No data."));
                break;
            case /*GPGME_Conflict           = */11:
                return(i18n("Conflict."));
                break;
            case /*GPGME_Not_Implemented    = */12:
                return(i18n("Not implemented."));
                break;
            case /*GPGME_Read_Error         = */13:
                return(i18n("Read error."));
                break;
            case /*GPGME_Write_Error        = */14:
                return(i18n("Write error."));
                break;
            case /*GPGME_Invalid_Type       = */15:
                return(i18n("Invalid type."));
                break;
            case /*GPGME_Invalid_Mode       = */16:
                return(i18n("Invalid mode."));
                break;
            case /*GPGME_File_Error         = */17:  // errno is set in this case.
                return(i18n("File error."));
                break;
            case /*GPGME_Decryption_Failed  = */18:
                return(i18n("Decryption failed."));
                break;
            case /*GPGME_No_Passphrase      = */19:
                passphraseError = true;
                return(i18n("No passphrase."));
                break;
            case /*GPGME_Canceled           = */20:
                passphraseError = true;
                return(i18n("Canceled."));
                break;
            case /*GPGME_Invalid_Key        = */21:
                passphraseError = true;
                return(i18n("Invalid key."));
                break;
            case /*GPGME_Invalid_Engine     = */22:
                return(i18n("Invalid engine."));
                break;
            case /*GPGME_Invalid_Recipients = */23:
                return(i18n("Invalid recipients."));
                break;
            default:
                return(i18n("Unknown error."));
            }
    }
};

bool KMReaderWin::okDecryptMIME( KMReaderWin* reader,
                                 CryptPlugWrapperList* cryptPlugList,
                                 CryptPlugWrapper*     useThisCryptPlug,
                                 partNode& data,
                                 QCString& decryptedData,
                                 bool showWarning,
                                 bool& passphraseError,
                                 QString& aErrorText )
{
  passphraseError = false;
  aErrorText = "";
  const QString errorContentCouldNotBeDecrypted( i18n("Content could *not* be decrypted.") );

  bool bDecryptionOk = false;
  CryptPlugWrapper* cryptPlug = useThisCryptPlug ? useThisCryptPlug : cryptPlugList->active();
  if( cryptPlug ) {
    QByteArray ciphertext( data.msgPart().bodyDecodedBinary() );
    QCString cipherStr( ciphertext );
    bool cipherIsBinary = (-1 == cipherStr.find("BEGIN ENCRYPTED MESSAGE", 0, false) ) &&
                          (-1 == cipherStr.find("BEGIN PGP ENCRYPTED MESSAGE", 0, false) ) &&
                          (-1 == cipherStr.find("BEGIN PGP MESSAGE", 0, false) );
    int cipherLen = ciphertext.size();
//#define KHZ_TEST
#ifdef KHZ_TEST
    QFile fileC( "testdat_xx1.encrypted" );
    if( fileC.open( IO_WriteOnly ) ) {
        QDataStream dc( &fileC );
        dc.writeRawBytes( ciphertext, ciphertext.size() );
        fileC.close();
    }
#endif

    QCString deb;
    deb =  "\n\nE N C R Y P T E D    D A T A = ";
    if( cipherIsBinary )
      deb += "[binary data]";
    else {
      deb += "\"";
      deb += ciphertext;
      deb += "\"";
    }
    deb += "\n\n";
    kdDebug(5006) << deb << endl;




    char* cleartext = 0;
    const char* certificate = 0;

    if( reader && ! cryptPlug->hasFeature( Feature_DecryptMessages ) ) {
      reader->showMessageAndSetData( errorContentCouldNotBeDecrypted,
        i18n("Crypto Plug-In %1 can not decrypt any messages.").arg(cryptPlug->libName()),
        i18n("Please specify a *matching* Plug-In by invoking"),
        i18n("the 'Settings/Configure KMail / Plug-In' dialog!"),
        decryptedData );
    } else {
      kdDebug() << "\nKMReaderWin::decryptMIME: going to call CRYPTPLUG "
                << cryptPlug->libName() << endl;
      int errId = 0;
      char* errTxt = 0;
      bDecryptionOk = cryptPlug->decryptMessage( ciphertext,
                                                 cipherIsBinary,
                                                 cipherLen,
                                                 &cleartext,
                                                 certificate,
                                                 &errId,
                                                 &errTxt );
      kdDebug() << "\nKMReaderWin::decryptMIME: returned from CRYPTPLUG" << endl;
      aErrorText = tmpHelper::pluginErrorIdToErrorText( errId, passphraseError );
      if( bDecryptionOk )
        decryptedData = cleartext;
      else if( reader && showWarning ){
        reader->showMessageAndSetData( errorContentCouldNotBeDecrypted,
          i18n("Crypto Plug-In %1 could not decrypt the data.")
            .arg(cryptPlug->libName()),
          i18n("Error: %2")
            .arg( aErrorText ),
          passphraseError
          ? ""
          : i18n("Make sure the Plug-In is installed properly and check "
                 "your specifications made in the "
                 "'Settings/Configure KMail / Plug-In' dialog!"),
          decryptedData );
      }
      delete errTxt;
    }

    delete cleartext;

  } else {
      if( reader )
        reader->showMessageAndSetData( errorContentCouldNotBeDecrypted,
          i18n("No Crypto Plug-In settings found."),
          i18n("Please specify a Plug-In by invoking"),
          i18n("the 'Settings/Configure KMail / Plug-In' dialog!"),
          decryptedData );
  }
  return bDecryptionOk;
}



//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(KMMessage* aMsg, bool onlyProcessHeaders)
{
  QString s("\n#######\n#######\n#######  parseMsg(KMMessage* aMsg ");
  if( aMsg == message() )
    s += "==";
  else
    s += "!=";
  s += " aMsg, bool onlyProcessHeaders == ";
  if( onlyProcessHeaders )
    s += "true";
  else
    s += "false";
  s += "\n#######\n#######";
kdDebug(5006) << s << endl;

  mColorBar->setEraseColor( QColor( "white" ) );
  mColorBar->setText("");

  if( !onlyProcessHeaders )
    removeTempFiles();
  KMMessagePart msgPart;
  int numParts;
  QCString type, subtype, contDisp;
  QByteArray str;
  partNode* savedRootNode = 0;

  assert(aMsg!=NULL);

  type = aMsg->typeStr();
  numParts = aMsg->numBodyParts();


  int mainType    = aMsg->type();
  int mainSubType = aMsg->subtype();
  QString mainCntTypeStr;
  if(    (DwMime::kTypeNull    == mainType)
      || (DwMime::kTypeUnknown == mainType) ){
    mainType    = DwMime::kTypeText;
    mainSubType = DwMime::kSubtypePlain;
    mainCntTypeStr = "text/plain";
  } else {
    mainCntTypeStr = aMsg->typeStr();
    int scpos = mainCntTypeStr.find(';');
    if( -1 < scpos)
      mainCntTypeStr.truncate( scpos );
  }

  VCard *vc = 0;
  bool hasVCard = false;

  // make sure we have a valid CryptPlugList
  bool tmpPlugList = !mCryptPlugList;
  if( tmpPlugList ) {
    mCryptPlugList = new CryptPlugWrapperList();
    KConfig *config = KGlobal::config();
    mCryptPlugList->loadFromConfig( config );
  }

  // store message body in mRootNode if *no* body parts found
  // (please read the comment below before crying about me)  :-)
  DwBodyPart* mainBody = 0;
  DwBodyPart* firstBodyPart = aMsg->getFirstDwBodyPart();
  if( !firstBodyPart ) {
    // ATTENTION: This definitely /should/ be optimized.
    //            Copying the message text into a new body part
    //            surely is not the most efficient way to go.
    //            I decided to do so for being able to get a
    //            solution working for old style (== non MIME)
    //            mails without spending much time on implementing.
    //            During code revisal when switching to KMime
    //            all this will probably disappear anyway (or it
    //            will be optimized, resp.).       (khz, 6.12.2001)
    kdDebug() << "*no* first body part found, creating one from Message" << endl;
    mainBody = new DwBodyPart(aMsg->asDwString(), 0);
    mainBody->Parse();
  }

  if( mRootNode ) {
    if( onlyProcessHeaders )
      savedRootNode = mRootNode;
    else
      delete mRootNode;
  }
  mRootNode = new partNode( mainBody ? mainBody : 0,
                            mainType,
                            mainSubType );

  mRootNode->setFromAddress( aMsg->from() );

  QString cntDesc, cntSize, cntEnc;
  cntDesc = aMsg->subject();
  if( cntDesc.isEmpty() )
    cntDesc = i18n("( body part )");
  cntSize = QString::number( aMsg->msgSize() );
  if( aMsg->contentTransferEncodingStr().isEmpty() )
    cntEnc = "7bit";
  else
    cntEnc = aMsg->contentTransferEncodingStr();

  if( firstBodyPart ) {
kdDebug(5006) << "\n     ----->  First body part *was* found, filling the Mime Part Tree" << endl;
    // store pointers to the MIME objects in our fast access tree
    partNode* curNode = mRootNode->setFirstChild( new partNode(firstBodyPart) );
    curNode->buildObjectTree();
    // fill the MIME part tree viewer
    if( mMimePartTree && !onlyProcessHeaders )
      mRootNode->fillMimePartTree( 0,
                                   mMimePartTree,
                                   cntDesc,
                                   mainCntTypeStr,
                                   cntEnc,
                                   cntSize );
  } else if( mMimePartTree && !onlyProcessHeaders ) {
kdDebug(5006) << "\n     ----->  Inserting Root Node into the Mime Part Tree" << endl;
    mRootNode->fillMimePartTree( 0,
                                 mMimePartTree,
                                 cntDesc,
                                 mainCntTypeStr,
                                 cntEnc,
                                 cntSize );
kdDebug(5006) << "\n     <-----  Finished inserting Root Node into Mime Part Tree" << endl;
  } else if(  !onlyProcessHeaders ){
kdDebug(5006) << "\n     ------  Sorry, no Mime Part Tree - can NOT insert Root Node!" << endl;
  }

  partNode* vCardNode = mRootNode->findType( DwMime::kTypeText, DwMime::kSubtypeXVCard );
  if( vCardNode ) {
    int vcerr;
    QTextCodec *atmCodec = (mAutoDetectEncoding) ?
      KMMsgBase::codecForName(vCardNode->msgPart().charset()) : mCodec;
    if (!atmCodec) atmCodec = mCodec;
    vc = VCard::parseVCard(atmCodec->toUnicode(
            vCardNode->msgPart().bodyDecoded() ),
            &vcerr);
    if( vc ) {
      delete vc;
      kdDebug(5006) << "FOUND A VALID VCARD" << endl;
      hasVCard = true;
      writePartIcon(&vCardNode->msgPart(), aMsg->partNumber(vCardNode->dwPart()), TRUE );
    }
  }

  queueHtml("<div id=\"header\">"
          + (writeMsgHeader(aMsg, hasVCard))
          + "</div><div><br></div>");


  mIsFirstTextPart = true;
  // show message content
  if( !onlyProcessHeaders )
    parseObjectTree( this,
                     0,
                     mCryptPlugList,
                     0,
                     mRootNode );


  // store encrypted/signed status information in the KMMessage
  //  - this can only be done *after* calling parseObjectTree()
  KMMsgEncryptionState encryptionState = mRootNode->overallEncryptionState();
  KMMsgSignatureState  signatureState  = mRootNode->overallSignatureState();
  aMsg->setEncryptionState( encryptionState );
  aMsg->setSignatureState(  signatureState  );

  bool emitReplaceMsgByUnencryptedVersion = false;

// note: The following define is specified on top of this file. To compile
//       a less strict version of KMail just comment it out there above.
#ifdef STRICT_RULES_OF_GERMAN_GOVERNMENT_02

  // Hack to make sure the S/MIME CryptPlugs follows the strict requirement
  // of german government:
  // --> All received encrypted messages *must* be stored in unencrypted form
  //     after they have been decrypted once the user has read them.
  //     ( "Aufhebung der Verschluesselung nach dem Lesen" )
  //
  // note: Since there is no configuration option for this, we do that for
  //       all kinds of encryption now - *not* just for S/MIME.
  //       This could be changed in the objectTreeToDecryptedMsg() function
  //       by deciding when (or when not, resp.) to set the 'dataNode' to
  //       something different than 'curNode'.


kdDebug(5006) << "\n\n\nKMReaderWin::parseMsg()  -  special post-encryption handling:\n1." << endl;
kdDebug(5006) << "(!onlyProcessHeaders) = "                        << (!onlyProcessHeaders) << endl;
kdDebug(5006) << "(aMsg == msg) = "                               << (aMsg == message()) << endl;
kdDebug(5006) << "   (KMMsgStatusUnknown == mLastStatus) = "           << (KMMsgStatusUnknown == mLastStatus) << endl;
kdDebug(5006) << "|| (KMMsgStatusNew     == mLastStatus) = "           << (KMMsgStatusNew     == mLastStatus) << endl;
kdDebug(5006) << "|| (KMMsgStatusUnread  == mLastStatus) = "           << (KMMsgStatusUnread  == mLastStatus) << endl;
kdDebug(5006) << "(mIdOfLastViewedMessage != aMsg->msgId()) = "    << (mIdOfLastViewedMessage != aMsg->msgId()) << endl;
kdDebug(5006) << "   (KMMsgFullyEncrypted == encryptionState) = "     << (KMMsgFullyEncrypted == encryptionState) << endl;
kdDebug(5006) << "|| (KMMsgPartiallyEncrypted == encryptionState) = " << (KMMsgPartiallyEncrypted == encryptionState) << endl;
         // only proceed if we were called the normal way - not by
         // click in the MIME tree viewer
  if(    !onlyProcessHeaders
         // only proceed if we were called the normal way - not by
         // double click on the message (==not running in a separate window)
      && (aMsg == message())
         // only proceed if this message was not saved encryptedly before
         // to make sure only *new* messages are saved in decrypted form
      && (    (KMMsgStatusUnknown == mLastStatus)
           || (KMMsgStatusNew     == mLastStatus)
           || (KMMsgStatusUnread  == mLastStatus) )
         // avoid endless recursions
      && (mIdOfLastViewedMessage != aMsg->msgId())
         // only proceed if this message is (at least partially) encrypted
      && (    (KMMsgFullyEncrypted == encryptionState)
           || (KMMsgPartiallyEncrypted == encryptionState) ) ) {

kdDebug(5006) << "KMReaderWin  -  calling objectTreeToDecryptedMsg()" << endl;

    NewByteArray decryptedData;
    // note: The following call may change the message's headers.
    objectTreeToDecryptedMsg( mRootNode, decryptedData, *aMsg );
    // add a NULL to the data
    decryptedData.appendNULL();
    QCString resultString( decryptedData.data() );
kdDebug(5006) << "KMReaderWin  -  resulting data:" << resultString << endl;

    if( !resultString.isEmpty() ) {
kdDebug(5006) << "KMReaderWin  -  composing unencrypted message" << endl;
      // try this:
      aMsg->setBody( resultString );
      KMMessage* unencryptedMessage = new KMMessage( *aMsg );
      // because this did not work:
      /*
      DwMessage dwMsg( DwString( aMsg->asString() ) );
      dwMsg.Body() = DwBody( DwString( resultString.data() ) );
      dwMsg.Body().Parse();
      KMMessage* unencryptedMessage = new KMMessage( &dwMsg );
      */
kdDebug(5006) << "KMReaderWin  -  resulting message:" << unencryptedMessage->asString() << endl;
kdDebug(5006) << "KMReaderWin  -  attach unencrypted message to aMsg" << endl;
      aMsg->setUnencryptedMsg( unencryptedMessage );
      emitReplaceMsgByUnencryptedVersion = true;
    }
  }
#endif

  // remove temp. CryptPlugList
  if( tmpPlugList )
    delete mCryptPlugList;

  // save current main Content-Type before deleting mRootNode
  int rootNodeCntType = mRootNode ? mRootNode->type() : DwMime::kTypeUnknown;

  // if necessary restore original mRootNode
  if( savedRootNode ) {
    if( mRootNode )
      delete mRootNode;
    mRootNode = savedRootNode;
  }

  // store message id to avoid endless recursions
  setIdOfLastViewedMessage( aMsg->msgId() );

  if( emitReplaceMsgByUnencryptedVersion ) {
kdDebug(5006) << "KMReaderWin  -  invoce saving in decrypted form:" << endl;
    emit replaceMsgByUnencryptedVersion();
  } else {
kdDebug(5006) << "KMReaderWin  -  finished parsing and displaying of message." << endl;
    if (!onlyProcessHeaders)
      showHideMimeTree( (DwMime::kTypeMultipart   == rootNodeCntType) ||
                        (DwMime::kTypeApplication == rootNodeCntType) ||
                        (DwMime::kTypeMessage     == rootNodeCntType) ||
                        (DwMime::kTypeModel       == rootNodeCntType) );

    if( mColorBar->text().isEmpty() ) {
      if(    (KMMsgFullyEncrypted     == encryptionState)
          || (KMMsgPartiallyEncrypted == encryptionState)
          || (KMMsgFullySigned        == signatureState)
          || (KMMsgPartiallySigned    == signatureState) ){
        mColorBar->setEraseColor( mPrinting ? QColor( "white" ) : cCBpgp );
        partNode::CryptoType crypt = mRootNode->firstCryptoType();
        switch( crypt ){
            case partNode::CryptoTypeUnknown:
                kdDebug(5006) << "KMReaderWin  -  BUG: crypto flag is set but CryptoTypeUnknown." << endl;
                break;
            case partNode::CryptoTypeNone:
                kdDebug(5006) << "KMReaderWin  -  BUG: crypto flag is set but CryptoTypeNone." << endl;
                break;
            case partNode::CryptoTypeInlinePGP:
            case partNode::CryptoTypeOpenPgpMIME:
                mColorBar->setText(i18n("\nP\nG\nP\n \nM\ne\ns\ns\na\ng\ne"));
                break;
            case partNode::CryptoTypeSMIME:
                mColorBar->setText(i18n("\nS\n-\nM\nI\nM\nE\n \nM\ne\ns\ns\na\ng\ne"));
                break;
            case partNode::CryptoType3rdParty:
                mColorBar->setText(i18n("\nS\ne\nc\nu\n\rn\ne\n \nM\ne\ns\ns\na\ng\ne"));
                break;
        }
      }
    }
  }
  if( mColorBar->text().isEmpty() ) {
    mColorBar->setEraseColor( cCBplain );
    mColorBar->setText(i18n("\nI\nn\ns\ne\nc\nu\nr\ne\n \nM\ne\ns\ns\na\ng\ne"));
  }
}


//-----------------------------------------------------------------------------
QString KMReaderWin::writeMsgHeader(KMMessage* aMsg, bool hasVCard)
{
  if( !aMsg )
    return QString();

  QString vcname;

// The direction of the header is determined according to the direction
// of the application layout.

  QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );
  QString headerStr = QString("<div dir=\"%1\">").arg(dir);

// However, the direction of the message subject within the header is
// determined according to the contents of the subject itself. Since
// the "Re:" and "Fwd:" prefixes would always cause the subject to be
// considered left-to-right, they are ignored when determining its
// direction. TODO: Implement this using the custom prefixes.

   QString subjectDir;
   if (!aMsg->subject().isEmpty()) {
      subjectDir = (KMMsgBase::skipKeyword(aMsg->subject())
                         .isRightToLeft()) ? "rtl" : "ltr";
   } else
      subjectDir = i18n("No Subject").isRightToLeft() ? "rtl" : "ltr";


  if (hasVCard) vcname = mTempFiles.last();

  switch (mHeaderStyle)
  {
  case HdrBrief:
    headerStr += QString("<div dir=\"%1\"><b style=\"font-size:130%\">" +
                        strToHtml(aMsg->subject()) +
                        "</b>&nbsp; (" +
                        KMMessage::emailAddrAsAnchor(aMsg->from(),TRUE) + ", ")
                        .arg(subjectDir);

    if (!aMsg->cc().isEmpty())
    {
      headerStr.append(i18n("CC: ")+
                       KMMessage::emailAddrAsAnchor(aMsg->cc(),TRUE) + ", ");
    }

    headerStr.append("&nbsp;"+strToHtml(aMsg->dateShortStr()) + ")");

    if (hasVCard)
    {
      headerStr.append("&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }

    headerStr.append("</div>");
    break;

  case HdrStandard:
    headerStr += QString("<div dir=\"%1\"><b style=\"font-size:130%\">" +
                        strToHtml(aMsg->subject()) + "</b></div>")
                        .arg(subjectDir);
    headerStr.append(i18n("From: ") +
                     KMMessage::emailAddrAsAnchor(aMsg->from(),FALSE));
    if (hasVCard)
    {
      headerStr.append("&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }
    headerStr.append("<br>");
    headerStr.append(i18n("To: ") +
                     KMMessage::emailAddrAsAnchor(aMsg->to(),FALSE) + "<br>");
    if (!aMsg->cc().isEmpty())
      headerStr.append(i18n("CC: ")+
                       KMMessage::emailAddrAsAnchor(aMsg->cc(),FALSE) + "<br>");
    break;

  case HdrFancy:
  {
    // the subject line and box below for details
    headerStr += QString("<div class=\"fancyHeaderSubj\" dir=\"%1\">"
                        "<b>%2</b></div>"
                        "<div class=\"fancyHeaderDtls\">"
                        "<table class=\"fancyHeaderDtls\">")
                        .arg(subjectDir)
		        .arg(aMsg->subject().isEmpty()?
			     i18n("No Subject") :
			     strToHtml(aMsg->subject()));

    // from line
    headerStr.append(QString("<tr><th class=\"fancyHeaderDtls\">%1</th><td class=\"fancyHeaderDtls\">%2%3%4</td></tr>")
                            .arg(i18n("From: "))
                            .arg(KMMessage::emailAddrAsAnchor(aMsg->from(),FALSE))
                            .arg(hasVCard ?
                                 "&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>"
                                 : "")
                            .arg((aMsg->headerField("Organization").isEmpty()) ?
                                 ""
                                 : "&nbsp;&nbsp;(" +
                                   strToHtml(aMsg->headerField("Organization")) +
                                   ")"));

    // to line
    headerStr.append(QString("<tr><th class=\"fancyHeaderDtls\">%1</th><td class=\"fancyHeaderDtls\">%2</td></tr>")
                            .arg(i18n("To: "))
                            .arg(KMMessage::emailAddrAsAnchor(aMsg->to(),FALSE)));

    // cc line, if any
    if (!aMsg->cc().isEmpty())
    {
      headerStr.append(QString("<tr><th class=\"fancyHeaderDtls\">%1</th><td class=\"fancyHeaderDtls\">%2</td></tr>")
                              .arg(i18n("CC: "))
                              .arg(KMMessage::emailAddrAsAnchor(aMsg->cc(),FALSE)));
    }

    // the date
    QString dateString;
    if (mPrinting)
    {
        QDateTime dateTime;
        KLocale* locale = KGlobal::locale();
        dateTime.setTime_t(aMsg->date());
        dateString = locale->formatDateTime(dateTime);
    }
    else
    {
        dateString = aMsg->dateStr();
    }
    headerStr.append(QString("<tr><th class=\"fancyHeaderDtls\">%1</th><td dir=\"%2\" class=\"fancyHeaderDtls\">%3</td></tr>")
                            .arg(i18n("Date: "))
			    .arg(aMsg->dateStr().isRightToLeft() ? "rtl" : "ltr")
                            .arg(strToHtml(dateString)));
    headerStr.append("</table></div>");
    break;
  }
  case HdrLong:
    headerStr += QString("<div dir=\"%1\"><b style=\"font-size:130%\">" +
                        strToHtml(aMsg->subject()) + "</b></div>")
                        .arg(subjectDir);
    headerStr.append(i18n("Date: ") + strToHtml(aMsg->dateStr())+"<br>");
    headerStr.append(i18n("From: ") +
                     KMMessage::emailAddrAsAnchor(aMsg->from(),FALSE));
    if (hasVCard)
    {
      headerStr.append("&nbsp;&nbsp;<a href=\"" +
                       vcname +
                       "\">"+i18n("[vCard]")+"</a>");
    }

    if (!aMsg->headerField("Organization").isEmpty())
    {
      headerStr.append("&nbsp;&nbsp;(" +
                       strToHtml(aMsg->headerField("Organization")) + ")");
    }

    headerStr.append("<br>");
    headerStr.append(i18n("To: ")+
                   KMMessage::emailAddrAsAnchor(aMsg->to(),FALSE) + "<br>");
    if (!aMsg->cc().isEmpty())
    {
      headerStr.append(i18n("CC: ")+
                       KMMessage::emailAddrAsAnchor(aMsg->cc(),FALSE) + "<br>");
    }

    if (!aMsg->bcc().isEmpty())
    {
      headerStr.append(i18n("BCC: ")+
                       KMMessage::emailAddrAsAnchor(aMsg->bcc(),FALSE) + "<br>");
    }

    if (!aMsg->replyTo().isEmpty())
    {
      headerStr.append(i18n("Reply to: ")+
                     KMMessage::emailAddrAsAnchor(aMsg->replyTo(),FALSE) + "<br>");
    }
    break;

  case HdrAll:
      // we force the direction to ltr here, even in a arabic/hebrew UI,
      // as the headers are almost all Latin1
    headerStr += "<div dir=\"ltr\">";
    headerStr += strToHtml(aMsg->headerAsString(), true);
    if (hasVCard)
    {
      headerStr.append("<br><a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }
    headerStr += "</div>";
    break;

  default:
    kdDebug(5006) << "Unsupported header style " << mHeaderStyle << endl;
  }

  headerStr += "</div>";

  return headerStr;
}



#define SIG_FRAME_COL_UNDEF  99
#define SIG_FRAME_COL_RED    -1
#define SIG_FRAME_COL_YELLOW  0
#define SIG_FRAME_COL_GREEN   1
QString KMReaderWin::sigStatusToString( CryptPlugWrapper* cryptPlug,
                                        int status_code,
                                        CryptPlugWrapper::SigStatusFlags statusFlags,
                                        int& frameColor,
                                        bool& showKeyInfos )
{
    // note: At the moment frameColor and showKeyInfos are
    //       used for CMS only but not for PGP signatures
    // pending(khz): Implement usage of these for PGP sigs as well.
    showKeyInfos = true;
    QString result;
    if( cryptPlug ) {
        if( 0 <= cryptPlug->libName().find( "gpgme-openpgp", 0, false ) ) {
            // process enum according to it's definition to be read in
            // GNU Privacy Guard CVS repository /gpgme/gpgme/gpgme.h
            switch( status_code ) {
            case 0: // GPGME_SIG_STAT_NONE
                result = i18n("Error: Signature not verified");
                break;
            case 1: // GPGME_SIG_STAT_GOOD
                result = i18n("Good signature");
                break;
            case 2: // GPGME_SIG_STAT_BAD
                result = i18n("BAD signature");
                break;
            case 3: // GPGME_SIG_STAT_NOKEY
                result = i18n("No public key to verify the signature");
                break;
            case 4: // GPGME_SIG_STAT_NOSIG
                result = i18n("No signature found");
                break;
            case 5: // GPGME_SIG_STAT_ERROR
                result = i18n("Error verifying the signature");
                break;
            case 6: // GPGME_SIG_STAT_DIFF
                result = i18n("Different results for signatures");
                break;
            /* PENDING(khz) Verify exact meaning of the following values:
            case 7: // GPGME_SIG_STAT_GOOD_EXP
                return i18n("Signature certificate is expired");
            break;
            case 8: // GPGME_SIG_STAT_GOOD_EXPKEY
                return i18n("One of the certificate's keys is expired");
            break;
            */
            default:
                result = "";   // do *not* return a default text here !
                break;
            }
        } else
        if( 0 <= cryptPlug->libName().find( "gpgme-smime",   0, false ) ) {
            // process status bits according to SigStatus_...
            // definitions in kdenetwork/libkdenetwork/cryptplug.h

            if( CryptPlugWrapper::SigStatus_UNKNOWN == statusFlags ) {
                result = i18n("No status information available.");
                frameColor = SIG_FRAME_COL_YELLOW;
                showKeyInfos = false;
                return result;
            }

            if( CryptPlugWrapper::SigStatus_VALID & statusFlags ) {
                result = i18n("GOOD signature!");
                // Note:
                // Here we are work differently than KMail did before!
                //
                // The GOOD case ( == sig matching and the complete
                // certificate chain was verified and is valid today )
                // by definition does *not* show any key
                // information but just states that things are OK.
                //           (khz, according to LinuxTag 2002 meeting)
                frameColor = SIG_FRAME_COL_GREEN;
                showKeyInfos = false;
                return result;
            }

            // we are still there?  OK, let's test the different cases:

            // we assume green, test for yellow or red (in this order!)
            frameColor = SIG_FRAME_COL_GREEN;
            QString result2;
            if( CryptPlugWrapper::SigStatus_KEY_EXPIRED & statusFlags ){
                // still is green!
                result2 += i18n("One key has expired.");
            }
            if( CryptPlugWrapper::SigStatus_SIG_EXPIRED & statusFlags ){
                // and still is green!
                result2 += i18n("The signature has expired.");
            }

            // test for yellow:
            if( CryptPlugWrapper::SigStatus_KEY_MISSING & statusFlags ) {
                result2 += i18n("Unable to verify: key missing.");
                // if the signature certificate is missing
                // we cannot show infos on it
                showKeyInfos = false;
                frameColor = SIG_FRAME_COL_YELLOW;
            }
            if( CryptPlugWrapper::SigStatus_CRL_MISSING & statusFlags ){
                result2 += i18n("CRL not available.");
                frameColor = SIG_FRAME_COL_YELLOW;
            }
            if( CryptPlugWrapper::SigStatus_CRL_TOO_OLD & statusFlags ){
                result2 += i18n("Available CRL is too old.");
                frameColor = SIG_FRAME_COL_YELLOW;
            }
            if( CryptPlugWrapper::SigStatus_BAD_POLICY & statusFlags ){
                result2 += i18n("A policy was not met.");
                frameColor = SIG_FRAME_COL_YELLOW;
            }
            if( CryptPlugWrapper::SigStatus_SYS_ERROR & statusFlags ){
                result2 += i18n("A system error occurred.");
                // if a system error occured
                // we cannot trust any information
                // that was given back by the plug-in
                showKeyInfos = false;
                frameColor = SIG_FRAME_COL_YELLOW;
            }
            if( CryptPlugWrapper::SigStatus_NUMERICAL_CODE & statusFlags ) {
                result2 += i18n("Internal system error #%1 occurred.")
                        .arg( statusFlags - CryptPlugWrapper::SigStatus_NUMERICAL_CODE );
                // if an unsupported internal error occured
                // we cannot trust any information
                // that was given back by the plug-in
                showKeyInfos = false;
                frameColor = SIG_FRAME_COL_YELLOW;
            }

            // test for red:
            if( CryptPlugWrapper::SigStatus_KEY_REVOKED & statusFlags ){
                // this is red!
                result2 += i18n("One key has been revoked.");
                frameColor = SIG_FRAME_COL_RED;
            }
            if( CryptPlugWrapper::SigStatus_RED & statusFlags ) {
                if( result2.isEmpty() )
                    // Note:
                    // Here we are work differently than KMail did before!
                    //
                    // The BAD case ( == sig *not* matching )
                    // by definition does *not* show any key
                    // information but just states that things are BAD.
                    //
                    // The reason for this: In this case ALL information
                    // might be falsificated, we can NOT trust the data
                    // in the body NOT the signature - so we don't show
                    // any key/signature information at all!
                    //         (khz, according to LinuxTag 2002 meeting)
                    showKeyInfos = false;
                frameColor = SIG_FRAME_COL_RED;
            }
            else
                result = "";

            if( SIG_FRAME_COL_GREEN == frameColor ) {
                if( result2.isEmpty() )
                    result = i18n("GOOD signature!");
                else
                    result = i18n("Good signature.");
            } else if( SIG_FRAME_COL_RED == frameColor ) {
                if( result2.isEmpty() )
                    result = i18n("BAD signature!");
                else
                    result = i18n("Bad signature.");
            } else
                result = "";

            if( !result2.isEmpty() ) {
                if( !result.isEmpty() )
                    result.append("<br />");
                result.append( result2 );
            }
        }
        /*
        // add i18n support for 3rd party plug-ins here:
        else if (0 <= cryptPlug->libName().find( "yetanotherpluginname", 0, false )) {

        }
        */
    }
    return result;
}

//---------------------------------------------------

QString KMReaderWin::writeSigstatHeader( PartMetaData& block,
                                         CryptPlugWrapper* cryptPlug,
                                         const QString& fromAddress )
{
    bool isSMIME = cryptPlug && (0 <= cryptPlug->libName().find( "smime",   0, false ));
    QString signer = block.signer;

    QString htmlStr;
    QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );

    if( block.isEncapsulatedRfc822Message )
    {
        htmlStr += "<table cellspacing=\"1\" cellpadding=\"0\" class=\"rfc822\">"
            "<tr class=\"rfc822H\"><td dir=\"" + dir + "\">";
        htmlStr += i18n("Encapsulated message");
        htmlStr += "</td></tr><tr class=\"rfc822B\"><td>";
    }

    if( block.isEncrypted )
    {
        htmlStr += "<table cellspacing=\"1\" cellpadding=\"0\" class=\"encr\">"
            "<tr class=\"encrH\"><td dir=\"" + dir + "\">";
        if( block.isDecryptable )
            htmlStr += i18n("Encrypted message");
        else {
            htmlStr +=
                QString("%1<br />%2 <i>%3</i>")
                .arg( i18n("Cannot decrypt message.") )
                .arg( block.errorText.isEmpty() ? "" : i18n("Error: ") )
                .arg( block.errorText );
        }
        htmlStr += "</td></tr><tr class=\"encrB\"><td>";
    }

    if (block.isSigned) {
        QStringList& blockAddrs( block.signerMailAddresses );
        // note: At the moment frameColor and showKeyInfos are
        //       used for CMS only but not for PGP signatures
        // pending(khz): Implement usage of these for PGP sigs as well.
        int frameColor = SIG_FRAME_COL_UNDEF;
        bool showKeyInfos;
        bool onlyShowKeyURL = false;
        bool cannotCheckSignature;
        QString statusStr = sigStatusToString( cryptPlug,
                                               block.status_code,
                                               block.sigStatusFlags,
                                               frameColor,
                                               showKeyInfos );
        // if needed fallback to english status text
        // that was reported by the plugin
        if( statusStr.isEmpty() )
            statusStr = block.status;

        switch( frameColor ){
            case SIG_FRAME_COL_RED:
                cannotCheckSignature = false;
                break;
            case SIG_FRAME_COL_YELLOW:
                cannotCheckSignature = true;
                break;
            case SIG_FRAME_COL_GREEN:
                cannotCheckSignature = false;
                break;
        }

        // compose the string for displaying the key ID
        // either as URL or not linked (for PGP)
        // note: Once we can start PGP key manager programs
        //       from within KMail we could change this and
        //       allways show the URL.    (khz, 2002/06/27)
        QString startKeyHREF;
        if( isSMIME )
            startKeyHREF =
                QString("<a href=\"kmail:showCertificate#%1 ### %2 ### %3\">")
                .arg( cryptPlug->displayName() )
                .arg( cryptPlug->libName() )
                .arg( block.keyId );
        QString keyWithWithoutURL
            = isSMIME
            ? QString("%1%2</a>")
                .arg( startKeyHREF )
                .arg( cannotCheckSignature ? i18n("[Details]") : ("0x" + block.keyId) )
            : "0x" + QString::fromUtf8( block.keyId );


        // temporary hack: allways show key infos!
        showKeyInfos = true;

        // Sorry for using 'black' as NULL color but .isValid()
        // checking with QColor default c'tor did not work for
        // some reason.
        if( isSMIME && (SIG_FRAME_COL_UNDEF != frameColor) ) {

            // new frame settings for CMS:
            // beautify the status string
            if( !statusStr.isEmpty() ) {
                statusStr.prepend("<i>");
                statusStr.append( "</i>");
            }

            // special color handling: S/MIME uses only green/yellow/red.
            switch( frameColor ){
                case SIG_FRAME_COL_RED:
                    block.signClass = "signErr";//"signCMSRed";
                    onlyShowKeyURL = true;
                    break;
                case SIG_FRAME_COL_YELLOW:
                    block.signClass = "signOkKeyBad";//"signCMSYellow";
                    break;
                case SIG_FRAME_COL_GREEN: {
                        block.signClass = "signOkKeyOk";//"signCMSGreen";
                        // extra hint for green case
                        // that email addresses in DN do not match fromAddress
                        QString greenCaseWarning;
                        QString msgFrom( KMMessage::getEmailAddr(fromAddress) );
                        QString certificate;
                        if( block.keyId.isEmpty() )
                            certificate = "certificate";
                        else
                            certificate = QString("%1%2</a>")
                                          .arg( startKeyHREF )
                                          .arg( "certificate" );
                        if( blockAddrs.count() ){
                            if( blockAddrs.grep(
                                    msgFrom,
                                    false ).isEmpty() ) {
                                greenCaseWarning =
                                    "<u>" +
                                    i18n("Warning:") +
                                    "</u> " +
                                    i18n("Sender's mail address is not stored "
                                         "in the %1 used for signing.").arg(certificate) +
                                    "<br />" +
                                    i18n("sender: ") +
                                    "&lt;" +
                                    msgFrom +
                                    "&gt;<br />" +
                                    i18n("stored: ") +
                                    "&lt;";
                                // We cannot use Qt's join() function here but
                                // have to join the addresses manually to
                                // extract the mail addresses (without '<''>')
                                // before including it into our string:
                                bool bStart = true;
                                for(QStringList::ConstIterator it = blockAddrs.begin();
                                    it != blockAddrs.end(); ++it ){
                                    if( !bStart )
                                        greenCaseWarning.append("&gt;, <br />&nbsp; &nbsp;&lt;");
                                    bStart = false;
                                    greenCaseWarning.append( KMMessage::getEmailAddr(*it) );
                                }
                                greenCaseWarning.append( "&gt;" );
                            }
                        } else {
                            greenCaseWarning =
                                "<u>" +
                                i18n("Warning:") +
                                "</u> " +
                                i18n("No mail address is stored in the %1 used for signing, "
                                     "so we cannot compare it to the sender's address &lt;%2&gt;.")
                                .arg(certificate)
                                .arg(msgFrom);
                        }
                        if( !greenCaseWarning.isEmpty() ) {
                            if( !statusStr.isEmpty() )
                                statusStr.append("<br />&nbsp;<br />");
                            statusStr.append( greenCaseWarning );
                        }
                    }
                    break;
            }

            htmlStr += "<table cellspacing=\"1\" cellpadding=\"0\" "
                "class=\"" + block.signClass + "\">"
                "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
            if( showKeyInfos ) {

                if( cannotCheckSignature ) {
                    htmlStr += i18n( "Not enough information to check "
                                     "signature. %1" )
                                .arg( keyWithWithoutURL );
                }
                else {

                    if (block.signer.isEmpty())
                        signer = "";
                    else {
                        // HTMLize the signer's user id and try to create mailto: link
                        signer.replace( QRegExp("&"), "&amp;" );
                        signer.replace( QRegExp("<"), "&lt;" );
                        signer.replace( QRegExp(">"), "&gt;" );
                        signer.replace( QRegExp("\""), "&quot;" );
                        if( blockAddrs.count() ){
                            QString address( blockAddrs.first() );
                            address.replace( QRegExp("&"), "&amp;" );
                            address.replace( QRegExp("<"), "&lt;" );
                            address.replace( QRegExp(">"), "&gt;" );
                            address.replace( QRegExp("\""), "&quot;" );
                            signer = "<a href=\"mailto:" + address + "\">" + signer + "</a>";
                        }
                    }

                    if( block.keyId.isEmpty() ) {
                        if( signer.isEmpty() || onlyShowKeyURL )
                            htmlStr += i18n( "Message was signed with unknown key." );
                        else
                            htmlStr += i18n( "Message was signed by %1." )
                                    .arg( signer );
                    } else {
                        bool dateOK = (0 < block.creationTime.tm_year);
                        QDate created( 1900 + block.creationTime.tm_year,
                                    block.creationTime.tm_mon,
                                    block.creationTime.tm_mday );
                        if( dateOK && created.isValid() ) {
                            if( signer.isEmpty() ) {
                                if( onlyShowKeyURL )
                                    htmlStr += i18n( "Message was signed with key %1." )
                                                .arg( keyWithWithoutURL );
                                else
                                    htmlStr += i18n( "Message was signed with key %1, created %2." )
                                                .arg( keyWithWithoutURL ).arg( created.toString( Qt::LocalDate ) );
                            }
                            else {
                                if( onlyShowKeyURL )
                                    htmlStr += i18n( "Message was signed with key %1." )
                                            .arg( keyWithWithoutURL );
                                else
                                    htmlStr += i18n( "Message was signed by %1 with key %2, created %3." )
                                            .arg( signer )
                                            .arg( keyWithWithoutURL )
                                            .arg( created.toString( Qt::LocalDate ) );
                            }
                        }
                        else {
                            if( signer.isEmpty() || onlyShowKeyURL )
                                htmlStr += i18n( "Message was signed with key %1." )
                                        .arg( keyWithWithoutURL );
                            else
                                htmlStr += i18n( "Message was signed by %1 with key %2." )
                                        .arg( signer )
                                        .arg( keyWithWithoutURL );
                        }
                    }
                }
                htmlStr += "<br />";
                if( !statusStr.isEmpty() ) {
                    htmlStr += "&nbsp;<br />";
                    htmlStr += i18n( "Status: " );
                    htmlStr += statusStr;
                }
            } else {
                htmlStr += statusStr;
            }
            htmlStr += "</td></tr><tr class=\"" + block.signClass + "B\"><td>";

        } else {

            // old frame settings for PGP:

            if (block.signer.isEmpty()) {
                block.signClass = "signWarn";
                htmlStr += "<table cellspacing=\"1\" cellpadding=\"0\" "
                    "class=\"" + block.signClass + "\">"
                    "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
                if( !block.keyId.isEmpty() ) {
                    bool dateOK = (0 < block.creationTime.tm_year);
                    QDate created( 1900 + block.creationTime.tm_year,
                                  block.creationTime.tm_mon,
                                  block.creationTime.tm_mday );
                    if( dateOK && created.isValid() )
                        htmlStr += i18n( "Message was signed with unknown key %1, created %2." )
                                .arg( keyWithWithoutURL ).arg( created.toString( Qt::LocalDate ) );
                    else
                        htmlStr += i18n( "Message was signed with unknown key %1." )
                                .arg( keyWithWithoutURL );
                }
                else
                    htmlStr += i18n( "Message was signed with unknown key." );
                htmlStr += "<br />";
                htmlStr += i18n( "The validity of the signature cannot be "
                        "verified." );
                if( !statusStr.isEmpty() ) {
                    htmlStr += "<br />";
                    htmlStr += i18n( "Status: " );
                    htmlStr += "<i>";
                    htmlStr += statusStr;
                    htmlStr += "</i>";
                }
                htmlStr += "</td></tr><tr class=\"" + block.signClass + "B\"><td>";
            }
            else
            {
                // HTMLize the signer's user id and create mailto: link
                signer.replace( QRegExp("&"), "&amp;" );
                signer.replace( QRegExp("<"), "&lt;" );
                signer.replace( QRegExp(">"), "&gt;" );
                signer.replace( QRegExp("\""), "&quot;" );
                signer = "<a href=\"mailto:" + signer + "\">" + signer + "</a>";


                if (block.isGoodSignature) {
                    if( block.keyTrust < Kpgp::KPGP_VALIDITY_MARGINAL )
                        block.signClass = "signOkKeyBad";
                    else
                        block.signClass = "signOkKeyOk";
                    htmlStr += "<table cellspacing=\"1\" cellpadding=\"0\" "
                        "class=\"" + block.signClass + "\">"
                        "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
                    if( !block.keyId.isEmpty() )
                        htmlStr += i18n( "Message was signed by %1 (Key ID: %2)." )
                        .arg( signer )
                        .arg( keyWithWithoutURL );
                    else
                        htmlStr += i18n( "Message was signed by %1." ).arg( signer );
                    htmlStr += "<br />";

                    switch( block.keyTrust )
                    {
                        case Kpgp::KPGP_VALIDITY_UNKNOWN:
                        htmlStr += i18n( "The signature is valid, but the key's "
                                "validity is unknown." );
                        break;
                        case Kpgp::KPGP_VALIDITY_MARGINAL:
                        htmlStr += i18n( "The signature is valid and the key is "
                                "marginally trusted." );
                        break;
                        case Kpgp::KPGP_VALIDITY_FULL:
                        htmlStr += i18n( "The signature is valid and the key is "
                                "fully trusted." );
                        break;
                        case Kpgp::KPGP_VALIDITY_ULTIMATE:
                        htmlStr += i18n( "The signature is valid and the key is "
                                "ultimately trusted." );
                        break;
                        default:
                        htmlStr += i18n( "The signature is valid, but the key is "
                                "untrusted." );
                    }
                    htmlStr += "</td></tr>"
                        "<tr class=\"" + block.signClass + "B\"><td>";
                }
                else
                {
                    block.signClass = "signErr";
                    htmlStr += "<table cellspacing=\"1\" cellpadding=\"0\" "
                        "class=\"" + block.signClass + "\">"
                        "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
                    if( !block.keyId.isEmpty() )
                        htmlStr += i18n( "Message was signed by %1 (Key ID: %2)." )
                        .arg( signer )
                        .arg( keyWithWithoutURL );
                    else
                        htmlStr += i18n( "Message was signed by %1." ).arg( signer );
                    htmlStr += "<br />";
                    htmlStr += i18n("Warning: The signature is bad.");
                    htmlStr += "</td></tr>"
                        "<tr class=\"" + block.signClass + "B\"><td>";
                }
            }
        }
    }

    return htmlStr;
}

QString KMReaderWin::writeSigstatFooter( PartMetaData& block )
{
    QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );

    QString htmlStr;

    if (block.isSigned) {
	htmlStr += "</td></tr><tr class=\"" + block.signClass + "H\">";
	htmlStr += "<td dir=\"" + dir + "\">" +
	    i18n( "End of signed message" ) +
	    "</td></tr></table>";
    }

    if (block.isEncrypted) {
	htmlStr += "</td></tr><tr class=\"encrH\"><td dir=\"" + dir + "\">" +
		i18n( "End of encrypted message" ) +
	    "</td></tr></table>";
    }

    if( block.isEncapsulatedRfc822Message )
    {
        htmlStr += "</td></tr><tr class=\"rfc822H\"><td dir=\"" + dir + "\">" +
            i18n( "End of encapsulated message" ) +
            "</td></tr></table>";
    }

    return htmlStr;
}

//-----------------------------------------------------------------------------
void KMReaderWin::writeBodyStr( const QCString aStr, QTextCodec *aCodec,
                                const QString& fromAddress,
                                bool* flagSigned, bool* flagEncrypted )
{
  QString line, htmlStr;
  QString signClass;
  bool goodSignature = false;
  Kpgp::Module* pgp = Kpgp::Module::getKpgp();
  assert(pgp != NULL);
  bool isPgpMessage = false; // true if the message contains at least one
                             // PGP MESSAGE or one PGP SIGNED MESSAGE block
  QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );
  QString headerStr = QString("<div dir=\"%1\">").arg(dir);

  QPtrList<Kpgp::Block> pgpBlocks;
  QStrList nonPgpBlocks;
  if( Kpgp::Module::prepareMessageForDecryption( aStr, pgpBlocks, nonPgpBlocks ) )
  {
      bool isEncrypted = false, isSigned = false;
      bool couldDecrypt = false;
      QString signer;
      QCString keyId;
      QString decryptionError;
      Kpgp::Validity keyTrust = Kpgp::KPGP_VALIDITY_FULL;

      QPtrListIterator<Kpgp::Block> pbit( pgpBlocks );

      QStrListIterator npbit( nonPgpBlocks );

      for( ; *pbit != 0; ++pbit, ++npbit )
      {
	  // insert the next Non-OpenPGP block
	  QCString str( *npbit );
	  if( !str.isEmpty() )
	    htmlStr += quotedHTML( aCodec->toUnicode( str ) );

	  //htmlStr += "<br>";

	  Kpgp::Block* block = *pbit;
	  if( ( block->type() == Kpgp::PgpMessageBlock ) ||
	      ( block->type() == Kpgp::ClearsignedBlock ) )
	  {
	      isPgpMessage = true;
	      if( block->type() == Kpgp::PgpMessageBlock )
	      {
		  emit noDrag();
		  // try to decrypt this OpenPGP block
		  couldDecrypt = block->decrypt();
		  isEncrypted = block->isEncrypted();
		  if (!couldDecrypt) {
		    decryptionError = pgp->lastErrorMsg();
		  }
	      }
	      else
	      {
		  // try to verify this OpenPGP block
		  block->verify();
	      }

	      isSigned = block->isSigned();
	      if( isSigned )
	      {
                  keyId = block->signatureKeyId();
		  signer = block->signatureUserId();
		  if( !signer.isEmpty() )
		  {
		      goodSignature = block->goodSignature();

		      if( !keyId.isEmpty() )
			keyTrust = pgp->keyTrust( keyId );
		      else
			// This is needed for the PGP 6 support because PGP 6 doesn't
			// print the key id of the signing key if the key is known.
			keyTrust = pgp->keyTrust( signer );
		  }
	      }

	      if( isSigned ) {
		if( flagSigned )
		  *flagSigned = true;
	      }
	      if( isEncrypted ) {
		if( flagEncrypted )
		  *flagEncrypted = true;
	      }

	      PartMetaData messagePart;

	      messagePart.isSigned = isSigned;
	      messagePart.isGoodSignature = goodSignature;
	      messagePart.isEncrypted = isEncrypted;
	      messagePart.isDecryptable = couldDecrypt;
	      messagePart.decryptionError = decryptionError;
	      messagePart.signer = signer;
	      messagePart.keyId = keyId;
	      messagePart.keyTrust = keyTrust;

	      htmlStr += writeSigstatHeader( messagePart, 0, fromAddress );

	      htmlStr += quotedHTML( aCodec->toUnicode( block->text() ) );
	      htmlStr += writeSigstatFooter( messagePart );
	  }
	  else // block is neither message block nor clearsigned block
	    htmlStr += quotedHTML( aCodec->toUnicode( block->text() ) );
      }

      // add the last Non-OpenPGP block
      QCString str( nonPgpBlocks.last() );
      if( !str.isEmpty() )
	  htmlStr += quotedHTML( aCodec->toUnicode( str ) );
  }
  else
      htmlStr = quotedHTML( aCodec->toUnicode( aStr ) );

  queueHtml(htmlStr);
}


//-----------------------------------------------------------------------------
void KMReaderWin::writeHTMLStr(const QString& aStr)
{
  mColorBar->setEraseColor( cCBhtml );
  mColorBar->setText(i18n("\nH\nT\nM\nL\n \nM\ne\ns\ns\na\ng\ne"));
  queueHtml(aStr);
}

//-----------------------------------------------------------------------------

QString KMReaderWin::quotedHTML(const QString& s)
{
  QString htmlStr;
  QString normalStartTag;
  const QString normalEndTag = "</div>";
  const QString quoteEnd = "</div>";

  unsigned int pos, beg;
  unsigned int length = s.length();

  QString style;
  if( mBodyFont.bold() )
    style += "font-weight:bold;";
  if( mBodyFont.italic() )
    style += "font-style:italic;";
  if( style.isEmpty() )
    normalStartTag = "<div>";
  else
    normalStartTag = QString("<div style=\"%1\">").arg( style );

  // skip leading empty lines
  for( pos = 0; pos < length && s[pos] <= ' '; pos++ );
  while (pos > 0 && (s[pos-1] == ' ' || s[pos-1] == '\t')) pos--;
  beg = pos;

  int currQuoteLevel = -2; // -2 == no previous lines

  while (beg<length)
  {
    QString line;

    /* search next occurance of '\n' */
    pos = s.find('\n', beg, FALSE);
    if (pos == (unsigned int)(-1))
	pos = length;

    line = s.mid(beg,pos-beg);
    beg = pos+1;

    /* calculate line's current quoting depth */
    int actQuoteLevel = -1;
    for (unsigned int p=0; p<line.length(); p++) {
      switch (line[p].latin1()) {
        case '>':
        case '|':
          actQuoteLevel++;
          break;
        case ' ':  // spaces and tabs are allowed between the quote markers
        case '\t':
        case '\r':
          break;
        default:  // stop quoting depth calculation
          p = line.length();
          break;
      }
    } /* for() */

    if ( actQuoteLevel != currQuoteLevel ) {
      /* finish last quotelevel */
      if (currQuoteLevel == -1)
        htmlStr.append( normalEndTag );
      else if (currQuoteLevel >= 0)
        htmlStr.append( quoteEnd );

      /* start new quotelevel */
      currQuoteLevel = actQuoteLevel;
      if (actQuoteLevel == -1)
        htmlStr += normalStartTag;
      else
        htmlStr += mQuoteFontTag[currQuoteLevel%3];
    }

    // don't write empty <div ...></div> blocks (they have zero height)
    if( !line.isEmpty() )
    {
      if( line.isRightToLeft() )
        htmlStr += QString( "<div dir=\"rtl\">" );
      else
        htmlStr += QString( "<div dir=\"ltr\">" );
      htmlStr += strToHtml( line, true );
      htmlStr += QString( "</div>" );
    }
    else
      htmlStr += "<br>";
  } /* while() */

  /* really finish the last quotelevel */
  if (currQuoteLevel == -1)
     htmlStr.append( normalEndTag );
  else
     htmlStr.append( quoteEnd );

  //kdDebug(5006) << "KMReaderWin::quotedHTML:\n"
  //              << "========================================\n"
  //              << htmlStr
  //              << "\n======================================\n";
  return htmlStr;
}



//-----------------------------------------------------------------------------
void KMReaderWin::writePartIcon(KMMessagePart* aMsgPart, int aPartNum,
  bool quiet)
{
  QString iconName, href, label, comment, contDisp;
  QString fileName;

  if(aMsgPart == NULL) {
    kdDebug(5006) << "writePartIcon: aMsgPart == NULL\n" << endl;
    return;
  }

  kdDebug(5006) << "writePartIcon: PartNum: " << aPartNum << endl;

  comment = aMsgPart->contentDescription();
  comment.replace(QRegExp("&"), "&amp;");
  comment.replace(QRegExp("<"), "&lt;");
  comment.replace(QRegExp(">"), "&gt;");

  fileName = aMsgPart->fileName();
  if (fileName.isEmpty()) fileName = aMsgPart->name();
  label = fileName;
      /* HTMLize label */
  label.replace(QRegExp("&"), "&amp;");
  label.replace(QRegExp("<"), "&lt;");
  label.replace(QRegExp(">"), "&gt;");

//--- Sven's save attachments to /tmp start ---
  KTempFile *tempFile = new KTempFile(QString::null,
    "." + QString::number(aPartNum));
  tempFile->setAutoDelete(true);
  QString fname = tempFile->name();
  delete tempFile;

  bool ok = true;

  if (access(QFile::encodeName(fname), W_OK) != 0) // Not there or not writable
    if (mkdir(QFile::encodeName(fname), 0) != 0
      || chmod (QFile::encodeName(fname), S_IRWXU) != 0)
        ok = false; //failed create

  if (ok)
  {
    mTempDirs.append(fname);
    fileName.replace(QRegExp("[/\"\']"),"");
    if (fileName.isEmpty()) fileName = "unnamed";
    fname += "/" + fileName;

    if (!kByteArrayToFile(aMsgPart->bodyDecodedBinary(), fname, false, false, false))
      ok = false;
    mTempFiles.append(fname);
  }
  if (ok)
  {
    href = QString("file:")+fname;
    //debug ("Wrote attachment to %s", href.data());
  }
  else {
    //--- Sven's save attachments to /tmp end ---
    href = QString("part://%1").arg(aPartNum+1);
  }

  // sven: for viewing images inline
  if (mInlineImage)
    iconName = href;
  else
    iconName = aMsgPart->iconName();
  if (iconName.right(14)=="mime_empty.png")
  {
    aMsgPart->magicSetType();
    iconName = aMsgPart->iconName();
  }
  if (!quiet)
    queueHtml("<table><tr><td><a href=\"" + href + "\"><img src=\"" +
                   iconName + "\" border=\"0\">" + label +
                   "</a></td></tr></table>" + comment + "<br>");
}


//-----------------------------------------------------------------------------
QString KMReaderWin::strToHtml(const QString &aStr, bool aPreserveBlanks) const
{
  return LinkLocator::convertToHtml(aStr, aPreserveBlanks);
}


//-----------------------------------------------------------------------------
void KMReaderWin::printMsg(void)
{
  if (!message()) return;

  if (mPrinting)
    mViewer->view()->print();
  else {
    KMReaderWin printWin;
    printWin.setPrinting(TRUE);
    printWin.readConfig();
    printWin.setMsg(message(), TRUE);
    printWin.printMsg();
  }
}


//-----------------------------------------------------------------------------
int KMReaderWin::msgPartFromUrl(const KURL &aUrl)
{
  if (aUrl.isEmpty() || !message()) return -1;

  if (!aUrl.isLocalFile()) return -1;

  QString path = aUrl.path();
  uint right = path.findRev('/');
  uint left = path.findRev('.', right);

  bool ok;
  int res = path.mid(left + 1, right - left - 1).toInt(&ok);
  return (ok) ? res : -1;
}


//-----------------------------------------------------------------------------
void KMReaderWin::resizeEvent(QResizeEvent *)
{
  if( !mResizeTimer.isActive() )
  {
    //
    // Combine all resize operations that are requested as long a
    // the timer runs.
    //
    mResizeTimer.start( 100, true );
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotDelayedResize()
{
  //mViewer->widget()->setGeometry(0, 0, width(), height());
  mBox->setGeometry(0, 0, width(), height());
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotTouchMessage()
{
  if (message())
  {
    KMMsgStatus st = message()->status();
    if (st == KMMsgStatusNew || st == KMMsgStatusUnread
        || st == KMMsgStatusRead)
      message()->setStatus(KMMsgStatusOld);
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::closeEvent(QCloseEvent *e)
{
  KMReaderWinInherited::closeEvent(e);
  writeConfig();
}


bool foundSMIMEData( const QString aUrl,
                     QString& displayName,
                     QString& libName,
                     QString& keyId )
{
  static QString showCertMan("showCertificate#");
  displayName = "";
  libName = "";
  keyId = "";
  int i1 = aUrl.find( showCertMan );
  if( -1 < i1 ) {
    i1 += showCertMan.length();
    int i2 = aUrl.find(" ### ", i1);
    if( i1 < i2 )
    {
      displayName = aUrl.mid( i1, i2-i1 );
      i1 = i2+5;
      i2 = aUrl.find(" ### ", i1);
      if( i1 < i2 )
      {
        libName = aUrl.mid( i1, i2-i1 );
        i2 += 5;

        keyId = aUrl.mid( i2 );
        /*
        int len = aUrl.length();
        if( len > i2+1 ) {
          keyId = aUrl.mid( i2, 2 );
          i2 += 2;
          while( len > i2+1 ) {
            keyId += ':';
            keyId += aUrl.mid( i2, 2 );
            i2 += 2;
          }
        }
        */
      }
    }
  }
  return !keyId.isEmpty();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOn(const QString &aUrl)
{
  bool bOk = false;

  QString dummyStr;
  QString keyId;

  KURL url(aUrl);
  int id = msgPartFromUrl(url);

  if (id > 0)
  {
    partNode* node = mRootNode ? mRootNode->findId( id ) : 0;
    if( node ) {
      KMMessagePart& msgPart = node->msgPart();
      QString str = msgPart.fileName();
      if (str.isEmpty()) str = msgPart.name();
      emit statusMsg(i18n("Attachment: ") + str);
      bOk = true;
    }
  }
  else if( aUrl == "kmail:showHTML" )
  {
    emit statusMsg( i18n("Turn on HTML rendering for this message.") );
    bOk = true;
  }
  else if( foundSMIMEData( aUrl, dummyStr, dummyStr, keyId ) )
  {
    emit statusMsg(i18n("Show certificate 0x%1").arg(keyId));
    bOk = true;
  }
  if( !bOk )
    emit statusMsg(aUrl);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen(const KURL &aUrl, const KParts::URLArgs &)
{
  QString displayName;
  QString libName;
  QString keyId;
  if( aUrl.hasRef() && foundSMIMEData( aUrl.path()+"#"+aUrl.ref(), displayName, libName, keyId ) )
  {
    QString query( "-query " );
    query += keyId;
    KProcess certManagerProc; // save to create on the heap, since
                            // there is no parent
    certManagerProc << "kgpgcertmanager";
    certManagerProc << displayName;
    certManagerProc << libName;
    certManagerProc << "-query";
    certManagerProc << keyId;
    if( !certManagerProc.start( KProcess::DontCare ) )
      KMessageBox::error( this, i18n( "Could not start certificate manager. Please check your installation!" ),
                          i18n( "KMail Error" ) );
    else
      kdDebug(5006) << "\nKMReaderWin::slotUrlOn(): certificate manager started.\n" << endl;
    // process continues to run even after the KProcess object goes
    // out of scope here, since it is started in DontCare run mode.
    return;
  }

  if (!aUrl.hasHost() && aUrl.path() == "/" && aUrl.hasRef())
  {
    if (!mViewer->gotoAnchor(aUrl.ref()))
      static_cast<QScrollView *>(mViewer->widget())->ensureVisible(0,0);
    return;
  }
  int id = msgPartFromUrl(aUrl);
  if (id > 0)
  {
    // clicked onto an attachment
    mAtmCurrent = id;
    mAtmCurrentName = aUrl.path();
    slotAtmOpen();
  }
  else {
//      if (aUrl.protocol().isEmpty() || (aUrl.protocol() == "file"))
//	  return;
      emit urlClicked(aUrl,/* aButton*/LeftButton); //### FIXME: add button to URLArgs!
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlPopup(const QString &aUrl, const QPoint& aPos)
{
  if (!message()) return;
  KURL url( aUrl );

  int id = msgPartFromUrl(url);
  if (id <= 0)
  {
    emit popupMenu(*message(), url, aPos);
  }
  else
  {
    // Attachment popup
    mAtmCurrent = id;
    mAtmCurrentName = url.path();
    KPopupMenu *menu = new KPopupMenu();
    menu->insertItem(i18n("Open..."), this, SLOT(slotAtmOpen()));
    menu->insertItem(i18n("Open With..."), this, SLOT(slotAtmOpenWith()));
    menu->insertItem(i18n("View..."), this, SLOT(slotAtmView()));
    menu->insertItem(i18n("Save As..."), this, SLOT(slotAtmSave()));
    menu->insertItem(i18n("Properties..."), this,
		     SLOT(slotAtmProperties()));
    menu->exec(aPos,0);
    delete menu;
  }
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotFind()
{
  //dnaber:
  KAction *act = mViewer->actionCollection()->action("find");
  if( act )
    act->activate();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotToggleFixedFont()
{
  mUseFixedFont = !mUseFixedFont;
  mBodyFamily = (mUseFixedFont) ? mFixedFont.family() : mBodyFont.family();
  fntSize = (mUseFixedFont) ? mFixedFont.pointSize() : mBodyFont.pointSize();
  mViewer->setStandardFont(mBodyFamily);
  update(true);
}

//-----------------------------------------------------------------------------
void KMReaderWin::atmViewMsg(KMMessagePart* aMsgPart)
{
  KMMessage* msg = new KMMessage;
  assert(aMsgPart!=NULL);

  msg->fromString(aMsgPart->bodyDecoded());
  emit showAtmMsg(msg);
}


//-----------------------------------------------------------------------------
void KMReaderWin::atmView(KMReaderWin* aReaderWin, KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname, QTextCodec *codec)
{
  QString str;

  if (aReaderWin && qstricmp(aMsgPart->typeStr(), "message")==0)
  {
    aReaderWin->atmViewMsg(aMsgPart);
    return;
  }

  kernel->kbp()->busy();
  {
    KMReaderWin* win = new KMReaderWin; //new reader
    if (qstricmp(aMsgPart->typeStr(), "message")==0)
    {               // if called from compose win
      KMMessage* msg = new KMMessage;
      assert(aMsgPart!=NULL);
      msg->fromString(aMsgPart->bodyDecoded());
      win->setCaption(msg->subject());
      win->setMsg(msg, true);
      win->show();
    }
    else if (qstricmp(aMsgPart->typeStr(), "text")==0)
    {
      if (qstricmp(aMsgPart->subtypeStr(), "x-vcard") == 0) {
        KMDisplayVCard *vcdlg;
	int vcerr;
	VCard *vc = VCard::parseVCard(codec->toUnicode(aMsgPart
          ->bodyDecoded()), &vcerr);

	if (!vc) {
          QString errstring = i18n("Error reading in vCard:\n");
	  errstring += VCard::getError(vcerr);
          kernel->kbp()->idle();
	  KMessageBox::error(NULL, errstring, i18n("vCard error"));
	  return;
	}

	vcdlg = new KMDisplayVCard(vc);
        kernel->kbp()->idle();
	vcdlg->show();
	return;
      }
      win->readConfig();
      if ( codec )
	win->setCodec( codec );
      else
	win->setCodec( KGlobal::charsets()->codecForName( "iso8859-1" ) );
      win->mViewer->begin( KURL( "file:/" ) );
      win->queueHtml("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 "
		     "Transitional//EN\">\n<html><head><title></title>"
		     "<style type=\"text/css\">" +
		 QString("a { color: %1;").arg(win->c2.name()) +
		 "text-decoration: none; }" + // just playing
		 "</style></head><body " +
                 QString(" text=\"%1\"").arg(win->c1.name()) +
  		 QString(" bgcolor=\"%1\"").arg(win->c4.name()) +
		 ">" );

      QCString str = aMsgPart->bodyDecoded();
      if (aHTML && (qstricmp(aMsgPart->subtypeStr(), "html")==0))  // HTML
        //win->mViewer->write(win->codec()->toUnicode(str));
	win->writeHTMLStr(win->codec()->toUnicode(str));
      else // plain text
        win->writeBodyStr( str,
                           win->codec(),
                           win->message() ? win->message()->from() : "" );
      win->queueHtml("</body></html>");
      win->sendNextHtmlChunk();
      win->setCaption(i18n("View Attachment: ") + pname);
      win->show();
    }
    else if (qstricmp(aMsgPart->typeStr(), "image")==0 ||
             (qstricmp(aMsgPart->typeStr(), "application")==0 &&
              qstricmp(aMsgPart->subtypeStr(), "postscript")))
    {
      if (aFileName.isEmpty()) return;  // prevent crash
      // Open the window with a size so the image fits in (if possible):
      QImageIO *iio = new QImageIO();
      iio->setFileName(aFileName);
      if( iio->read() ) {
        QImage img = iio->image();
        int scnum = QApplication::desktop()->screenNumber(win);
	if( img.width() > 50 && img.width() > 50	// avoid super small windows
	    && img.width() < QApplication::desktop()->screen(scnum)->width()	// avoid super large windows
	    && img.height() < QApplication::desktop()->screen(scnum)->height() ) {
	  win->resize(img.width()+10, img.height()+10);
	}
      }
      // Just write the img tag to HTML:
      win->mViewer->begin( KURL( "file:/" ) );
      win->mViewer->write("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 "
			  "Transitional//EN\">\n<html><title></title><body>");
      QString linkName = QString("<img src=\"file:%1\" border=0>").arg(aFileName);
      win->mViewer->write(linkName);
      win->mViewer->write("</body></html>");
      win->mViewer->end();
      win->setCaption(i18n("View Attachment: ") + pname);
      win->show();
    } else {
      QMultiLineEdit *medit = new QMultiLineEdit();
      QString str = aMsgPart->bodyDecoded();
      // A QString cannot handle binary data. So if it's shorter than the
      // attachment, we assume the attachment is binary:
      if( str.length() < (unsigned) aMsgPart->decodedSize() ) {
        str += i18n("\n[KMail: Attachment contains binary data. Trying to show first %1 characters.]").arg(str.length());
      }
      medit->setText(str);
      medit->setReadOnly(true);
      medit->resize(500, 550);
      medit->show();
    }
  }
  // ---Sven's view text, html and image attachments in html widget end ---
  kernel->kbp()->idle();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmView()
{
  partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
  if( node ) {
    KMMessagePart& msgPart = node->msgPart();
    QString pname = msgPart.fileName();
    if (pname.isEmpty()) pname=msgPart.name();
    if (pname.isEmpty()) pname=msgPart.contentDescription();
    if (pname.isEmpty()) pname="unnamed";
    // image Attachment is saved already
    QTextCodec *atmCodec = (mAutoDetectEncoding) ?
      KMMsgBase::codecForName(msgPart.charset()) : mCodec;
    if (!atmCodec) atmCodec = mCodec;
    atmView(this, &msgPart, htmlMail(), mAtmCurrentName, pname, atmCodec);
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmOpen()
{
  QString str, pname, cmd, fileName;

  partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
  if( !node )
    return;

  KMMessagePart& msgPart = node->msgPart();

  if (qstricmp(msgPart.typeStr(), "message")==0)
  {
    atmViewMsg(&msgPart);
    return;
  }

  if (qstricmp(msgPart.typeStr(), "text") == 0)
  {
    if (qstricmp(msgPart.subtypeStr(), "x-vcard") == 0) {
      KMDisplayVCard *vcdlg;
      int vcerr;
      QTextCodec *atmCodec = (mAutoDetectEncoding) ?
        KMMsgBase::codecForName(msgPart.charset()) : mCodec;
      if (!atmCodec) atmCodec = mCodec;
      VCard *vc = VCard::parseVCard(atmCodec->toUnicode(msgPart
        .bodyDecoded()), &vcerr);

      if (!vc) {
        QString errstring = i18n("Error reading in vCard:\n");
        errstring += VCard::getError(vcerr);
        KMessageBox::error(this, errstring, i18n("vCard error"));
        return;
      }

      vcdlg = new KMDisplayVCard(vc);
      vcdlg->show();
      return;
    }
  }

  // What to do when user clicks on an attachment --dnaber, 2000-06-01
  // TODO: show full path for Service, not only name
  QString mimetype = KMimeType::findByURL(KURL(mAtmCurrentName))->name();
  KService::Ptr offer = KServiceTypeProfile::preferredService(mimetype, "Application");
  QString question;
  QString open_text = i18n("&Open");
  QString filenameText = msgPart.fileName();
  if (filenameText.isEmpty()) filenameText = msgPart.name();
  if ( offer ) {
    question = i18n("Open attachment '%1' using '%2'?").arg(filenameText)
      .arg(offer->name());
  } else {
    question = i18n("Open attachment '%1'?").arg(filenameText);
    open_text = i18n("&Open With...");
  }
  question += i18n("\n\nNote that opening an attachment may compromise your system's security!");
  // TODO: buttons don't have the correct order, but "Save" should be default
  int choice = KMessageBox::warningYesNoCancel(this, question,
      i18n("Open Attachment?"), i18n("&Save to Disk"), open_text);
  if( choice == KMessageBox::Yes ) {		// Save
    slotAtmSave();
  } else if( choice == KMessageBox::No ) {	// Open
    if ( offer ) {
      // There's a default service for this kind of file - use it
      KURL::List lst;
      KURL url;
      url.setPath(mAtmCurrentName);
      lst.append(url);
      KRun::run(*offer, lst);
    } else {
      // There's no know service that handles this type of file, so open
      // the "Open with..." dialog.
      KURL::List lst;
      KURL url;
      url.setPath(mAtmCurrentName);
      lst.append(url);
      KRun::displayOpenWithDialog(lst);
    }
  } else {					// Cancel
    kdDebug(5006) << "Canceled opening attachment" << endl;
  }

}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmOpenWith()
{
  // It makes sense to have an extra "Open with..." entry in the menu
  // so the user can change filetype associations.

    KURL::List lst;
    KURL url;
    url.setPath(mAtmCurrentName);
    lst.append(url);
    KRun::displayOpenWithDialog(lst);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmSave()
{
  QString fileName;
  fileName = mSaveAttachDir;

  partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
  if( node ) {
    KMMessagePart& msgPart = node->msgPart();

    if (!msgPart.fileName().isEmpty())
      fileName.append(msgPart.fileName());
    else
      fileName.append(msgPart.name());

    while (fileName.find(':') != -1)
      fileName = fileName.mid(fileName.find(':') + 1).stripWhiteSpace();
    fileName.replace(QRegExp("/"), "");
    KURL url = KFileDialog::getSaveURL( fileName, QString::null, this );

    if( url.isEmpty() )
      return;

    mSaveAttachDir = url.directory() + "/";

    kernel->byteArrayToRemoteFile(msgPart.bodyDecodedBinary(), url);
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmProperties()
{
  KMMsgPartDialogCompat dlg(0,TRUE);

  kernel->kbp()->busy();
  partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
  if( node ) {
    KMMessagePart& msgPart = node->msgPart();

    dlg.setMsgPart(&msgPart);
    kernel->kbp()->idle();

    dlg.exec();
  }
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollUp()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, -10);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollDown()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, 10);
}

bool KMReaderWin::atBottom() const
{
    const QScrollView *view = static_cast<const QScrollView *>(mViewer->widget());
    return view->contentsY() + view->visibleHeight() >= view->contentsHeight();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotJumpDown()
{
    QScrollView *view = static_cast<QScrollView *>(mViewer->widget());
    int offs = (view->clipper()->height() < 30) ? view->clipper()->height() : 30;
    view->scrollBy( 0, view->clipper()->height() - offs );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollPrior()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, -(int)(height()*0.8));
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotScrollNext()
{
  static_cast<QScrollView *>(mViewer->widget())->scrollBy(0, (int)(height()*0.8));
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotDocumentChanged()
{

}


//-----------------------------------------------------------------------------
void KMReaderWin::slotTextSelected(bool)
{

  QString temp = mViewer->selectedText();
  kapp->clipboard()->setText(temp);
}

//-----------------------------------------------------------------------------
void KMReaderWin::selectAll()
{
  mViewer->selectAll();
}

//-----------------------------------------------------------------------------
QString KMReaderWin::copyText()
{
  QString temp = mViewer->selectedText();
  return temp;
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotDocumentDone()
{
  // mSbVert->setValue(0);
}


//-----------------------------------------------------------------------------
void KMReaderWin::setHtmlOverride(bool override)
{
  mHtmlOverride = override;
  if (message())
      message()->setDecodeHTML(htmlMail());
}


//-----------------------------------------------------------------------------
bool KMReaderWin::htmlMail()
{
  return ((mHtmlMail && !mHtmlOverride) || (!mHtmlMail && mHtmlOverride));
}


//-----------------------------------------------------------------------------
void KMReaderWin::update( bool force )
{
    setMsg( message(), force );
}


//-----------------------------------------------------------------------------
KMMessage* KMReaderWin::message() const
{
  if (mMessage)
      return mMessage;
  if (mLastSerNum) {
    KMFolder *folder;
    KMMessage *message = 0;
    int index;
    kernel->msgDict()->getLocation( mLastSerNum, &folder, &index );
    if (folder )
      message = folder->getMsg( index );
    if (!message)
      kdDebug(5006) << "Attempt to reference invalid serial number " << mLastSerNum << "\n" << endl;
    return message;
  }
  return 0;
}

void KMReaderWin::deleteAllStandaloneWindows()
{
  mStandaloneWindows.setAutoDelete(true);
  mStandaloneWindows.clear();
}

//-----------------------------------------------------------------------------
#include "kmreaderwin.moc"
