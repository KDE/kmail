/*  -*- c++ -*-
    objecttreeparser.cpp

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

// my header file
#include "objecttreeparser.h"

// other KMail headers 
#include "kmreaderwin.h"
#include "partNode.h"
#include "kmgroupware.h"
#include "kmkernel.h"
#include "partmetadata.h"
using KMail::PartMetaData;

// other module headers (none)
#include <mimelib/enum.h>
#include <mimelib/bodypart.h>
#include <mimelib/string.h>
#include <mimelib/text.h>

// other KDE headers
#include <kdebug.h>
#include <klocale.h>
#include <khtml_part.h>
#include <khtmlview.h>
#include <kmessagebox.h>

// other Qt headers
#include <qlabel.h>
#include <qtextcodec.h>
#include <qregexp.h>
#include <qfile.h>

// other headers


namespace KMail {

  void ObjectTreeParser::insertAndParseNewChildNode( KMReaderWin* reader,
						     QCString* resultStringPtr,
						     CryptPlugWrapper*     useThisCryptPlug,
						     partNode& startNode,
						     const char* content,
						     const char* cntDesc,
						     bool append )
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

    partNode* parentNode = &startNode;
    partNode* newNode = new partNode(false, myBody);
    if( append && parentNode->mChild ){
      parentNode = parentNode->mChild;
      while( parentNode->mNext )
	parentNode = parentNode->mNext;
      newNode = parentNode->setNext( newNode );
    }else
      newNode = parentNode->setFirstChild( newNode );
    newNode->buildObjectTree( false );

    if( startNode.mimePartTreeItem() ) {
      kdDebug(5006) << "\n     ----->  Inserting items into MimePartTree\n" << endl;
      newNode->fillMimePartTree( startNode.mimePartTreeItem(), 0,
				 QString::null, QString::null, QString::null, 0,
				 append );
      kdDebug(5006) << "\n     <-----  Finished inserting items into MimePartTree\n" << endl;
    } else {
      kdDebug(5006) << "\n     ------  Sorry, node.mimePartTreeItem() returns ZERO so"
		    << "\n                    we cannot insert new lines into MimePartTree. :-(\n" << endl;
    }
    kdDebug(5006) << "\n     ----->  Now parsing the MimePartTree\n" << endl;
    parseObjectTree( reader,
		     resultStringPtr,
		     useThisCryptPlug,
		     newNode );// showOneMimePart, keepEncryptions, includeSignatures );
    kdDebug(5006) << "\n     <-----  Finished parsing the MimePartTree in insertAndParseNewChildNode()\n" << endl;
  }


  void ObjectTreeParser::parseObjectTree( KMReaderWin* reader,
					  QCString* resultStringPtr,
					  CryptPlugWrapper*     useThisCryptPlug,
					  partNode* node,
					  bool showOneMimePart,
					  bool keepEncryptions,
					  bool includeSignatures )
  {
    kdDebug(5006) << "\n**\n** ObjectTreeParser::parseObjectTree( "
		  << (node ? "node OK, " : "no node, ")
		  << "showOneMimePart: " << (showOneMimePart ? "TRUE" : "FALSE")
		  << " ) **\n**" << endl;

    // make widgets visible that might have been hidden by
    // previous groupware activation
    if( reader && reader->mUseGroupware )
      emit reader->signalGroupwareShow( false );

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

    if( showOneMimePart && reader ) {
      // clear the viewer
      reader->mViewer->view()->setUpdatesEnabled( false );
      reader->mViewer->view()->viewport()->setUpdatesEnabled( false );
      static_cast<QScrollView *>(reader->mViewer->widget())->ensureVisible(0,0);

      if (reader->mHtmlTimer.isActive()) {
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
    bool bNeverDisplayInline = false;

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
        bool signatureFound;
        struct CryptPlugWrapper::SignatureMetaData sigMeta;
        sigMeta.status              = 0;
        sigMeta.extended_info       = 0;
        sigMeta.extended_info_count = 0;
        sigMeta.nota_xml            = 0;
        if( okDecryptMIME( *plugForDecrypting,
                           versionPart,
                           *encryptedDataPart,
                           signatureFound,
                           sigMeta,
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

	int curNode_replacedType    = curNode->type();
	int curNode_replacedSubType = curNode->subType();
	// In order to correctly recognoze clearsigned data we threat the old
	// "Content-Type=application/pgp" like plain text.
	// Note: This does not cover "application/pgp-signature" nor
	//                    "application/pgp-encrypted".  (khz, 2002/08/28)
	if( DwMime::kTypeApplication       == curNode->type() &&
	    DwMime::kSubtypePgpClearsigned == curNode->subType() ){
	  curNode_replacedType    = DwMime::kTypeText;
	  curNode_replacedSubType = DwMime::kSubtypePlain;
	}

	switch( curNode_replacedType ){
	case DwMime::kTypeText: {
	  kdDebug(5006) << "* text *" << endl;
          switch( curNode_replacedSubType ){
          case DwMime::kSubtypeHtml: {
            if( reader )
              kdDebug(5006) << "html, attachmentstyle = " << reader->mAttachmentStyle << endl;
            else
              kdDebug(5006) << "html" << endl;
            QCString cstr( curNode->msgPart().bodyDecoded() );
	    //resultingRawData += cstr;
            resultString = cstr;
            if( !reader ) {
              bDone = true;
            } else if( reader->mAttachmentStyle == KMReaderWin::InlineAttmnt ||
                       (reader->mAttachmentStyle == KMReaderWin::SmartAttmnt &&
                        !curNode->isAttachment()) ||
                       (reader->mAttachmentStyle == KMReaderWin::IconicAttmnt &&
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
                                  .arg( reader->cHtmlWarning.name() ) );
                reader->writeHTMLStr(i18n("<b>Note:</b> This is an HTML message. For "
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
          case DwMime::kSubtypeVCal: {
	    kdDebug(5006) << "calendar" << endl;
	    DwMediaType ct = curNode->dwPart()->Headers().ContentType();
            DwParameter* param = ct.FirstParameter();
            QCString method( "" );
            while( param && !bDone ) {
              if( DwStrcasecmp(param->Attribute(), "method") == 0 ){
                // Method parameter found, here we are!
                bDone = true;
                method = QCString( param->Value().c_str() ).lower();
		kdDebug(5006) << "         method=" << method << endl;
		if( method == "request" || // an invitation to a meeting *or*
                    method == "reply" ||   // a reply to an invitation we sent
		    method == "cancel" ) { // Outlook uses this when cancelling
                  QCString vCal( curNode->msgPart().bodyDecoded() );
                  if( reader ){
                    QByteArray theBody( curNode->msgPart().bodyDecodedBinary() );
                    QString fname( reader->byteArrayToTempFile( reader,
                                                                "groupware",
                                                                "vCal_request.raw",
                                                                theBody ) );
                    if( !fname.isEmpty() && !showOneMimePart ){
                      QString prefix;
                      QString postfix;
                      // We let KMGroupware do most of our 'print formatting':
                      // generates text preceeding to and following to the vCal
                      if( KMGroupware::vPartToHTML( KMGroupware::NoUpdateCounter,
                                                    vCal,
                                                    fname,
                                                    reader->mUseGroupware,
                                                    prefix, postfix ) ){
                        reader->queueHtml( prefix );
                        vCal.replace( '&',  "&amp;"  );
                        vCal.replace( '<',  "&lt;"   );
                        vCal.replace( '>',  "&gt;"   );
                        vCal.replace( '\"', "&quot;" );
                        reader->writeBodyStr( vCal,
                                              reader->mCodec,
                                              curNode->trueFromAddress(),
                                              &isInlineSigned, &isInlineEncrypted );
                        reader->queueHtml( postfix );
                      }
                    }
                  }
                  resultString = vCal;
                }
              }
              param = param->Next();
            }
            break;
          }
          case DwMime::kSubtypeXVCard: {
	    kdDebug(5006) << "v-card" << endl;
	    // do nothing: X-VCard is handled in parseMsg(KMMessage* aMsg)
	    //             _before_ calling parseObjectTree()
	  }
            break;
          case DwMime::kSubtypeRtf:
	    kdDebug(5006) << "rtf" << endl;
            // RTF shouldn't be displayed inline
            bNeverDisplayInline = true;
            break;
	    // All 'Text' types which are not treated above are processed like
	    // 'Plain' text:
          case DwMime::kSubtypeRichtext:
	    kdDebug(5006) << "rich text" << endl;
          case DwMime::kSubtypeEnriched:
	    kdDebug(5006) << "enriched " << endl;
          case DwMime::kSubtypePlain:
	    kdDebug(5006) << "plain " << endl;
          default: {
	    kdDebug(5006) << "default " << endl;
	    QCString cstr( curNode->msgPart().bodyDecoded() );
	    //resultingRawData += cstr;
	    if( !reader || reader->mAttachmentStyle == KMReaderWin::InlineAttmnt ||
		(reader->mAttachmentStyle == KMReaderWin::SmartAttmnt &&
		 !curNode->isAttachment()) ||
		(reader->mAttachmentStyle == KMReaderWin::IconicAttmnt &&
		 reader->mIsFirstTextPart) ||
		showOneMimePart )
            {
	      if (reader) reader->mIsFirstTextPart = false;
	      if ( reader && curNode->isAttachment() && !showOneMimePart )
		reader->queueHtml("<br><hr><br>");
	      if ( reader ) {
		// process old style not-multipart Mailman messages to
		// enable verification of the embedded messages' signatures
		if ( DwMime::kSubtypePlain == curNode_replacedSubType &&
		     curNode->dwPart() &&
		     curNode->dwPart()->hasHeaders() ) {
		  DwHeaders& headers( curNode->dwPart()->Headers() );
		  bool bIsMailman = headers.HasField("X-Mailman-Version");
		  if ( !bIsMailman ) {
		    if ( headers.HasField("X-Mailer") )
		      bIsMailman =
			( 0 == QCString( headers.FieldBody("X-Mailer").AsString().c_str() )
                                   .find("MAILMAN", 0, false) );
		  }
		  if ( bIsMailman ) {
		    const QCString delim1( "--__--__--\n\nMessage:");
		    const QCString delim2( "--__--__--\r\n\r\nMessage:");
		    const QCString delimZ2("--__--__--\n\n_____________");
		    const QCString delimZ1("--__--__--\r\n\r\n_____________");
		    QCString partStr, digestHeaderStr;
		    int thisDelim = cstr.find(delim1, 0, false);
		    if ( -1 == thisDelim )
		      thisDelim = cstr.find(delim2, 0, false);
		    if ( -1 == thisDelim ) {
		      kdDebug(5006) << "        Sorry: Old style Mailman message but no delimiter found." << endl;
		    } else {
		      int nextDelim = cstr.find(delim1, thisDelim+1, false);
		      if ( -1 == nextDelim )
			nextDelim = cstr.find(delim2, thisDelim+1, false);
		      if ( -1 == nextDelim )
			nextDelim = cstr.find(delimZ1, thisDelim+1, false);
		      if ( -1 == nextDelim )
			nextDelim = cstr.find(delimZ2, thisDelim+1, false);
		      if( -1 < nextDelim ){
			kdDebug(5006) << "        processing old style Mailman digest" << endl;
			//if( curNode->mRoot )
			//  curNode = curNode->mRoot;

			// at least one message found: build a mime tree
			digestHeaderStr = "Content-Type=text/plain\nContent-Description=digest header\n\n";
			digestHeaderStr += cstr.mid( 0, thisDelim );
			insertAndParseNewChildNode( reader,
						    &resultString,
						    useThisCryptPlug,
						    *curNode,
						    &*digestHeaderStr,
						    "Digest Header", true );
			//reader->queueHtml("<br><hr><br>");
			// temporarily change curent node's Content-Type
			// to get our embedded RfC822 messages properly inserted
			curNode->setType(    DwMime::kTypeMultipart );
			curNode->setSubType( DwMime::kSubtypeDigest );
			while( -1 < nextDelim ){
			  int thisEoL = cstr.find("\nMessage:", thisDelim, false);
			  if( -1 < thisEoL )
			    thisDelim = thisEoL+1;
			  else{
			    thisEoL = cstr.find("\n_____________", thisDelim, false);
			    if( -1 < thisEoL )
			      thisDelim = thisEoL+1;
			  }
			  thisEoL = cstr.find('\n', thisDelim);
			  if( -1 < thisEoL )
			    thisDelim = thisEoL+1;
			  else
			    thisDelim = thisDelim+1;
			  //while( thisDelim < cstr.size() && '\n' == cstr[thisDelim] )
			  //  ++thisDelim;

			  partStr = "Content-Type=message/rfc822\nContent-Description=embedded message\n";
			  partStr += cstr.mid( thisDelim, nextDelim-thisDelim );
			  QCString subject("embedded message");
			  QCString subSearch("\nSubject:");
			  int subPos = partStr.find(subSearch, 0, false);
			  if( -1 < subPos ){
			    subject = partStr.mid(subPos+subSearch.length());
			    thisEoL = subject.find('\n');
			    if( -1 < thisEoL )
			      subject.truncate( thisEoL );
			  }
			  kdDebug(5006) << "        embedded message found: \"" << subject << "\"" << endl;
			  insertAndParseNewChildNode( reader,
						      &resultString,
						      useThisCryptPlug,
						      *curNode,
						      &*partStr,
						      subject, true );
			  //reader->queueHtml("<br><hr><br>");
			  thisDelim = nextDelim+1;
			  nextDelim = cstr.find(delim1, thisDelim, false);
			  if( -1 == nextDelim )
			    nextDelim = cstr.find(delim2, thisDelim, false);
			  if( -1 == nextDelim )
			    nextDelim = cstr.find(delimZ1, thisDelim, false);
			  if( -1 == nextDelim )
			    nextDelim = cstr.find(delimZ2, thisDelim, false);
			}
			// reset curent node's Content-Type
			curNode->setType(    DwMime::kTypeText );
			curNode->setSubType( DwMime::kSubtypePlain );
			int thisEoL = cstr.find("_____________", thisDelim);
			if( -1 < thisEoL ){
			  thisDelim = thisEoL;
			  thisEoL = cstr.find('\n', thisDelim);
			  if( -1 < thisEoL )
			    thisDelim = thisEoL+1;
			}
			else
			  thisDelim = thisDelim+1;
			partStr = "Content-Type=text/plain\nContent-Description=digest footer\n\n";
			partStr += cstr.mid( thisDelim );
			insertAndParseNewChildNode( reader,
						    &resultString,
						    useThisCryptPlug,
						    *curNode,
						    &*partStr,
						    "Digest Footer", true );
			bDone = true;
		      }
		    }
		  }
		}
		if( !bDone )
		  reader->writeBodyStr( cstr.data(),
					reader->mCodec,
					curNode->trueFromAddress(),
					&isInlineSigned, &isInlineEncrypted);
	      }
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
          switch( curNode_replacedSubType ){
          case DwMime::kSubtypeMixed: {
	    kdDebug(5006) << "mixed" << endl;
	    if( curNode->mChild ){

	      // Might be a Kroupware message,
	      // let's look for the parts contained in the mixture:
	      partNode* dataPlain =
		curNode->mChild->findType( DwMime::kTypeText, DwMime::kSubtypePlain, false, true );
	      if( dataPlain ) {
		partNode* dataCal =
		  curNode->mChild->findType( DwMime::kTypeText, DwMime::kSubtypeVCal, false, true );
		if( dataCal ){
		  // Kroupware message found,
		  // we ignore the plain text but process the calendar part.
		  dataPlain->mWasProcessed = true;
		  parseObjectTree( reader,
				   &resultString,
				   useThisCryptPlug,
				   dataCal,
				   false,
				   keepEncryptions,
				   includeSignatures );
		  bDone = true;
		}else {
		  partNode* dataTNEF =
		    curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypeMsTNEF, false, true );
		  if( dataTNEF ){
		    // encoded Kroupware message found,
		    // we ignore the plain text but process the MS-TNEF part.
		    dataPlain->mWasProcessed = true;
		    parseObjectTree( reader,
				     &resultString,
				     useThisCryptPlug,
				     dataTNEF,
				     false,
				     keepEncryptions,
				     includeSignatures );
		    bDone = true;
		  }
		}
	      }
	      if( !bDone ) {
		parseObjectTree( reader,
				 &resultString,
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
				 useThisCryptPlug,
				 dataPlain,
				 false,
				 keepEncryptions,
				 includeSignatures );
	      }
	      else
		parseObjectTree( reader,
				 &resultString,
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


	    // ATTENTION: We currently do _not_ support "multipart/signed" with _multiple_ signatures.
	    //            Instead we expect to find two objects: one object containing the signed data
	    //            and another object containing exactly one signature, this is determined by
	    //            looking for an "application/pgp-signature" object.
	    if( !curNode->mChild )
	      kdDebug(5006) << "       SORRY, signed has NO children" << endl;
	    else {
	      kdDebug(5006) << "       signed has children" << endl;

	      bool plugFound = false;

	      // ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
	      partNode* data = 0;
	      partNode* sign;
	      sign = curNode->mChild->findType(      DwMime::kTypeApplication, DwMime::kSubtypePgpSignature, false, true );
	      if( sign ) {
		kdDebug(5006) << "       OpenPGP signature found" << endl;
		data = curNode->mChild->findTypeNot( DwMime::kTypeApplication, DwMime::kSubtypePgpSignature, false, true );
		if( data ){
		  curNode->setCryptoType( partNode::CryptoTypeOpenPgpMIME );
		  plugFound = KMReaderWin::foundMatchingCryptPlug( "openpgp", &useThisCryptPlug, reader, "OpenPGP" );
		}
	      }
	      else {
		sign = curNode->mChild->findType(      DwMime::kTypeApplication, DwMime::kSubtypePkcs7Signature, false, true );
		if( sign ) {
		  kdDebug(5006) << "       S/MIME signature found" << endl;
		  data = curNode->mChild->findTypeNot( DwMime::kTypeApplication, DwMime::kSubtypePkcs7Signature, false, true );
		  if( data ){
		    curNode->setCryptoType( partNode::CryptoTypeSMIME );
		    plugFound = KMReaderWin::foundMatchingCryptPlug( "smime", &useThisCryptPlug, reader, "S/MIME" );
		  }
		} else {
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

	      bool isOpenPGP = false;
	      bool plugFound = false;

	      /*
		ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
	      */
	      partNode* data =
		curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypeOctetStream, false, true );
	      if( data ){
		isOpenPGP = true;
		curNode->setCryptoType( partNode::CryptoTypeOpenPgpMIME );
		plugFound = KMReaderWin::foundMatchingCryptPlug( "openpgp", &useThisCryptPlug, reader, "OpenPGP" );
	      }
	      if( !data ) {
		data = curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypePkcs7Mime, false, true );
		if( data ){
		  curNode->setCryptoType( partNode::CryptoTypeSMIME );
		  plugFound = KMReaderWin::foundMatchingCryptPlug( "smime", &useThisCryptPlug, reader, "S/MIME" );
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
		  bool signatureFound;
		  struct CryptPlugWrapper::SignatureMetaData sigMeta;
		  sigMeta.status              = 0;
		  sigMeta.extended_info       = 0;
		  sigMeta.extended_info_count = 0;
		  sigMeta.nota_xml            = 0;
		  bool passphraseError;

		  bool bOkDecrypt = KMReaderWin::okDecryptMIME( reader, useThisCryptPlug,
						   *data,
						   decryptedData,
						   signatureFound,
						   sigMeta,
						   true,
						   passphraseError,
						   messagePart.errorText );

		  if( bOkDecrypt ){
		    // paint the frame
		    if( reader ) {
		      messagePart.isDecryptable = true;
		      messagePart.isEncrypted = true;
		      messagePart.isSigned = false;
		      reader->queueHtml( reader->writeSigstatHeader( messagePart,
								     useThisCryptPlug,
								     curNode->trueFromAddress() ) );
		    }

		    // Note: Multipart/Encrypted might also be signed
		    //       without encapsulating a nicely formatted
		    //       ~~~~~~~                 Multipart/Signed part.
		    //                               (see RFC 3156 --> 6.2)
		    // In this case we paint a _2nd_ frame inside the
		    // encryption frame, but we do _not_ show a respective
		    // encapsulated MIME part in the Mime Tree Viewer
		    // since we do want to show the _true_ structure of the
		    // message there - not the structure that the sender's
		    // MUA 'should' have sent.  :-D       (khz, 12.09.2002)
		    //
		    if( signatureFound ){
		      writeOpaqueOrMultipartSignedData( reader,
							&resultString,
							useThisCryptPlug,
							0,
							*curNode,
							curNode->trueFromAddress(),
							false,
							&decryptedData,
							&sigMeta,
							false );
		      curNode->setSigned( true );
		    }else{
		      insertAndParseNewChildNode( reader,
						  &resultString,
						  useThisCryptPlug,
						  *curNode,
						  &*decryptedData,
						  "encrypted data" );
		    }

		    if( reader )
		      reader->queueHtml( reader->writeSigstatFooter( messagePart ) );
		  } else {
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
          switch( curNode_replacedSubType  ){
          case DwMime::kSubtypeRfc822: {
	    kdDebug(5006) << "RfC 822" << endl;
	    if( reader && reader->mAttachmentStyle != KMReaderWin::InlineAttmnt &&
		(reader->mAttachmentStyle != KMReaderWin::SmartAttmnt ||
		 curNode->isAttachment()) && !showOneMimePart)
	      break;

	    if( curNode->mChild ) {
	      kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
	      parseObjectTree( reader,
			       &resultString,
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
          switch( curNode_replacedSubType  ){
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
		  if( KMReaderWin::foundMatchingCryptPlug( "openpgp", &useThisCryptPlug, reader, "OpenPGP" ) ) {
		    QCString decryptedData;
		    bool signatureFound;
		    struct CryptPlugWrapper::SignatureMetaData sigMeta;
		    sigMeta.status              = 0;
		    sigMeta.extended_info       = 0;
		    sigMeta.extended_info_count = 0;
		    sigMeta.nota_xml            = 0;
		    bool passphraseError;
		    if( KMReaderWin::okDecryptMIME( reader, useThisCryptPlug,
				       *curNode,
				       decryptedData,
				       signatureFound,
				       sigMeta,
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
						  useThisCryptPlug,
						  *curNode,
						  &*decryptedData,
						  "encrypted data" );
		      if( reader )
			reader->queueHtml( reader->writeSigstatFooter( messagePart ) );
		    } else {
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
			       useThisCryptPlug,
			       curNode->mChild );
	      bDone = true;
	      kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
	    } else {
	      kdDebug(5006) << "\n----->  Initially processing signed and/or encrypted data\n" << endl;
	      curNode->setCryptoType( partNode::CryptoTypeSMIME );
	      if( curNode->dwPart() && curNode->dwPart()->hasHeaders() ) {
		CryptPlugWrapper* oldUseThisCryptPlug = useThisCryptPlug;
		
		if( KMReaderWin::foundMatchingCryptPlug( "smime", &useThisCryptPlug, reader, "S/MIME" ) ) {
		  
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
		    bool signatureFound;
		    struct CryptPlugWrapper::SignatureMetaData sigMeta;
		    sigMeta.status              = 0;
		    sigMeta.extended_info       = 0;
		    sigMeta.extended_info_count = 0;
		    sigMeta.nota_xml            = 0;
		    bool passphraseError;
		    if( KMReaderWin::okDecryptMIME( reader, useThisCryptPlug,
				       *curNode,
				       decryptedData,
				       signatureFound,
				       sigMeta,
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
								      useThisCryptPlug,
								      0,
								      *signTestNode,
								      curNode->trueFromAddress(),
								      true,
								      0,
								      0,
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
          case DwMime::kSubtypeMsTNEF: {
	    kdDebug(5006) << "MS TNEF encoded" << endl;
	    QString vPart( curNode->msgPart().bodyDecoded() );
	    QByteArray theBody( curNode->msgPart().bodyDecodedBinary() );
	    QString fname( KMReaderWin::byteArrayToTempFile( reader,
                                                               "groupware",
                                                               "msTNEF.raw",
                                                               theBody ) );
	    if( !fname.isEmpty() ){
	      QString prefix;
	      QString postfix;
	      // We let KMGroupware do most of our 'print formatting':
	      // 1. decodes the TNEF data and produces a vPart
	      //    or preserves the old data (if no vPart can be created)
	      // 2. generates text preceeding to / following to the vPart
	      bool bVPartCreated
		= KMGroupware::msTNEFToHTML( reader, vPart, fname,
					     reader && reader->mUseGroupware,
					     prefix, postfix );
	      if( bVPartCreated && reader && !showOneMimePart ){
		reader->queueHtml( prefix );
		vPart.replace( '&',  "&amp;"  );
		vPart.replace( '<',  "&lt;"   );
		vPart.replace( '>',  "&gt;"   );
		vPart.replace( '\"', "&quot;" );
		reader->writeBodyStr( vPart.latin1(),
				      reader->mCodec,
				      curNode->trueFromAddress(),
				      &isInlineSigned, &isInlineEncrypted );
		reader->queueHtml( postfix );
	      }
	    }
	    resultString = vPart.latin1();
	    bDone = true;
	  }
            break;
          }
        }
	  break;
	case DwMime::kTypeImage: {
	  kdDebug(5006) << "* image *" << endl;
	  
          switch( curNode_replacedSubType  ){
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
          switch( curNode_replacedSubType  ){
          case DwMime::kSubtypeBasic: {
	    kdDebug(5006) << "basic" << endl;
	  }
            break;
          }
          // We allways show audio as icon.
          if( reader && ( reader->mAttachmentStyle != KMReaderWin::HideAttmnt || showOneMimePart ) )
            reader->writePartIcon(&curNode->msgPart(), curNode->nodeId());
          bDone = true;
        }
	  break;
	case DwMime::kTypeVideo: {
	  kdDebug(5006) << "* video *" << endl;
          switch( curNode_replacedSubType  ){
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
	    ( reader->mAttachmentStyle != KMReaderWin::HideAttmnt ||
	      ( curNode && !curNode->isAttachment() ) ||
	      showOneMimePart ) ) {
	  bool asIcon = true;
	  if (showOneMimePart) {
	    asIcon = ( curNode->msgPart().contentDisposition().find("inline") < 0 );
	  }
	  else if (bNeverDisplayInline) {
	    asIcon = true;
	  } else {
	    switch (reader->mAttachmentStyle) {
	    case KMReaderWin::IconicAttmnt:
	      asIcon = TRUE;
	      break;
	    case KMReaderWin::InlineAttmnt:
	      asIcon = FALSE;
              break;
            case KMReaderWin::SmartAttmnt:
              asIcon = ( curNode->msgPart().contentDisposition().find("inline") < 0 );
              break;
            case KMReaderWin::HideAttmnt:
              // the node is the message! show it!
              asIcon = false;
	    }
	  }
	  bool forcedIcon = !isImage && curNode->type() != DwMime::kTypeText;
	  if (forcedIcon) asIcon = TRUE;
	  if( asIcon ) {
	    if (!forcedIcon || reader->mAttachmentStyle != KMReaderWin::HideAttmnt)
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
  }


  //////////////////
  //////////////////
  //////////////////

  bool ObjectTreeParser::writeOpaqueOrMultipartSignedData( KMReaderWin* reader,
						      QCString* resultString,
						      CryptPlugWrapper* useThisCryptPlug,
						      partNode* data,
						      partNode& sign,
						      const QString& fromAddress,
						      bool doCheck,
						      QCString* cleartextData,
						      struct CryptPlugWrapper::SignatureMetaData* paramSigMeta,
						      bool hideErrors )
  {
    bool bIsOpaqueSigned = false;

    CryptPlugWrapperList *cryptPlugList = kernel->cryptPlugList();
    CryptPlugWrapper* cryptPlug = useThisCryptPlug ? useThisCryptPlug : cryptPlugList->active();
    if( cryptPlug ) {
      if( !doCheck )
	kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: showing OpenPGP (Encrypted+Signed) data" << endl;
      else
	if( data )
	  kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: processing SMIME Signed data" << endl;
	else
	  kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: processing Opaque Signed data" << endl;

      if( doCheck ){
        kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: going to call CRYPTPLUG "
		      << cryptPlug->libName() << endl;

        if( !cryptPlug->initStatus( 0 ) == CryptPlugWrapper::InitStatus_Ok ) {
	  if( reader && !hideErrors )
            KMessageBox::sorry(reader, i18n("Crypto plug-in \"%1\" is not initialized.\n"
					    "Please specify the plug-in using the 'Settings->Configure KMail->Security' dialog.").arg(cryptPlug->libName()));
	  return false;
        }
      }
      QCString cleartext;
      char* new_cleartext = 0;
      QByteArray signaturetext;
      bool signatureIsBinary = false;
      int signatureLen = 0;

      if( doCheck ){
	if( data )
	  cleartext = data->dwPart()->AsString().c_str();

	if( reader && reader->mDebugReaderCrypto ){
	  QFile fileD0( "dat_01_reader_signedtext_before_canonicalization" );
	  if( fileD0.open( IO_WriteOnly ) ) {
            if( data ) {
	      QDataStream ds( &fileD0 );
	      ds.writeRawBytes( cleartext, cleartext.length() );
            }
            fileD0.close();  // If data is 0 we just create a zero length file.
	  }
	}

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
	if( reader && reader->mDebugReaderCrypto ){
	  QFile fileD( "dat_02_reader_signedtext_after_canonicalization" );
	  if( fileD.open( IO_WriteOnly ) ) {
            if( data ) {
	      QDataStream ds( &fileD );
	      ds.writeRawBytes( cleartext, cleartext.length() );
            }
            fileD.close();  // If data is 0 we just create a zero length file.
	  }
	}

	signaturetext = sign.msgPart().bodyDecodedBinary();
	QCString signatureStr( signaturetext );
	signatureIsBinary = (-1 == signatureStr.find("BEGIN SIGNED MESSAGE", 0, false) ) &&
                          (-1 == signatureStr.find("BEGIN PGP SIGNED MESSAGE", 0, false) ) &&
                          (-1 == signatureStr.find("BEGIN PGP MESSAGE", 0, false) );
	signatureLen = signaturetext.size();

	if( reader && reader->mDebugReaderCrypto ){
	  QFile fileS( "dat_03_reader.sig" );
	  if( fileS.open( IO_WriteOnly ) ) {
            QDataStream ds( &fileS );
            ds.writeRawBytes( signaturetext, signaturetext.size() );
            fileS.close();
	  }
	}

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
      }

      struct CryptPlugWrapper::SignatureMetaData localSigMeta;
      if( doCheck ){
	localSigMeta.status              = 0;
	localSigMeta.extended_info       = 0;
	localSigMeta.extended_info_count = 0;
	localSigMeta.nota_xml            = 0;
      }
      struct CryptPlugWrapper::SignatureMetaData* sigMeta = doCheck ? &localSigMeta : paramSigMeta;

      const char* cleartextP = cleartext;
      PartMetaData messagePart;
      messagePart.isSigned = true;
      messagePart.isGoodSignature = false;
      messagePart.isEncrypted = false;
      messagePart.isDecryptable = false;
      messagePart.keyTrust = Kpgp::KPGP_VALIDITY_UNKNOWN;
      messagePart.status = i18n("Wrong Crypto Plug-In!");

      if( doCheck && !cryptPlug->hasFeature( Feature_VerifySignatures ) ) {
	KMessageBox::information(reader,
				 i18n("Problem: This Crypto plug-in cannot verify message signatures.\n"
				      "Please specify an appropriate plug-in using the 'Settings->Configure KMail->Security' dialog."),
				 QString::null,
				 "cryptoPluginBox");
      } else {
	if( !doCheck ||
	    cryptPlug->checkMessageSignature( data ? const_cast<char**>(&cleartextP)
					      : &new_cleartext,
					      signaturetext,
					      signatureIsBinary,
					      signatureLen,
					      sigMeta ) ) {
	  messagePart.isGoodSignature = true;
	}

	if( doCheck )
	  kdDebug(5006) << "\nObjectTreeParser::writeOpaqueOrMultipartSignedData: returned from CRYPTPLUG" << endl;

	if( sigMeta->status && *sigMeta->status )
	  messagePart.status = QString::fromUtf8( sigMeta->status );
	messagePart.status_code = sigMeta->status_code;

	// only one signature supported
	if( sigMeta->extended_info_count != 0 ) {

	  kdDebug(5006) << "\nObjectTreeParser::writeOpaqueOrMultipartSignedData: found extended sigMeta info" << endl;

	  CryptPlugWrapper::SignatureMetaDataExtendedInfo& ext = sigMeta->extended_info[0];

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
      }

      QString unknown( i18n("(unknown)") );
      if( !doCheck || !data ){
	if( cleartextData || new_cleartext ) {
	  if( reader )
            reader->queueHtml( reader->writeSigstatHeader( messagePart,
                                                           useThisCryptPlug,
                                                           fromAddress ) );
	  bIsOpaqueSigned = true;

	  if( doCheck ){
	    QCString deb;
	    deb = "\n\nN E W    C O N T E N T = \"";
	    deb += new_cleartext;
	    deb += "\"  <--  E N D    O F    N E W    C O N T E N T\n\n";
	    kdDebug(5006) << deb << endl;
	  }

	  insertAndParseNewChildNode( reader,
				      resultString,
				      useThisCryptPlug,
				      sign,
				      doCheck ? new_cleartext : cleartextData->data(),
				      "opaqued signed data" );
	  if( doCheck )
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
	  if( sigMeta->status && *sigMeta->status ) {
	    txt.append( "<i>" );
	    txt.append( sigMeta->status );
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
			 useThisCryptPlug,
			 data );
	if( reader )
	  reader->queueHtml( reader->writeSigstatFooter( messagePart ) );
      }

      cryptPlug->freeSignatureMetaData( sigMeta );

    } else {
      if( reader && !hideErrors ) {
	KMessageBox::information(reader,
				 i18n("problem: No Crypto plug-ins found.\n"
				      "Please specify a plug-in using the 'Settings->Configure KMail->Security' dialog."),
				 QString::null,
				 "cryptoPluginBox");
	reader->queueHtml(i18n("<hr><b><h2>Signature could <u>not</u> be verified!</h2></b><br>"
			       "reason:<br><i>&nbsp; &nbsp; No Crypto plug-ins found.</i><br>"
			       "proposal:<br><i>&nbsp; &nbsp; Please specify a plug-in from<br>&nbsp; &nbsp; the "
			       "'Settings->Configure KMail->Security' dialog.</i>"));
      }
    }
    kdDebug(5006) << "\nObjectTreeParser::writeOpaqueOrMultipartSignedData: done, returning "
		  << ( bIsOpaqueSigned ? "TRUE" : "FALSE" ) << endl;
    return bIsOpaqueSigned;
  }



}; // namespace KMail
