// kmreaderwin.cpp
// Author: Markus Wuebben <markus.wuebben@kde.org>

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <qclipboard.h>
//#include <qlayout.h>
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
#include <kmessagebox.h>
#include <kpgp.h>
#include <kpgpblock.h>
#include <krun.h>
#include <ktempfile.h>

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









// INTERIM SOLUTION variable
//
// TO BE REMOVED ONCE AUTOMATIC PLUG-IN DETECTION IS FULLY WORKING
//
static CryptPlugWrapper* useThisCryptPlug = 0;



// this function will be replaced once KMime is alive (khz, 29.11.2001)
void KMReaderWin::parseObjectTree( partNode* node, bool showOneMimePart,
                                                   bool keepEncryptions,
                                                   bool includeSignatures )
{
  kdDebug(5006) << "\n**\n** KMReaderWin::parseObjectTree( "
                << (node ? "node OK, " : "no node, ")
                << "showOneMimePart: " << (showOneMimePart ? "TRUE" : "FALSE")
                << " ) **\n**" << endl;
  if( showOneMimePart ) {
    // clear the viewer
    mViewer->view()->setUpdatesEnabled( false );
    mViewer->view()->viewport()->setUpdatesEnabled( false );
    static_cast<QScrollView *>(mViewer->widget())->ensureVisible(0,0);

    if (mHtmlTimer.isActive())
    {
      mHtmlTimer.stop();
      mViewer->end();
    }
    mHtmlQueue.clear();

    mColorBar->hide();

    // start the new viewer content
    mViewer->begin( KURL( "file:/" ) );
    mViewer->write("<html><body" +
      (mPrinting ? " bgcolor=\"#FFFFFF\""
                 : QString(" bgcolor=\"%1\"").arg(c4.name())));
    if (mBackingPixmapOn && !mPrinting )
      mViewer->write(" background=\"file://" + mBackingPixmapStr + "\"");
    mViewer->write(">");
  }
  if(node && (showOneMimePart || (mShowCompleteMessage && !node->mRoot ))) {
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
                           decryptedData ) ) {
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
kdDebug(5006) << "html" << endl;
              QCString cstr( curNode->msgPart().bodyDecoded() );
              if( htmlMail() ) {
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
                writeHTMLStr(     "<table border=1 cellpadding=10><tr><td align=center>");
                writeHTMLStr(i18n("<b>HTML source</b> shown because mail contains <i>no plain text</i> data."));
                writeHTMLStr(     "<br>");
                writeHTMLStr(i18n("This is the default - and safe - behavior of KMail."));
                writeHTMLStr(     "<br><font size=-1>");
                writeHTMLStr(i18n("To enable HTML rendering (at your own risk) "
                                  "use &quot;Folder&quot; menu option."));
                writeHTMLStr(     " </font></td></tr></table>&nbsp;<br>&nbsp;<br>");
              }
              writeHTMLStr(mCodec->toUnicode( htmlMail() ? cstr : KMMessage::html2source( cstr )));
              bDone = true;
            }
            break;
          case DwMime::kSubtypeXVCard: {
kdDebug(5006) << "v-card" << endl;
              // do nothing: X-VCard is handled in parseMsg(KMMessage* aMsg)
              //             _before_ calling parseObjectTree()
              bDone = true;
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
              writeBodyStr(curNode->msgPart().bodyDecoded().data(), mCodec, &isInlineSigned, &isInlineEncrypted);
              bDone = true;
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
                parseObjectTree( curNode->mChild, showOneMimePart,
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

                if( htmlMail() && dataHtml ) {
                    if( dataPlain )
                        dataPlain->mWasProcessed = true;
                    parseObjectTree( dataHtml, showOneMimePart,
                                               keepEncryptions,
                                               includeSignatures );
                }
                else if( !htmlMail() && dataPlain ) {
                    if( dataHtml )
                        dataHtml->mWasProcessed = true;
                    parseObjectTree( dataPlain, showOneMimePart,
                                                keepEncryptions,
                                                includeSignatures );
                }
                else
                    parseObjectTree( curNode->mChild, showOneMimePart,
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

                /*
                  ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
                */
                partNode* data = 0;
                partNode* sign;
                sign = curNode->mChild->findType(      DwMime::kTypeApplication, DwMime::kSubtypePgpSignature, false, true );
                if( sign ) {
kdDebug(5006) << "       OpenPGP signature found" << endl;
                  data = curNode->mChild->findTypeNot( DwMime::kTypeApplication, DwMime::kSubtypePgpSignature, false, true );
// INTERIM SOLUTION
//
// TO BE REMOVED ONCE AUTOMATIC PLUG-IN DETECTION IS FULLY WORKING
//
if(data){
  CryptPlugWrapper* current;
  QPtrListIterator<CryptPlugWrapper> it( *mCryptPlugList );
  while( ( current = it.current() ) ) {
    ++it;
    if( 0 <= current->libName().find( "openpgp", 0, false ) ) {
      useThisCryptPlug = current;
//kdDebug(5006) << "\n\n\ngefunden1\n\n\n" << endl;
      break;
    }
  }
}
                }
                else {
                  sign = curNode->mChild->findType(      DwMime::kTypeApplication, DwMime::kSubtypePkcs7Signature, false, true );
                  if( sign ) {
kdDebug(5006) << "       S/MIME signature found" << endl;
                    data = curNode->mChild->findTypeNot( DwMime::kTypeApplication, DwMime::kSubtypePkcs7Signature, false, true );
// INTERIM SOLUTION
//
// TO BE REMOVED ONCE AUTOMATIC PLUG-IN DETECTION IS FULLY WORKING
//
if(data){
  CryptPlugWrapper* current;
  QPtrListIterator<CryptPlugWrapper> it( *mCryptPlugList );
  while( ( current = it.current() ) ) {
    ++it;
    if( 0 <= current->libName().find( "smime", 0, false ) ) {
      useThisCryptPlug = current;
//kdDebug(5006) << "\n\n\ngefunden2\n\n\n" << endl;
      break;
    }
  }
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
                  writeBodyStr(cstr, mCodec, &isInlineSigned, &isInlineEncrypted);
                  bDone = true;
                } else if( sign && data ) {
                  // Set the signature node to done to prevent it from being processed
                  // by parseObjectTree( data ) called from writeOpaqueOrMultipartSignedData().
                  sign->mWasProcessed = true;
                  writeOpaqueOrMultipartSignedData( data, *sign );
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
                writeBodyStr(cstr, mCodec, &isInlineSigned, &isInlineEncrypted);
                bDone = true;
              } else if( curNode->mChild ) {

                /*
                  ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
                */
                partNode* data =
                  curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypeOctetStream, false, true );
// INTERIM SOLUTION
//
// TO BE REMOVED ONCE AUTOMATIC PLUG-IN DETECTION IS FULLY WORKING
//
if(data){
  CryptPlugWrapper* current;
  QPtrListIterator<CryptPlugWrapper> it( *mCryptPlugList );
  while( ( current = it.current() ) ) {
    ++it;
    if( 0 <= current->libName().find( "openpgp", 0, false ) ) {
      useThisCryptPlug = current;
//kdDebug(5006) << "\n\n\ngefunden3\n\n\n" << endl;
      break;
    }
  }
}
                if( !data ) {
                  data = curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypePkcs7Mime, false, true );
// INTERIM SOLUTION
//
// TO BE REMOVED ONCE AUTOMATIC PLUG-IN DETECTION IS FULLY WORKING
//
if(data){
  CryptPlugWrapper* current;
  QPtrListIterator<CryptPlugWrapper> it( *mCryptPlugList );
  while( ( current = it.current() ) ) {
    ++it;
    if( 0 <= current->libName().find( "smime", 0, false ) ) {
//kdDebug(5006) << "\n\n\ngefunden4\n\n\n" << endl;
      useThisCryptPlug = current;
      break;
    }
  }
}
                }
                /*
                  ---------------------------------------------------------------------------------------------------------------
                */

                if( data ) {
                  curNode->setEncrypted( true );
                  QCString decryptedData;
                  if( okDecryptMIME( *data, decryptedData ) ) {
                    DwBodyPart* myBody = new DwBodyPart(DwString( decryptedData ), data->dwPart());
                    myBody->Parse();
                    partNode myBodyNode( true, myBody );
                    myBodyNode.buildObjectTree( false );
                    parseObjectTree( &myBodyNode, showOneMimePart,
                                                  keepEncryptions,
                                                  includeSignatures );
                  }
                  else
                  {
                    writeHTMLStr(mCodec->toUnicode( decryptedData ));
                  }
                  data->mWasProcessed = true; // Set the data node to done to prevent it from being processed
                  bDone = true;
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
              parseObjectTree( curNode->mChild, showOneMimePart,
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
            }
            break;
          case DwMime::kSubtypeOctetStream: {
kdDebug(5006) << "octet stream" << endl;
              CryptPlugWrapper* oldUseThisCryptPlug = useThisCryptPlug;
              if(    curNode->mRoot
                  && DwMime::kTypeMultipart    == curNode->mRoot->type()
                  && DwMime::kSubtypeEncrypted == curNode->mRoot->subType() ) {
                curNode->setEncrypted( true );
                if( keepEncryptions ) {
                  QCString cstr( curNode->msgPart().bodyDecoded() );
                  writeBodyStr(cstr, mCodec, &isInlineSigned, &isInlineEncrypted);
                  bDone = true;
                } else {


// INTERIM SOLUTION
//
// TO BE REMOVED ONCE AUTOMATIC PLUG-IN DETECTION IS FULLY WORKING
//
CryptPlugWrapper* current;
QPtrListIterator<CryptPlugWrapper> it( *mCryptPlugList );
while( ( current = it.current() ) ) {
    ++it;
    if( 0 <= current->libName().find( "openpgp", 0, false ) ) {
        useThisCryptPlug = current;
//kdDebug(5006) << "\n\n\ngefunden5\n\n\n" << endl;
        break;
    }
}



                  QCString decryptedData;
                  if( okDecryptMIME( *curNode, decryptedData ) ) {
                    DwBodyPart* myBody = new DwBodyPart(DwString( decryptedData ), curNode->dwPart());
                    myBody->Parse();
                    partNode myBodyNode( true, myBody );
                    myBodyNode.buildObjectTree( false );
                    parseObjectTree( &myBodyNode, showOneMimePart,
                                                  keepEncryptions,
                                                  includeSignatures );
                  }
                  else
                  {
                    writeHTMLStr(mCodec->toUnicode( decryptedData ));
                  }
                  bDone = true;
                }
              }
              useThisCryptPlug = oldUseThisCryptPlug;
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
              if( curNode->dwPart() && curNode->dwPart()->hasHeaders() ) {
                CryptPlugWrapper* oldUseThisCryptPlug = useThisCryptPlug;

              /*
                ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
              */


// INTERIM SOLUTION
//
// TO BE REMOVED ONCE AUTOMATIC PLUG-IN DETECTION IS FULLY WORKING
//
CryptPlugWrapper* current;
QPtrListIterator<CryptPlugWrapper> it( *mCryptPlugList );
while( ( current = it.current() ) ) {
    ++it;
    if( 0 <= current->libName().find( "smime", 0, false ) ) {
        useThisCryptPlug = current;
//kdDebug(5006) << "\n\n\ngefunden6\n\n\n" << endl;
        break;
    }
}

                DwHeaders& headers( curNode->dwPart()->Headers() );
                QCString ctypStr( headers.ContentType().AsString().c_str() );

                bool isSigned    = 0 <= ctypStr.find("smime-type=signed-data",    0, false);
                bool isEncrypted = 0 <= ctypStr.find("smime-type=enveloped-data", 0, false);

                // we call signature verification
                // if we either *know* that it is signed mail or
                // if there is *neither* signed *nor* encrypted parameter
                if( !isEncrypted ) {
                  if( isSigned )
                    kdDebug(5006) << "pkcs7 mime     ==      S/MIME TYPE: opaque signed data" << endl;
                  else
                    kdDebug(5006) << "pkcs7 mime  -  type unknown  -  opaque signed data ?" << endl;

                  if(    writeOpaqueOrMultipartSignedData( 0, *curNode )
                      && !isSigned ) {
                    kdDebug(5006) << "pkcs7 mime  -  signature found  -  opaque signed data !" << endl;
                    isSigned = true;
                  }


                  if( isSigned )
                    curNode->setSigned( true );
                }

                // we call decryption function
                // if it turned out that this is not signed mail
                if( !isSigned ) {
                  if( isEncrypted )
                    kdDebug(5006) << "pkcs7 mime     ==      S/MIME TYPE: enveloped (encrypted) data" << endl;
                  else
                    kdDebug(5006) << "pkcs7 mime  -  type unknown  -  enveloped (encrypted) data ?" << endl;
                  if( curNode->mChild ) {
kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
                    parseObjectTree( curNode->mChild );
kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
                  } else {
kdDebug(5006) << "\n----->  Initially processing encrypted data\n" << endl;
                      QCString decryptedData;
                      if( okDecryptMIME( *curNode, decryptedData ) ) {
                        isEncrypted = true;
                        DwBodyPart* myBody = new DwBodyPart( DwString( decryptedData ),
                                                            curNode->dwPart() );
                        myBody->Parse();

                        if( myBody->hasHeaders() ) {
                          DwText& desc = myBody->Headers().ContentDescription();
                          desc.FromString( "encrypted data" );
                          desc.SetModified();
                          //desc.Assemble();
                          myBody->Headers().Parse();
                        }

                        curNode->setFirstChild(
                          new partNode( false, myBody ) )->buildObjectTree( false );

                        if( curNode->mimePartTreeItem() ) {
kdDebug(5006) << "\n     ----->  Inserting items into MimePartTree\n" << endl;
                          curNode->mChild->fillMimePartTree( curNode->mimePartTreeItem(),
                                                            0,
                                                            "",   // cntDesc,
                                                            "",   // mainCntTypeStr,
                                                            "",   // cntEnc,
                                                            "" ); // cntSize );
kdDebug(5006) << "\n     <-----  Finished inserting items into MimePartTree\n" << endl;
                        } else {
kdDebug(5006) << "\n     ------  Sorry, curNode->mimePartTreeItem() returns ZERO so"
              << "\n                    we cannot insert new lines into MimePartTree. :-(\n" << endl;
                        }
                        parseObjectTree( curNode->mChild );
                      }
                      else
                      {
                        writeHTMLStr(mCodec->toUnicode( decryptedData ));
                      }
kdDebug(5006) << "\n<-----  Returning from initially processing encrypted data\n" << endl;
                  }
                  if( isEncrypted )
                    curNode->setEncrypted( true );
                }
                if( isSigned || isEncrypted )
                  bDone = true;

                useThisCryptPlug = oldUseThisCryptPlug;
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
          writePartIcon(&curNode->msgPart(), curNode->nodeId());
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
      if( !bDone ) {
        bool asIcon = true;
        switch (mAttachmentStyle)
        {
        case IconicAttmnt:
          asIcon = TRUE;
          break;
        case InlineAttmnt:
          asIcon = FALSE;
          break;
        case SmartAttmnt:
          asIcon = ( curNode->msgPart().contentDisposition().find("inline") < 0 );
        }
        if( asIcon ) {
          if( isImage )
            inlineImage = true;
          writePartIcon(&curNode->msgPart(), curNode->nodeId());
          if( isImage )
            inlineImage = false;
        } else {
          QCString cstr( curNode->msgPart().bodyDecoded() );
          writeBodyStr(cstr, mCodec, &isInlineSigned, &isInlineEncrypted);
        }
      }
      curNode->mWasProcessed = true;
    }
    // parse the siblings (children are parsed in the 'multipart' case terms)
    if( !showOneMimePart && curNode && curNode->mNext )
      parseObjectTree( curNode->mNext, showOneMimePart,
                                       keepEncryptions,
                                       includeSignatures );
    // adjust signed/encrypted flags if inline PGP was found
    if( isInlineSigned )
      curNode->setSigned( true );
    if( isInlineEncrypted )
      curNode->setEncrypted( true );
  }

  if( showOneMimePart ) {
    mViewer->write("</body></html>");
    sendNextHtmlChunk();
    /*mViewer->view()->viewport()->setUpdatesEnabled( true );
    mViewer->view()->setUpdatesEnabled( true );
    mViewer->view()->viewport()->repaint( false );*/
  }
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
    mRootNode( 0 )
{
  mAutoDelete = false;
  mLastSerNum = 0;
  mMsg = 0;
  mMsgBuf = 0;
  mMsgBufMD5 = "";
  mMsgBufSize = -1;
  mMsgDisplay = true;
  mPrinting = false;
  mShowColorbar = false;

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

  mCodec = 0;
  mAutoDetectEncoding = true;
}


//-----------------------------------------------------------------------------
KMReaderWin::~KMReaderWin()
{
  delete mViewer;  //hack to prevent segfault on exit
  if (mAutoDelete) delete mMsg;
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
     if (mMsg) {
       mMsg->readConfig();
       setMsg(mMsg, true); // Force update
     }
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

  readColorConfig();

  if (mMsg) {
    update();
    mMsg->readConfig();
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

  QString str;
  if (mPrinting) str = QString("<span style=\"color:#000000\">");
  else str = QString("<span style=\"color:%1\">").arg( color.name() );
  if( font.italic() ) { str += "<i>"; }
  if( font.bold() ) { str += "<b>"; }
  return( str );
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

  mMsg = aMsg;
  mLastSerNum = (aMsg) ? aMsg->getMsgSerNum() : 0;
  if (mMsg)
  {
    mMsg->setCodec(mCodec);
    mMsg->setDecodeHTML(htmlMail());
  }

  // Avoid flicker, somewhat of a cludge
  if (force) {
    // stop the timer to avoid calling updateReaderWin twice
    updateReaderWinTimer.stop();
    mMsgBuf = 0;
    updateReaderWin();
  }
  else if (updateReaderWinTimer.isActive())
    updateReaderWinTimer.changeInterval( delay );
  else
    updateReaderWinTimer.start( 0, TRUE );
}

//-----------------------------------------------------------------------------
void KMReaderWin::clearCache()
{
  updateReaderWinTimer.stop();
  mMsg = 0;
}

// enter items for the "new features" list here, so the main body of
// the welcome page can be left untouched (probably much easier for
// the translators). Note that the <li>...</li> tags are added
// automatically below:
static const char * const kmailNewFeatures[] = {
  I18N_NOOP("PGP/MIME (RFC 3156) support for GnuPG users"),
  I18N_NOOP("S/MIME support"),
  I18N_NOOP("Custom folder icons"),
  I18N_NOOP("Custom date format"),
  I18N_NOOP("Default identity can now be renamed"),
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
  mViewer->write(content.arg(pointsToPixel(fntSize), 0, 'f', 5).arg(info));
  mViewer->end();
}


//-----------------------------------------------------------------------------
void KMReaderWin::updateReaderWin()
{
/*  if (mMsgBuf && mMsg &&
      !mMsg->msgIdMD5().isEmpty() &&
      (mMsgBufMD5 == mMsg->msgIdMD5()) &&
      ((unsigned)mMsgBufSize == mMsg->msgSize()))
    return; */

  mMsgBuf = mMsg;
  if (mMsgBuf) {
    mMsgBufMD5 = mMsgBuf->msgIdMD5();
    mMsgBufSize = mMsgBuf->msgSize();
  }
  else {
    mMsgBufMD5 = "";
    mMsgBufSize = -1;
  }

/*  if (mMsg && !mMsg->msgIdMD5().isEmpty())
    updateReaderWinTimer.start( delay, TRUE ); */

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

  if (mMsg)
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
double KMReaderWin::pointsToPixel(int pointSize)
{
  QPaintDeviceMetrics pdm(mViewer->view());
  double pixelSize = pointSize;
  return pixelSize * pdm.logicalDpiY() / 72;
}

//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(void)
{
  if(mMsg == NULL)
    return;

  if( mMimePartTree )
    mMimePartTree->clear();

  QString type = mMsg->typeStr().lower();

  bool isMultipart = (type.find("multipart/") != -1);

  if( mMimePartTree && mShowMIMETreeMode && (1 == *mShowMIMETreeMode) ) {
    if( isMultipart )
      mMimePartTree->show();
    else
      mMimePartTree->hide();
  }

  QString bkgrdStr = "";
  if (mBackingPixmapOn)
    bkgrdStr = " background=\"file://" + mBackingPixmapStr + "\"";

  mViewer->begin( KURL( "file:/" ) );

  if (mAutoDetectEncoding) {
    mCodec = 0;
    QCString encoding;
    if (type.find("text/") != -1)
      encoding = mMsg->charset();
    else if ( isMultipart ) {
      if (mMsg->numBodyParts() > 0) {
        KMMessagePart msgPart;
        mMsg->bodyPart(0, &msgPart);
        encoding = msgPart.charset();
      }
    }
    if (encoding.isEmpty())
      encoding = kernel->networkCodec()->name();
    mCodec = KMMsgBase::codecForName(encoding);
  }

  if (!mCodec)
    mCodec = QTextCodec::codecForName("iso8859-1");
  mMsg->setCodec(mCodec);

  QColorGroup cg = kapp->palette().active();
  queueHtml("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 "
	    "Transitional//EN\">\n<html><head><title></title>"
	    "<style type=\"text/css\">" +
    ((mPrinting) ? QString("body { font-family: \"%1\"; font-size: %2pt; "
                           "color: #000000; background-color: #FFFFFF; }\n")
        .arg( mBodyFamily ).arg( fntSize )
      : QString("body { font-family: \"%1\"; font-size: %2px; "
        "color: %3; background-color: %4; }\n")
        .arg( mBodyFamily ).arg( pointsToPixel(fntSize), 0, 'f', 5 )
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
    setCaption(mMsg->subject());

  parseMsg(mMsg);

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


bool KMReaderWin::writeOpaqueOrMultipartSignedData( partNode* data, partNode& sign )
{
  bool bIsOpaqueSigned = false;

  if( data )
    parseObjectTree( data );

  CryptPlugWrapper* cryptPlug = useThisCryptPlug ? useThisCryptPlug : mCryptPlugList->active();
  if( cryptPlug ) {
    if( data )
      kdDebug(5006) << "\nKMReaderWin::writeOpaqueOrMultipartSignedData: processing SMIME Signed data" << endl;
    else
      kdDebug(5006) << "\nKMReaderWin::writeOpaqueOrMultipartSignedData: processing Opaque Signed data" << endl;

    kdDebug(5006) << "\nKMReaderWin::writeOpaqueOrMultipartSignedData: going to call CRYPTPLUG "
                  << cryptPlug->libName() << endl;

    bool oldCryptPlugActiveFlag;
    if( !cryptPlug->active() ) {
      if( !cryptPlug->initStatus( 0 ) == CryptPlugWrapper::InitStatus_Ok ) {
        KMessageBox::sorry(this,
          i18n("Crypto Plug-In \"%s"
               "\" is not initialized.\n"
               "Please specify the Plug-In using the 'Settings/Configure KMail / Plug-In' dialog.").arg(cryptPlug->libName()));
        return false;
      }
      oldCryptPlugActiveFlag = cryptPlug->active();
      cryptPlug->setActive( true );
    } else
      oldCryptPlugActiveFlag = true;

    QCString cleartext;
    char* new_cleartext;
    if( data )
      cleartext = data->dwPart()->AsString().c_str();
    else
      new_cleartext = 0;

    // the following code runs only for S/MIME
    if( data && (0 <= cryptPlug->libName().find( "smime", 0, false )) ) {
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
    bool signatureIsBinary = (-1 == signatureStr.find("BEGIN SIGNED MESSAGE", 0, false) );
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
    sigMeta.extended_info_count = 0;
    sigMeta.nota_xml            = 0;

    // PENDING(khz) Should we distinguish between an invalid signature
    // and the incapability of the plugin to verify the signature?
    const char* cleartextP = cleartext;
    bool bSignatureOk = cryptPlug->hasFeature( Feature_VerifySignatures ) &&
                        cryptPlug->checkMessageSignature(
                          data ? const_cast<char**>(&cleartextP)
                               : &new_cleartext,
                          signaturetext,
                          signatureIsBinary,
                          signatureLen,
                          &sigMeta );

    kdDebug(5006) << "\nKMReaderWin::writeOpaqueOrMultipartSignedData: returned from CRYPTPLUG" << endl;

    QString txt;
    QString unknown( i18n("(unknown)") );
    if( !data ){
      if( new_cleartext ) {
        queueHtml( new_cleartext );
        deb = "\n\nN E W    C O N T E N T = \"";
        deb += new_cleartext;
        deb += "\"  <--  E N D    O F    N E W    C O N T E N T\n\n";
        kdDebug(5006) << deb << endl;
        delete new_cleartext;
        bIsOpaqueSigned = true;
      } else {
        txt = "<hr><b><h2>";
        txt.append( i18n( "The crypto engine returned no cleartext data!" ) );
        txt.append( "</h2></b>" );
        txt.append( "<br>&nbsp;<br>" );
        txt.append( i18n( "Status: " ).local8Bit() );
        if( sigMeta.status && 0 < strlen(sigMeta.status) ) {
          txt.append( "<i>" );
          txt.append( sigMeta.status );
          txt.append( "</i>" );
        }
        else
          txt.append( unknown );
        queueHtml(txt);
      }
    }

    if( bSignatureOk ) {
      txt = "<hr><b>";
      txt.append( i18n( "Signature is OK." ).local8Bit() );
      txt.append( "</b><br>&nbsp;<br>" );
      txt.append( i18n( "Status: " ).local8Bit() );
      if( sigMeta.status && 0 < strlen(sigMeta.status) ) {
        txt.append( "<i>" );
        txt.append( sigMeta.status );
        txt.append( "</i>" );
      }
      else
        txt.append( unknown );
      txt.append( "<br>&nbsp;<br>" );
      txt.append( i18n( "Signature key information:" ).local8Bit() );
      txt.append( "<br>" );
      if( 0 < sigMeta.extended_info_count ) {
        txt.append( "<table border=1><tr><td>" );
        txt.append( i18n( "<u>created</u>" ).local8Bit() );
        txt.append( "</td><td>" );
        txt.append( i18n( "<u>status</u>" ).local8Bit() );
        txt.append( "</td><td>" );
        txt.append( i18n( "<u>fingerprint</u>" ).local8Bit() );
        txt.append( "</td></tr>" );
        for( int i=0; i<sigMeta.extended_info_count; ++i ) {
          CryptPlugWrapper::SignatureMetaDataExtendedInfo& ext = sigMeta.extended_info[i];
          // soll:
          // txt.append( QString("<tr><td>%1<td/><td>").arg( ext.creation_time ).latin1() );
          // ist:
             txt.append( "<tr><td><i>?</i></td>" );

          txt.append( QString(    "<td><i>%1</i></td>").arg( ext.status_text ).latin1() );
          txt.append( QString(    "<td><i>%1</i></td></tr>").arg( ext.fingerprint ).latin1() );
        }
        txt.append( "</table>" );
      }
      else
        txt.append( unknown );
      txt.append( "<br><u>Notation (XML):</u><br>" );
      if( sigMeta.nota_xml && 0 < sigMeta.nota_xml ) {
        txt.append( "<i><pre>" );
        txt.append( sigMeta.nota_xml );
        txt.append( "</pre></i>" );
      }
      else
        txt.append( unknown );
      cryptPlug->freeSignatureMetaData( &sigMeta );

      queueHtml(txt);
    }
    else {
      txt = "<hr><b><h2>";
      txt.append( i18n( "Signature could <em>not</em> be verified!" ) );
      txt.append( "</h2></b>" );
      txt.append( "<br>&nbsp;<br>" );
      txt.append( i18n( "Status: " ).local8Bit() );
      if( sigMeta.status && 0 < strlen(sigMeta.status) ) {
        txt.append( "<i>" );
        txt.append( sigMeta.status );
        txt.append( "</i>" );
      }
      else
        txt.append( unknown );
      queueHtml(txt);
    }
    cryptPlug->setActive( oldCryptPlugActiveFlag );
  } else {
    KMessageBox::information(this,
			     i18n("problem: No Crypto Plug-Ins found.\n"
				  "Please specify a Plug-In using the 'Settings/Configure KMail / Plug-In' dialog."),
			     QString::null, "cryptoPluginBox");
    queueHtml(i18n("<hr><b><h2>Signature could *not* be verified !</h2></b><br>"
                   "reason:<br><i>&nbsp; &nbsp; No Crypto Plug-Ins found.</i><br>"
                   "proposal:<br><i>&nbsp; &nbsp; Please specify a Plug-In by invoking<br>&nbsp; &nbsp; the "
                   "'Settings/Configure KMail / Plug-In' dialog!</i>"));
  }
  return bIsOpaqueSigned;
}


bool KMReaderWin::okDecryptMIME( partNode& data, QCString& decryptedData )
{
  const QString errorContentCouldNotBeDecrypted( i18n("Content could *not* be decrypted.") );

  bool bDecryptionOk = false;
  CryptPlugWrapper* cryptPlug = useThisCryptPlug ? useThisCryptPlug : mCryptPlugList->active();
  if( cryptPlug ) {
    QByteArray ciphertext( data.msgPart().bodyDecodedBinary() );
    QCString cipherStr( ciphertext );
    bool cipherIsBinary = (-1 == cipherStr.find("BEGIN ENCRYPTED MESSAGE", 0, false) );
    int cipherLen = ciphertext.size();
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

    if( ! cryptPlug->hasFeature( Feature_DecryptMessages ) ) {
      showMessageAndSetData( errorContentCouldNotBeDecrypted,
        i18n("Crypto Plug-In %1 can not decrypt any messages.").arg(cryptPlug->libName()),
        i18n("Please specify a *matching* Plug-In by invoking"),
        i18n("the 'Settings/Configure KMail / Plug-In' dialog!"),
        decryptedData );
    } else {
      kdDebug() << "\nKMReaderWin::decryptMIME: going to call CRYPTPLUG "
                << cryptPlug->libName() << endl;
      bDecryptionOk = cryptPlug->decryptMessage( ciphertext,
                                                 cipherIsBinary,
                                                 cipherLen,
                                                 &cleartext,
                                                 certificate );
      kdDebug() << "\nKMReaderWin::decryptMIME: returned from CRYPTPLUG" << endl;
      if( bDecryptionOk )
        decryptedData = cleartext;
      else {
        showMessageAndSetData( errorContentCouldNotBeDecrypted,
          i18n("Crypto Plug-In %1 could not decrypt the data.").arg(cryptPlug->libName()),
          i18n("Make sure the Plug-In is installed properly and check your"),
          i18n("specifications made in the 'Settings/Configure KMail / Plug-In' dialog!"),
          decryptedData );
      }
    }

    delete cleartext;

  } else {
      showMessageAndSetData( errorContentCouldNotBeDecrypted,
        i18n("No Crypto Plug-In settings found.").utf8(),
        i18n("Please specify a Plug-In by invoking").utf8(),
        i18n("the 'Settings/Configure KMail / Plug-In' dialog!").utf8(),
        decryptedData );
  }
  return bDecryptionOk;
}



//-----------------------------------------------------------------------------
void KMReaderWin::parseMsg(KMMessage* aMsg)
{
  removeTempFiles();
  KMMessagePart msgPart;
  int numParts;
  QCString type, subtype, contDisp;
  QByteArray str;

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

  assert( aMsg != NULL );

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
  if( mRootNode ) delete mRootNode;
  mRootNode = new partNode( mainBody ? mainBody : 0,
                            mainType,
                            mainSubType );

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
    if( mMimePartTree )
      mRootNode->fillMimePartTree( 0,
                                   mMimePartTree,
                                   cntDesc,
                                   mainCntTypeStr,
                                   cntEnc,
                                   cntSize );
  } else if( mMimePartTree ) {
kdDebug(5006) << "\n     ----->  Inserting Root Node into the Mime Part Tree" << endl;
    mRootNode->fillMimePartTree( 0,
                                 mMimePartTree,
                                 cntDesc,
                                 mainCntTypeStr,
                                 cntEnc,
                                 cntSize );
    /*new KMMimePartTreeItem( *mMimePartTree, mRootNode,
                                            cntDesc,
                                            mainCntTypeStr,
                                             cntEnc,
                                            cntSize );*/
kdDebug(5006) << "\n     <-----  Finished inserting Root Node into Mime Part Tree" << endl;
  } else {
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

  queueHtml("<div id=\"header\" style=\"margin-bottom: 1em;\">"
          + (writeMsgHeader(hasVCard))
          + "</div>");


  // show message content
  parseObjectTree( mRootNode );

  // store encrypted/signed status information in the KMMessage
  //  - this can only be done *after* calling parseObjectTree()
  aMsg->setEncryptionState( mRootNode->overallEncryptionState() );
  aMsg->setSignatureState(  mRootNode->overallSignatureState()  );

  // remove temp. Body Part that was created for a single part mail
  if( mainBody )
    delete mainBody;
  // remove temp. CryptPlugList
  if( tmpPlugList )
    delete mCryptPlugList;



/*
  old code *before* AEGYPTEN_BRANCH merger:


  if (numParts > 0)
  {
    // ---sven: handle multipart/alternative start ---
    // This is for multipart/alternative messages WITHOUT attachments
    // main header has type=multipart/alternative and one attachment is
    // text/html
    if (type.find("multipart/alternative") != -1 && numParts == 2)
    {
      kdDebug(5006) << "Alternative message, type: " << type << endl;
      //Now: Only two attachments one of them is html
      for (i=0; i<2; i++)                   // count parts...
      {
        aMsg->bodyPart(i, &msgPart);        // set part...
        subtype = msgPart.subtypeStr();     // get subtype...
        if (htmlMail() && qstricmp(subtype, "html")==0)    // is it html?
        {                                   // yes...
          str = msgPart.bodyDecoded();      // decode it...
          //mViewer->write(mCodec->toUnicode(str.data()));    // write it...
          writeHTMLStr(mCodec->toUnicode(str.data()));
          return;                           // return, finshed.
        }
	else if (!htmlMail() && (qstricmp(subtype, "plain")==0))
	                                    // wasn't html show only if
	{                                   // support for html is turned off
          str = msgPart.bodyDecoded();      // decode it...
          writeBodyStr(str.data(), mCodec);
          return;
	}
      }
      // if we are here we didnt find any html part. Handle it normaly then
    }
    // This works only for alternative msgs without attachments. Alternative
    // messages with attachments are broken with or without this. No need
    // to bother with strib </body> or </html> here, because if any part
    // follows this will not be shown correctly. You'll still be able to read the
    // main message and deal with attachments. Nothing I can do now :-(
    // ---sven: handle multipart/alternative end ---

    for (i=0; i<numParts; i++)
    {
      aMsg->bodyPart(i, &msgPart);
      type = msgPart.typeStr();
      subtype = msgPart.subtypeStr();
      contDisp = msgPart.contentDisposition();
      kdDebug(5006) << "type: " << type.data() << endl;
      kdDebug(5006) << "subtye: " << subtype.data() << endl;
      kdDebug(5006) << "contDisp " << contDisp.data() << endl;

      if (i <= 0) asIcon = FALSE;
      else switch (mAttachmentStyle)
      {
      case IconicAttmnt:
        asIcon=TRUE; break;
      case InlineAttmnt:
        asIcon=FALSE; break;
      case SmartAttmnt:
        asIcon=(contDisp.find("inline")<0);
      }

      if (!asIcon)
      {
//	if (i<=0 || qstricmp(type, "text")==0)//||qstricmp(type, "message")==0)
//	if (qstricmp(type, "text")==0)//||qstricmp(type, "message")==0)
	if ((type == "") || (qstricmp(type, "text")==0))
	{
          QCString cstr(msgPart.bodyDecoded());
	  if (i>0)
	      queueHtml("<br><hr><br>");

          QTextCodec *atmCodec = (mAutoDetectEncoding) ?
            KMMsgBase::codecForName(msgPart.charset()) : mCodec;
          if (!atmCodec) atmCodec = mCodec;

	  if (htmlMail() && (qstricmp(subtype, "html")==0))
          {
            // ---Sven's strip </BODY> and </HTML> from end of attachment start-
            // We must fo this, or else we will see only 1st inlined html attachment
            // It is IMHO enough to search only for </BODY> and put \0 there.
            int i;
            i = cstr.findRev("</body>", -1, false); //case insensitive
            if (i>0)
              cstr.truncate(i);
            else // just in case - search for </html>
            {
              i = cstr.findRev("</html>", -1, false); //case insensitive
              if (i>0) cstr.truncate(i);
            }
            // ---Sven's strip </BODY> and </HTML> from end of attachment end-
            //mViewer->write(atmCodec->toUnicode(cstr.data()));
            writeHTMLStr(atmCodec->toUnicode(cstr.data()));
	  }
          else writeBodyStr(cstr, atmCodec);
	}
        // ---Sven's view smart or inline image attachments in kmail start---
        else if (qstricmp(type, "image")==0)
        {
          inlineImage=true;
          writePartIcon(&msgPart, i);
          inlineImage=false;
        }
        // ---Sven's view smart or inline image attachments in kmail end---
	else asIcon = TRUE;
      }
      if (asIcon)
      {
        writePartIcon(&msgPart, i);
      }
    }
  }
  else // if numBodyParts <= 0
  {
    if (htmlMail() && ((type == "text/html") || (type.find("text/html") != -1)))
      //mViewer->write(mCodec->toUnicode(aMsg->bodyDecoded().data()));
      writeHTMLStr(mCodec->toUnicode(aMsg->bodyDecoded().data()));
    else
      writeBodyStr(aMsg->bodyDecoded(), mCodec);
  }
*/
}


//-----------------------------------------------------------------------------
QString KMReaderWin::writeMsgHeader(bool hasVCard)
{
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
   if (!mMsg->subject().isEmpty()) {
      subjectDir = (KMMsgBase::skipKeyword(mMsg->subject())
                         .isRightToLeft()) ? "rtl" : "ltr";
   } else
      subjectDir = i18n("No Subject").isRightToLeft() ? "rtl" : "ltr";


  if (hasVCard) vcname = mTempFiles.last();

  switch (mHeaderStyle)
  {
  case HdrBrief:
    headerStr += QString("<div dir=\"%1\"><b style=\"font-size:130%\">" +
                        strToHtml(mMsg->subject()) +
                        "</b>&nbsp; (" +
                        KMMessage::emailAddrAsAnchor(mMsg->from(),TRUE) + ", ")
                        .arg(subjectDir);

    if (!mMsg->cc().isEmpty())
    {
      headerStr.append(i18n("Cc: ")+
                       KMMessage::emailAddrAsAnchor(mMsg->cc(),TRUE) + ", ");
    }

    headerStr.append("&nbsp;"+strToHtml(mMsg->dateShortStr()) + ")");

    if (hasVCard)
    {
      headerStr.append("&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }

    headerStr.append("</div>");
    break;

  case HdrStandard:
    headerStr += QString("<div dir=\"%1\"><b style=\"font-size:130%\">" +
                        strToHtml(mMsg->subject()) + "</b></div>")
                        .arg(subjectDir);
    headerStr.append(i18n("From: ") +
                     KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE));
    if (hasVCard)
    {
      headerStr.append("&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>");
    }
    headerStr.append("<br>");
    headerStr.append(i18n("To: ") +
                     KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<br>");
    if (!mMsg->cc().isEmpty())
      headerStr.append(i18n("Cc: ")+
                       KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<br>");
    break;

  case HdrFancy:
  {
    // the subject line and box below for details
    headerStr += QString("<div class=\"fancyHeaderSubj\" dir=\"%1\">"
                        "<b>%2</b></div>"
                        "<div class=\"fancyHeaderDtls\">"
                        "<table class=\"fancyHeaderDtls\">")
                        .arg(subjectDir)
		        .arg(mMsg->subject().isEmpty()?
			     i18n("No Subject") :
			     strToHtml(mMsg->subject()));

    // from line
    headerStr.append(QString("<tr><th class=\"fancyHeaderDtls\">%1</th><td class=\"fancyHeaderDtls\">%2%3%4</td></tr>")
                            .arg(i18n("From: "))
                            .arg(KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE))
                            .arg(hasVCard ?
                                 "&nbsp;&nbsp;<a href=\""+vcname+"\">"+i18n("[vCard]")+"</a>"
                                 : "")
                            .arg((mMsg->headerField("Organization").isEmpty()) ?
                                 ""
                                 : "&nbsp;&nbsp;(" +
                                   strToHtml(mMsg->headerField("Organization")) +
                                   ")"));

    // to line
    headerStr.append(QString("<tr><th class=\"fancyHeaderDtls\">%1</th><td class=\"fancyHeaderDtls\">%2</td></tr>")
                            .arg(i18n("To: "))
                            .arg(KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE)));

    // cc line, if any
    if (!mMsg->cc().isEmpty())
    {
      headerStr.append(QString("<tr><th class=\"fancyHeaderDtls\">%1</th><td class=\"fancyHeaderDtls\">%2</td></tr>")
                              .arg(i18n("Cc: "))
                              .arg(KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE)));
    }

    // the date
    headerStr.append(QString("<tr><th class=\"fancyHeaderDtls\">%1</th><td dir=\"%2\" class=\"fancyHeaderDtls\">%3</td></tr>")
                            .arg(i18n("Date: "))
			    .arg(mMsg->dateStr().isRightToLeft() ? "rtl" : "ltr")
                            .arg(strToHtml(mMsg->dateStr())));
    headerStr.append("</table></div>");
    break;
  }
  case HdrLong:
    headerStr += QString("<div dir=\"%1\"><b style=\"font-size:130%\">" +
                        strToHtml(mMsg->subject()) + "</b></div>")
                        .arg(subjectDir);
    headerStr.append(i18n("Date: ") + strToHtml(mMsg->dateStr())+"<br>");
    headerStr.append(i18n("From: ") +
                     KMMessage::emailAddrAsAnchor(mMsg->from(),FALSE));
    if (hasVCard)
    {
      headerStr.append("&nbsp;&nbsp;<a href=\"" +
                       vcname +
                       "\">"+i18n("[vCard]")+"</a>");
    }

    if (!mMsg->headerField("Organization").isEmpty())
    {
      headerStr.append("&nbsp;&nbsp;(" +
                       strToHtml(mMsg->headerField("Organization")) + ")");
    }

    headerStr.append("<br>");
    headerStr.append(i18n("To: ")+
                   KMMessage::emailAddrAsAnchor(mMsg->to(),FALSE) + "<br>");
    if (!mMsg->cc().isEmpty())
    {
      headerStr.append(i18n("Cc: ")+
                       KMMessage::emailAddrAsAnchor(mMsg->cc(),FALSE) + "<br>");
    }

    if (!mMsg->bcc().isEmpty())
    {
      headerStr.append(i18n("Bcc: ")+
                       KMMessage::emailAddrAsAnchor(mMsg->bcc(),FALSE) + "<br>");
    }

    if (!mMsg->replyTo().isEmpty())
    {
      headerStr.append(i18n("Reply to: ")+
                     KMMessage::emailAddrAsAnchor(mMsg->replyTo(),FALSE) + "<br>");
    }
    break;

  case HdrAll:
      // we force the direction to ltr here, even in a arabic/hebrew UI,
      // as the headers are almost all Latin1
    headerStr += "<div dir=\"ltr\">";
    headerStr += strToHtml(mMsg->headerAsString(), true);
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


//-----------------------------------------------------------------------------
void KMReaderWin::writeBodyStr( const QCString aStr, QTextCodec *aCodec,
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
    QString signer;
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
          bool couldDecrypt = block->decrypt();
          isEncrypted = block->isEncrypted();
          if( isEncrypted )
          {
            htmlStr += "<table cellspacing=\"1\" cellpadding=\"0\" class=\"encr\">"
                       "<tr class=\"encrH\"><td dir=\"" + dir + "\">";
            if( couldDecrypt )
              htmlStr += i18n("Encrypted message");
            else
              htmlStr += QString("%1<br />%2")
                .arg(i18n("Cannot decrypt message:"))
                .arg(pgp->lastErrorMsg());
            htmlStr += "</td></tr><tr class=\"encrB\"><td>";
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
          signer = block->signatureUserId();
          if( signer.isEmpty() )
          {
            signClass = "signWarn";
            htmlStr += "<table cellspacing=\"1\" cellpadding=\"0\" "
                       "class=\"" + signClass + "\">"
                       "<tr class=\"" + signClass + "H\"><td dir=\"" + dir + "\">";
            htmlStr += i18n( "Message was signed with unknown key 0x%1." )
                       .arg( block->signatureKeyId() );
            htmlStr += "<br />";
            htmlStr += i18n( "The validity of the signature can't be "
                             "verified." );
            htmlStr += "</td></tr><tr class=\"" + signClass + "B\"><td>";
          }
          else
          {
            goodSignature = block->goodSignature();

            QCString keyId = block->signatureKeyId();
            Kpgp::Validity keyTrust;
            if( !keyId.isEmpty() )
              keyTrust = pgp->keyTrust( keyId );
            else
              // This is needed for the PGP 6 support because PGP 6 doesn't
              // print the key id of the signing key if the key is known.
              keyTrust = pgp->keyTrust( signer );

            // HTMLize the signer's user id and create mailto: link
            signer.replace( QRegExp("&"), "&amp;" );
            signer.replace( QRegExp("<"), "&lt;" );
            signer.replace( QRegExp(">"), "&gt;" );
            signer.replace( QRegExp("\""), "&quot;" );
            signer = "<a href=\"mailto:" + signer + "\">" + signer + "</a>";

            if( goodSignature )
            {
              if( keyTrust < Kpgp::KPGP_VALIDITY_MARGINAL )
                signClass = "signOkKeyBad";
              else
                signClass = "signOkKeyOk";
              htmlStr += "<table cellspacing=\"1\" cellpadding=\"0\" "
                         "class=\"" + signClass + "\">"
                         "<tr class=\"" + signClass + "H\"><td dir=\"" + dir + "\">";
              if( !keyId.isEmpty() )
                htmlStr += i18n( "Message was signed by %1 (Key ID: 0x%2)." )
                           .arg( signer )
                           .arg( keyId );
              else
                htmlStr += i18n( "Message was signed by %1." ).arg( signer );
              htmlStr += "<br />";
              switch( keyTrust )
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
                         "<tr class=\"" + signClass + "B\"><td>";
            }
            else
            {
              signClass = "signErr";
              htmlStr += "<table cellspacing=\"1\" cellpadding=\"0\" "
                         "class=\"" + signClass + "\">"
                         "<tr class=\"" + signClass + "H\"><td dir=\"" + dir + "\">";
              if( !keyId.isEmpty() )
                htmlStr += i18n( "Message was signed by %1 (Key ID: 0x%2)." )
                           .arg( signer )
                           .arg( keyId );
              else
                htmlStr += i18n( "Message was signed by %1." ).arg( signer );
              htmlStr += "<br />";
              htmlStr += i18n("Warning: The signature is bad.");
              htmlStr += "</td></tr>"
                         "<tr class=\"" + signClass + "B\"><td>";
            }
          }
        }

        htmlStr += quotedHTML( aCodec->toUnicode( block->text() ) );

        if( isSigned ) {
          htmlStr += "</td></tr><tr class=\"" + signClass + "H\">";
          htmlStr += "<td dir=\"" + dir + "\">" +
                     i18n( "End of signed message" ) +
                     "</td></tr></table>";
          if( flagSigned )
            *flagSigned = true;
        }
        if( isEncrypted ) {
          htmlStr += "</td></tr><tr class=\"encrH\"><td dir=\"" + dir + "\">" +
                     i18n( "End of encrypted message" ) +
                     "</td></tr></table>";
          if( flagEncrypted )
            *flagEncrypted = true;
        }
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

  if( isPgpMessage )
  {
    mColorBar->setEraseColor( cCBpgp );
    mColorBar->setText(i18n("\nP\nG\nP\n \nM\ne\ns\ns\na\ng\ne"));
  }
  else
  {
    mColorBar->setEraseColor( cCBplain );
    mColorBar->setText(i18n("\nN\no\n \nP\nG\nP\n \nM\ne\ns\ns\na\ng\ne"));
  }
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
  QString htmlStr, line;
  QString normalStartTag;
  QString normalEndTag;
  QString quoteEnd("</span>");

  unsigned int pos, beg;
  unsigned int length = s.length();
  enum { First, New, Mid } paragState = First;
  bool rightToLeft = false;

  if (mBodyFont.bold()) { normalStartTag += "<b>"; normalEndTag += "</b>"; }
  if (mBodyFont.italic()) { normalStartTag += "<i>"; normalEndTag += "</i>"; }

  // skip leading empty lines
  for( pos = 0; pos < length && s[pos] <= ' '; pos++ );
  while (pos > 0 && (s[pos-1] == ' ' || s[pos-1] == '\t')) pos--;
  beg = pos;

  int currQuoteLevel = -1;

  while (beg<length)
  {
    /* search next occurance of '\n' */
    pos = s.find('\n', beg, FALSE);
    if (pos == (unsigned int)(-1))
	pos = length;

    line = s.mid(beg,pos-beg);
    beg = pos+1;

    /* calculate line's current quoting depth */
    unsigned int p;
    int actQuoteLevel = -1;
    bool finish = FALSE;
    for (p=0; p<line.length() && !finish; p++) {
	switch (line[p].latin1()) {
	    /* case ':': */
	    case '>':
	    case '|':	actQuoteLevel++;
			break;
	    case ' ':
	    case '\t':
	    case '\r':  break;
	    default:	finish = TRUE;
			break;
	}
    } /* for() */

    if ( (paragState != Mid && finish) || (actQuoteLevel != currQuoteLevel) ) {

	/* finish last quotelevel */
	if (currQuoteLevel == -1)
	    htmlStr.append( normalEndTag );
	else
	    htmlStr.append( quoteEnd );

	if ( paragState == New )
            htmlStr += "<br>";

	/* start new quotelevel */
	currQuoteLevel = actQuoteLevel;
	if (actQuoteLevel == -1)
	    htmlStr += normalStartTag;
	else
	    htmlStr += mQuoteFontTag[currQuoteLevel%3];

	paragState = Mid;
    }

    if ( !finish && paragState == Mid )
	paragState = New;

    rightToLeft = line.isRightToLeft();
    line = strToHtml(line, TRUE);

    htmlStr+= QString("<div dir=\"%1\">" ).arg(rightToLeft ? "rtl" : "ltr");
    htmlStr+= (line);
    htmlStr+= QString("</div>");
  } /* while() */

  /* really finish the last quotelevel */
  if (currQuoteLevel == -1)
     htmlStr.append( normalEndTag );
  else
     htmlStr.append( quoteEnd );

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
  if (inlineImage)
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
  if (!mMsg) return;

  if (mPrinting)
    mViewer->view()->print();
  else {
    KMReaderWin printWin;
    printWin.setPrinting(TRUE);
    printWin.readConfig();
    printWin.setMsg(mMsg, TRUE);
    printWin.printMsg();
  }
}


//-----------------------------------------------------------------------------
int KMReaderWin::msgPartFromUrl(const KURL &aUrl)
{
  if (aUrl.isEmpty() || !mMsg) return -1;

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
void KMReaderWin::closeEvent(QCloseEvent *e)
{
  KMReaderWinInherited::closeEvent(e);
  writeConfig();
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOn(const QString &aUrl)
{
  bool bOk = false;

  KURL url(aUrl);
  int id = msgPartFromUrl(url);

  if (id > 0)
  {
    // KMMessagePart msgPart;
    // mMsg->bodyPart(id, &msgPart);
    partNode* node = mRootNode ? mRootNode->findId( id ) : 0;
    if( node ) {
      KMMessagePart& msgPart = node->msgPart();
      QString str = msgPart.fileName();
      if (str.isEmpty()) str = msgPart.name();
      emit statusMsg(i18n("Attachment: ") + str);
      bOk = true;
    }
  }
  if( !bOk )
    emit statusMsg(aUrl);
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlOpen(const KURL &aUrl, const KParts::URLArgs &)
{
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
  if (!mMsg) return;
  KURL url( aUrl );

  int id = msgPartFromUrl(url);
  if (id <= 0)
  {
    emit popupMenu(*mMsg, url, aPos);
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
      else  // plain text
	win->writeBodyStr(str, win->codec());
      win->queueHtml("</body></html>");
      win->sendNextHtmlChunk();
      win->setCaption(i18n("View Attachment: ") + pname);
      win->show();
    }
    else if (qstricmp(aMsgPart->typeStr(), "image")==0)
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
//  KMMessagePart msgPart;
//  mMsg->bodyPart(mAtmCurrent, &msgPart);
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

//  KMMessagePart msgPart;
//  mMsg->bodyPart(mAtmCurrent, &msgPart);
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

//  KMMessagePart msgPart;
//  mMsg->bodyPart(mAtmCurrent, &msgPart);


/*
  partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
  if( node ) {
    KMMessagePart& msgPart = node->msgPart();
*/
    KURL::List lst;
    KURL url;
    url.setPath(mAtmCurrentName);
    lst.append(url);
    KRun::displayOpenWithDialog(lst);
/*  }*/
}


//-----------------------------------------------------------------------------
void KMReaderWin::slotAtmSave()
{
  QString fileName;
  fileName = mSaveAttachDir;

//  KMMessagePart msgPart;
//  mMsg->bodyPart(mAtmCurrent, &msgPart);

  partNode* node = mRootNode ? mRootNode->findId( mAtmCurrent ) : 0;
  if( node ) {
    KMMessagePart& msgPart = node->msgPart();

    if (!msgPart.fileName().isEmpty())
      fileName.append(msgPart.fileName());
    else
      fileName.append(msgPart.name());

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
//  KMMessagePart msgPart;
//  mMsg->bodyPart(mAtmCurrent, &msgPart);
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
  if (mMsg) mMsg->setDecodeHTML(htmlMail());
}


//-----------------------------------------------------------------------------
bool KMReaderWin::htmlMail()
{
  return ((mHtmlMail && !mHtmlOverride) || (!mHtmlMail && mHtmlOverride));
}

//-----------------------------------------------------------------------------
#include "kmreaderwin.moc"
