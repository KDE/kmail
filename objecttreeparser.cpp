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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// my header file
#include "objecttreeparser.h"

// other KMail headers 
#include "kmreaderwin.h"
#include "partNode.h"
#include "kmgroupware.h"
#include "kmkernel.h"
#include "kfileio.h"
#include "partmetadata.h"
#include "attachmentstrategy.h"
#include "interfaces/htmlwriter.h"

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
#include <ktempfile.h>

// other Qt headers
#include <qlabel.h>
#include <qtextcodec.h>
#include <qfile.h>
#include <qapplication.h>

// other headers
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


namespace KMail {

  ObjectTreeParser::ObjectTreeParser( KMReaderWin * reader, CryptPlugWrapper * wrapper,
				      bool showOnlyOneMimePart, bool keepEncryptions,
				      bool includeSignatures,
				      const AttachmentStrategy * strategy,
				      HtmlWriter * htmlWriter )
    : mReader( reader ),
      mCryptPlugWrapper( wrapper ),
      mShowOnlyOneMimePart( showOnlyOneMimePart ),
      mKeepEncryptions( keepEncryptions ),
      mIncludeSignatures( includeSignatures ),
      mAttachmentStrategy( strategy ),
      mHtmlWriter( htmlWriter )
  {
    if ( !attachmentStrategy() )
      mAttachmentStrategy = reader ? reader->attachmentStrategy()
	                           : AttachmentStrategy::smart();
    if ( reader && !this->htmlWriter() )
      mHtmlWriter = reader->htmlWriter();
  }
  
  ObjectTreeParser::ObjectTreeParser( const ObjectTreeParser & other )
    : mReader( other.mReader ),
      mCryptPlugWrapper( other.cryptPlugWrapper() ),
      mShowOnlyOneMimePart( other.showOnlyOneMimePart() ),
      mKeepEncryptions( other.keepEncryptions() ),
      mIncludeSignatures( other.includeSignatures() ),
      mAttachmentStrategy( other.attachmentStrategy() ),
      mHtmlWriter( other.htmlWriter() )
  {

  }

  ObjectTreeParser::~ObjectTreeParser() {}

  void ObjectTreeParser::insertAndParseNewChildNode( partNode& startNode,
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
    ObjectTreeParser otp( mReader, cryptPlugWrapper() );
    otp.parseObjectTree( newNode );
    mResultString += otp.resultString();
    kdDebug(5006) << "\n     <-----  Finished parsing the MimePartTree in insertAndParseNewChildNode()\n" << endl;
  }


  void ObjectTreeParser::parseObjectTree( partNode* node ) {
    kdDebug(5006) << "\n**\n** ObjectTreeParser::parseObjectTree( "
		  << (node ? "node OK, " : "no node, ")
		  << "showOnlyOneMimePart: " << (showOnlyOneMimePart() ? "TRUE" : "FALSE")
		  << " ) **\n**" << endl;

    // make widgets visible that might have been hidden by
    // previous groupware activation
    if( mReader && mReader->mUseGroupware )
      emit mReader->signalGroupwareShow( false );

    /*
    // Use this array to return the complete data that were in this
    // message parts - *after* all encryption has been removed that
    // could be removed.
    // - This is used to store the message in decrypted form.
    NewByteArray dummyData;
    NewByteArray& resultingRawData( resultingRawDataPtr ? *resultingRawDataPtr
                                                      : dummyData );
    */

    if( showOnlyOneMimePart() && mReader ) {
      // clear the viewer
      mReader->mViewer->view()->setUpdatesEnabled( false );
      mReader->mViewer->view()->viewport()->setUpdatesEnabled( false );
      static_cast<QScrollView *>(mReader->mViewer->widget())->ensureVisible(0,0);

      htmlWriter()->reset();

      mReader->mColorBar->hide();

      // start the new viewer content
      htmlWriter()->begin();
      htmlWriter()->write("<html><body" +
      (mReader->mPrinting ? QString(" bgcolor=\"#FFFFFF\"")
                         : QString(" bgcolor=\"%1\"").arg(mReader->c4.name())));
      if (mReader->mBackingPixmapOn && !mReader->mPrinting )
	htmlWriter()->write(" background=\"file://" + mReader->mBackingPixmapStr + "\"");
      htmlWriter()->write(">");
    }
    if(node && (showOnlyOneMimePart() || (mReader && mReader->mShowCompleteMessage && !node->mRoot ))) {
      if( showOnlyOneMimePart() ) {
	// set this node and all it's children and their children to 'not yet processed'
	node->mWasProcessed = false;
	if( node->mChild )
	  node->mChild->setProcessed( false );
      } else
	// set this node and all it's siblings and all it's childrens to 'not yet processed'
	node->setProcessed( false );
    }

    ProcessResult processResult;

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
	case DwMime::kTypeText:
	  bDone = processTextType( curNode_replacedSubType, curNode,
				   processResult );
	  break;
	case DwMime::kTypeMultipart:
	  bDone = processMultiPartType( curNode_replacedSubType, curNode,
					processResult );
	  break;
	case DwMime::kTypeMessage:
	  bDone = processMessageType( curNode_replacedSubType, curNode,
				      processResult );
	  break;
	case DwMime::kTypeApplication:
	  bDone = processApplicationType( curNode_replacedSubType, curNode,
					  processResult );
	  break;
	case DwMime::kTypeImage:
	  bDone = processImageType( curNode_replacedSubType, curNode,
				    processResult );
	  break;
	case DwMime::kTypeAudio:
	  bDone = processAudioType( curNode_replacedSubType, curNode,
				    processResult );
	  break;
	case DwMime::kTypeVideo:
	  bDone = processVideoType( curNode_replacedSubType, curNode,
				    processResult );
	  break;
	case DwMime::kTypeModel:
	  bDone = processModelType( curNode_replacedSubType, curNode,
				    processResult );
	  break;
	}

	if( !bDone
            && mReader
            && ( attachmentStrategy() != AttachmentStrategy::hidden()
                 || showOnlyOneMimePart()
                 || !curNode->mRoot /* message is an attachment */ ) ) {
	  bool asIcon = true;
	  if( showOnlyOneMimePart() ) {
	    asIcon = !curNode->hasContentDispositionInline();
	  }
	  else if ( !processResult.neverDisplayInline() ) {
	    const AttachmentStrategy * as = attachmentStrategy();
	    if ( as == AttachmentStrategy::iconic() ) 
	      asIcon = true;
	    else if ( as == AttachmentStrategy::inlined() )
	      asIcon = false;
	    else if ( as == AttachmentStrategy::smart() )
              asIcon = !curNode->hasContentDispositionInline();
	    else if ( as == AttachmentStrategy::hidden() )
              // the node is the message! show it!
              asIcon = false;
	  }
          // neither image nor text -> show as icon
	  if( !processResult.isImage()
              && curNode->type() != DwMime::kTypeText )
            asIcon = true;
	  if( asIcon ) {
	    if( attachmentStrategy() != AttachmentStrategy::hidden() 
                || showOnlyOneMimePart() )
	      mReader->writePartIcon(&curNode->msgPart(), curNode->nodeId());
	  } else if ( processResult.isImage() ) {
	    mReader->mInlineImage = true;
	    mReader->writePartIcon(&curNode->msgPart(), curNode->nodeId());
	    mReader->mInlineImage = false;
	  } else {
	    QCString cstr( curNode->msgPart().bodyDecoded() );
	    writeBodyString( cstr, curNode->trueFromAddress(),
			     codecFor( curNode ), processResult );
	  }
	}
	curNode->mWasProcessed = true;
      }
      // parse the siblings (children are parsed in the 'multipart' case terms)
      if( !showOnlyOneMimePart() && curNode && curNode->mNext )
	parseObjectTree( curNode->mNext );

      // adjust signed/encrypted flags if inline PGP was found
      if( ( processResult.inlineSignatureState()  != KMMsgNotSigned ) ||
          ( processResult.inlineEncryptionState() != KMMsgNotEncrypted ) ) {
	if(    partNode::CryptoTypeUnknown == curNode->cryptoType()
	       || partNode::CryptoTypeNone    == curNode->cryptoType() ){
	  curNode->setCryptoType( partNode::CryptoTypeInlinePGP );
	}
        curNode->setSignatureState( processResult.inlineSignatureState() );
        curNode->setEncryptionState( processResult.inlineEncryptionState() );
      }
      if( partNode::CryptoTypeUnknown == curNode->cryptoType() )
	curNode->setCryptoType( partNode::CryptoTypeNone );
    }

    if( mReader && showOnlyOneMimePart() ) {
      htmlWriter()->write("</body></html>");
      htmlWriter()->flush();
      /*mReader->mViewer->view()->viewport()->setUpdatesEnabled( true );
	mReader->mViewer->view()->setUpdatesEnabled( true );
	mReader->mViewer->view()->viewport()->repaint( false );*/
    }
  }


  //////////////////
  //////////////////
  //////////////////

  bool ObjectTreeParser::writeOpaqueOrMultipartSignedData( partNode* data,
						      partNode& sign,
						      const QString& fromAddress,
						      bool doCheck,
						      QCString* cleartextData,
						      struct CryptPlugWrapper::SignatureMetaData* paramSigMeta,
						      bool hideErrors )
  {
    bool bIsOpaqueSigned = false;
    enum { NO_PLUGIN, NOT_INITIALIZED, CANT_VERIFY_SIGNATURES }
      cryptPlugError = NO_PLUGIN;

    CryptPlugWrapper* cryptPlug = cryptPlugWrapper();
    if( !cryptPlug )
      cryptPlug = kernel->cryptPlugList()->active();

    QString cryptPlugLibName;
    QString cryptPlugDisplayName;
    if( cryptPlug ) {
      cryptPlugLibName = cryptPlug->libName();
      if( 0 <= cryptPlugLibName.find( "openpgp", 0, false ) )
        cryptPlugDisplayName = "OpenPGP";
      else if( 0 <= cryptPlugLibName.find( "smime", 0, false ) )
        cryptPlugDisplayName = "S/MIME";
    }

#ifndef NDEBUG
    if( !doCheck )
      kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: showing OpenPGP (Encrypted+Signed) data" << endl;
    else
      if( data )
        kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: processing Multipart Signed data" << endl;
      else
        kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: processing Opaque Signed data" << endl;
#endif

    if( doCheck && cryptPlug ) {
      kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: going to call CRYPTPLUG "
                    << cryptPlugLibName << endl;

      // check whether the crypto plug-in is usable
      if( cryptPlug->initStatus( 0 ) != CryptPlugWrapper::InitStatus_Ok ) {
        cryptPlugError = NOT_INITIALIZED;
        cryptPlug = 0;
      }
      else if( !cryptPlug->hasFeature( Feature_VerifySignatures ) ) {
        cryptPlugError = CANT_VERIFY_SIGNATURES;
        cryptPlug = 0;
      }
    }

    QCString cleartext;
    char* new_cleartext = 0;
    QByteArray signaturetext;
    bool signatureIsBinary = false;
    int signatureLen = 0;

    if( doCheck && cryptPlug ) {
      if( data )
        cleartext = data->dwPart()->AsString().c_str();

      dumpToFile( "dat_01_reader_signedtext_before_canonicalization",
                  cleartext.data(), cleartext.length() );

      if( data && ( ( cryptPlugDisplayName == "OpenPGP" ) ||
                    ( cryptPlugDisplayName == "S/MIME" ) ) ) {
        // replace simple LFs by CRLSs
        // according to RfC 2633, 3.1.1 Canonicalization
        int posLF = cleartext.find( '\n' );
        if( ( 0 < posLF ) && ( '\r' != cleartext[posLF - 1] ) ) {
          kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
          cleartext = KMMessage::lf2crlf( cleartext );
          kdDebug(5006) << "                                                       done." << endl;
        }
      }

      dumpToFile( "dat_02_reader_signedtext_after_canonicalization",
                  cleartext.data(), cleartext.length() );

      signaturetext = sign.msgPart().bodyDecodedBinary();
      QCString signatureStr( signaturetext, signaturetext.size() + 1 );
      signatureIsBinary = (-1 == signatureStr.find("BEGIN SIGNED MESSAGE", 0, false) ) &&
                          (-1 == signatureStr.find("BEGIN PGP SIGNED MESSAGE", 0, false) ) &&
                          (-1 == signatureStr.find("BEGIN PGP MESSAGE", 0, false) );
      signatureLen = signaturetext.size();

      dumpToFile( "dat_03_reader.sig", signaturetext.data(),
                  signaturetext.size() );

#ifndef NDEBUG
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
#endif
    }

    struct CryptPlugWrapper::SignatureMetaData localSigMeta;
    if( doCheck ){
      localSigMeta.status              = 0;
      localSigMeta.extended_info       = 0;
      localSigMeta.extended_info_count = 0;
      localSigMeta.nota_xml            = 0;
    }
    struct CryptPlugWrapper::SignatureMetaData* sigMeta = doCheck
                                                          ? &localSigMeta
                                                          : paramSigMeta;

    const char* cleartextP = cleartext;
    PartMetaData messagePart;
    messagePart.isSigned = true;
    messagePart.technicalProblem = ( cryptPlug == 0 );
    messagePart.isGoodSignature = false;
    messagePart.isEncrypted = false;
    messagePart.isDecryptable = false;
    messagePart.keyTrust = Kpgp::KPGP_VALIDITY_UNKNOWN;
    messagePart.status = i18n("Wrong Crypto Plug-In!");

    if( !doCheck ||
        ( cryptPlug &&
          cryptPlug->checkMessageSignature( data
                                            ? const_cast<char**>(&cleartextP)
                                            : &new_cleartext,
                                            signaturetext,
                                            signatureIsBinary,
                                            signatureLen,
                                            sigMeta ) ) ) {
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

      kdDebug(5006) << "\n  key id: " << messagePart.keyId
                    << "\n  key trust: " << messagePart.keyTrust
                    << "\n  signer: " << messagePart.signer << endl;

    } else {
      messagePart.creationTime.tm_year = 0;
      messagePart.creationTime.tm_mon  = 1;
      messagePart.creationTime.tm_mday = 1;
    }

    QString unknown( i18n("(unknown)") );
    if( !doCheck || !data ){
      if( cleartextData || new_cleartext ) {
        if( mReader )
          htmlWriter()->queue( mReader->writeSigstatHeader( messagePart,
                                                            cryptPlug,
                                                            fromAddress ) );
        bIsOpaqueSigned = true;

#ifndef NDEBUG
        if( doCheck ) {
          kdDebug(5006) << "\n\nN E W    C O N T E N T = \""
                        << new_cleartext
                        << "\"  <--  E N D    O F    N E W    C O N T E N T\n\n"
                        << endl;
        }
#endif
        CryptPlugWrapper * oldCryptPlug = cryptPlugWrapper();
        setCryptPlugWrapper( cryptPlug );
        insertAndParseNewChildNode( sign,
                                    doCheck ? new_cleartext
                                            : cleartextData->data(),
                                    "opaqued signed data" );
        setCryptPlugWrapper( oldCryptPlug );
        if( doCheck )
          delete new_cleartext;

        if( mReader )
          htmlWriter()->queue( mReader->writeSigstatFooter( messagePart ) );

      }
      else if( !hideErrors ) {
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
        if( mReader )
          htmlWriter()->queue(txt);
      }
    }
    else {
      if( mReader ) {
        if( !cryptPlug ) {
          QString errorMsg;
          switch( cryptPlugError ) {
          case NOT_INITIALIZED:
            errorMsg = i18n( "Crypto plug-in \"%1\" is not initialized." )
                       .arg( cryptPlugLibName );
            break;
          case CANT_VERIFY_SIGNATURES:
            errorMsg = i18n( "Crypto plug-in \"%1\" can't verify signatures." )
                       .arg( cryptPlugLibName );
            break;
          case NO_PLUGIN:
            if( cryptPlugDisplayName.isEmpty() )
              errorMsg = i18n( "No appropriate crypto plug-in was found." );
            else
              errorMsg = i18n( "%1 is either 'OpenPGP' or 'S/MIME'",
                               "No %1 plug-in was found." )
                           .arg( cryptPlugDisplayName );
            break;
          }
          messagePart.errorText = i18n( "The message is signed, but the "
                                        "validity of the signature can't be "
                                        "verified.<br />"
                                        "Reason: %1" )
                                  .arg( errorMsg );
        }

        htmlWriter()->queue( mReader->writeSigstatHeader( messagePart,
                                                          cryptPlug,
                                                          fromAddress ) );
      }

      ObjectTreeParser otp( mReader, cryptPlug );
      otp.parseObjectTree( data );
      mResultString += otp.resultString();
	
      if( mReader )
        htmlWriter()->queue( mReader->writeSigstatFooter( messagePart ) );
    }

    if( cryptPlug )
      cryptPlug->freeSignatureMetaData( sigMeta );

    kdDebug(5006) << "\nObjectTreeParser::writeOpaqueOrMultipartSignedData: done, returning "
		  << ( bIsOpaqueSigned ? "TRUE" : "FALSE" ) << endl;
    return bIsOpaqueSigned;
  }

bool ObjectTreeParser::okDecryptMIME( partNode& data,
				      QCString& decryptedData,
				      bool& signatureFound,
				      struct CryptPlugWrapper::SignatureMetaData& sigMeta,
				      bool showWarning,
				      bool& passphraseError,
				      QString& aErrorText )
{
  passphraseError = false;
  aErrorText = QString::null;
  bool bDecryptionOk = false;
  enum { NO_PLUGIN, NOT_INITIALIZED, CANT_DECRYPT }
    cryptPlugError = NO_PLUGIN;

  CryptPlugWrapper* cryptPlug = cryptPlugWrapper();
  if ( !cryptPlug )
    cryptPlug = kernel->cryptPlugList()->active();

  QString cryptPlugLibName;
  if( cryptPlug )
    cryptPlugLibName = cryptPlug->libName();

  // check whether the crypto plug-in is usable
  if( cryptPlug ) {
    if( cryptPlug->initStatus( 0 ) != CryptPlugWrapper::InitStatus_Ok ) {
      cryptPlugError = NOT_INITIALIZED;
      cryptPlug = 0;
    }
    else if( !cryptPlug->hasFeature( Feature_DecryptMessages ) ) {
      cryptPlugError = CANT_DECRYPT;
      cryptPlug = 0;
    }
  }

  if( cryptPlug ) {
    QByteArray ciphertext( data.msgPart().bodyDecodedBinary() );
    QCString cipherStr( ciphertext );
    bool cipherIsBinary = (-1 == cipherStr.find("BEGIN ENCRYPTED MESSAGE", 0, false) ) &&
                          (-1 == cipherStr.find("BEGIN PGP ENCRYPTED MESSAGE", 0, false) ) &&
                          (-1 == cipherStr.find("BEGIN PGP MESSAGE", 0, false) );
    int cipherLen = ciphertext.size();

    dumpToFile( "dat_04_reader.encrypted", ciphertext.data(), ciphertext.size() );

#ifndef NDEBUG
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
#endif



    char* cleartext = 0;
    const char* certificate = 0;

    kdDebug(5006) << "ObjectTreeParser::decryptMIME: going to call CRYPTPLUG "
                  << cryptPlugLibName << endl;
    int errId = 0;
    char* errTxt = 0;
    bDecryptionOk = cryptPlug->decryptAndCheckMessage( ciphertext,
                                                       cipherIsBinary,
                                                       cipherLen,
                                                       &cleartext,
                                                       certificate,
                                                       &signatureFound,
                                                       &sigMeta,
                                                       &errId,
                                                       &errTxt );
    kdDebug(5006) << "ObjectTreeParser::decryptMIME: returned from CRYPTPLUG"
                  << endl;
    aErrorText = CryptPlugWrapper::errorIdToText( errId, passphraseError );
    if( bDecryptionOk )
      decryptedData = cleartext;
    else if( mReader && showWarning ) {
      decryptedData = "<div style=\"font-size:x-large; text-align:center;"
                      "padding:20pt;\">"
                    + i18n("Undecryptable data not shown.").utf8()
                    + "</div>";
      if( !passphraseError )
        aErrorText = i18n("Crypto plug-in \"%1\" could not decrypt the data.")
                       .arg( cryptPlugLibName )
                   + "<br />"
                   + i18n("Error: %1").arg( aErrorText );
    }
    delete errTxt;
    delete cleartext;
  }
  else {
    decryptedData = "<div style=\"text-align:center; padding:20pt;\">"
                  + i18n("Undecryptable data not shown.").utf8()
                  + "</div>";
    switch( cryptPlugError ) {
    case NOT_INITIALIZED:
      aErrorText = i18n( "Crypto plug-in \"%1\" is not initialized." )
                     .arg( cryptPlugLibName );
      break;
    case CANT_DECRYPT:
      aErrorText = i18n( "Crypto plug-in \"%1\" can't decrypt messages." )
                     .arg( cryptPlugLibName );
      break;
    case NO_PLUGIN:
      aErrorText = i18n( "No appropriate crypto plug-in was found." );
      break;
    }
  }

  dumpToFile( "dat_05_reader.decrypted", decryptedData.data(), decryptedData.size() );

  return bDecryptionOk;
}

QString ObjectTreeParser::byteArrayToTempFile( KMReaderWin* reader,
					       const QString& dirExt,
					       const QString& orgName,
					       const QByteArray& theBody )
{
  KTempFile *tempFile = new KTempFile( QString::null, "." + dirExt );
  tempFile->setAutoDelete(true);
  QString fname = tempFile->name();
  delete tempFile;

  bool bOk = true;

  if (access(QFile::encodeName(fname), W_OK) != 0) // Not there or not writable
    if (mkdir(QFile::encodeName(fname), 0) != 0
      || chmod (QFile::encodeName(fname), S_IRWXU) != 0)
        bOk = false; //failed create

  if( bOk )
  {
    QString fileName( orgName );
    if( reader )
      reader->mTempDirs.append(fname);
    //fileName.replace(QRegExp("[/\"\']"),"");
    // strip of a leading path
    int slashPos = fileName.findRev( '/' );
    if ( -1 != slashPos )
      fileName = fileName.mid( slashPos + 1 );
    if (fileName.isEmpty()) fileName = "unnamed";
    fname += "/" + fileName;

    if (!kByteArrayToFile(theBody, fname, false, false, false))
      bOk = false;
    if( reader )
      reader->mTempFiles.append(fname);
  }
  return bOk ? fname : QString();
}

  bool ObjectTreeParser::processTextType( int subtype, partNode * curNode,
					  ProcessResult & result ) {
    bool bDone = false;
    kdDebug(5006) << "* text *" << endl;
    switch( subtype ){
    case DwMime::kSubtypeHtml: {
      if( mReader )
	kdDebug(5006) << "html, attachmentstrategy = "
		      << attachmentStrategy()->name() << endl;
      else
	kdDebug(5006) << "html" << endl;
      QCString cstr( curNode->msgPart().bodyDecoded() );
      //resultingRawData += cstr;
      mResultString = cstr;
      if( !mReader ) {
	bDone = true;
      }
      else if( mReader->mIsFirstTextPart
               || attachmentStrategy() == AttachmentStrategy::inlined()
               || ( attachmentStrategy() == AttachmentStrategy::smart()
                    && curNode->hasContentDispositionInline() )
               || showOnlyOneMimePart() )
      {
	// ### move to OTP?
	mReader->mIsFirstTextPart = false;
	if( mReader->htmlMail() ) {
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
	  mReader->writeHTMLStr(QString("<div style=\"margin:0px 5%;"
					"border:2px solid %1;padding:10px;"
					"text-align:left;\">")
				.arg( mReader->cHtmlWarning.name() ) );
	  mReader->writeHTMLStr(i18n("<b>Note:</b> This is an HTML message. For "
				     "security reasons, only the raw HTML code "
				     "is shown. If you trust the sender of this "
				     "message then you can activate formatted "
				     //"HTML display by enabling <em>Prefer HTML "
				     //"to Plain Text</em> in the <em>Folder</em> "
				     //"menu."));
				     "HTML display for this message "
				     "<a href=\"kmail:showHTML\">by clicking here</a>."));
	  mReader->writeHTMLStr(     "</div><br /><br />");
	}
	mReader->writeHTMLStr(mReader->mCodec->toUnicode( mReader->htmlMail() ? cstr : KMMessage::html2source( cstr )));
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
	    QCString vCalC( curNode->msgPart().bodyDecoded() );
	    QString vCal( curNode->msgPart().bodyToUnicode() );
	    if( mReader ){
	      QByteArray theBody( curNode->msgPart().bodyDecodedBinary() );
	      QString fname( byteArrayToTempFile( mReader,
						  "groupware",
						  "vCal_request.raw",
						  theBody ) );
	      if( !fname.isEmpty() && !showOnlyOneMimePart() ){
		QString prefix;
		QString postfix;
		// We let KMGroupware do most of our 'print formatting':
		// generates text preceeding to and following to the vCal
		if( KMGroupware::vPartToHTML( KMGroupware::NoUpdateCounter,
					      vCal,
					      fname,
					      mReader->mUseGroupware,
					      prefix, postfix ) ){
		  htmlWriter()->queue( prefix );
		  // ### Hmmm, writeBdoyString should escape html specials...
		  vCal.replace( '&',  "&amp;"  );
		  vCal.replace( '<',  "&lt;"   );
		  vCal.replace( '>',  "&gt;"   );
		  vCal.replace( '\"', "&quot;" );
		  htmlWriter()->queue( mReader->quotedHTML( vCal ) );
		  htmlWriter()->queue( postfix );
		}
	      }
	    }
	    mResultString = vCalC;
	  }
	}
	param = param->Next();
      }
      break;
    }
    case DwMime::kSubtypeXVCard:
      kdDebug(5006) << "v-card" << endl;
      // do nothing: X-VCard is handled in parseMsg(KMMessage* aMsg)
      //             _before_ calling parseObjectTree()
      // It doesn't make sense to display raw vCards inline
      result.setNeverDisplayInline( true );
      break;
    case DwMime::kSubtypeRtf:
      kdDebug(5006) << "rtf" << endl;
      // RTF shouldn't be displayed inline
      result.setNeverDisplayInline( true );
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
      QString label = curNode->msgPart().fileName().stripWhiteSpace();
      if ( label.isEmpty() )
	label = curNode->msgPart().name().stripWhiteSpace();
      //resultingRawData += cstr;
      if( !mReader
          || mReader->mIsFirstTextPart
          || attachmentStrategy() == AttachmentStrategy::inlined()
          || ( attachmentStrategy() == AttachmentStrategy::smart()
               && ( curNode->hasContentDispositionInline() || label.isEmpty() ) )
          || showOnlyOneMimePart() )
	{
	  if( mReader ) {
            bool bDrawFrame = !mReader->mIsFirstTextPart
                              && !showOnlyOneMimePart()
                              && !label.isEmpty();
            if( bDrawFrame ) {
              label = KMMessage::quoteHtmlChars( label, true );

              QString comment = curNode->msgPart().contentDescription();
              comment = KMMessage::quoteHtmlChars( comment, true );

              QString fileName =
                mReader->writeMessagePartToTempFile( &curNode->msgPart(),
                                                     curNode->nodeId() );

              QString htmlStr;
              QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );

              htmlStr += "<table cellspacing=\"1\" class=\"textAtm\">"
                         "<tr class=\"textAtmH\"><td dir=\"" + dir + "\">";
              if( !fileName.isEmpty() )
                htmlStr += "<a href=\"" + QString("file:")
                         + KURL::encode_string( fileName ) + "\">"
                         + label + "</a>";
              else
                htmlStr += label;
              if( !comment.isEmpty() )
                htmlStr += "<br>" + comment;
              htmlStr += "</td></tr><tr class=\"textAtmB\"><td>";

              htmlWriter()->queue( htmlStr );
            }
	    // process old style not-multipart Mailman messages to
	    // enable verification of the embedded messages' signatures
	    if ( subtype == DwMime::kSubtypePlain &&
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
		    insertAndParseNewChildNode( *curNode,
						&*digestHeaderStr,
						"Digest Header", true );
		    //mReader->queueHtml("<br><hr><br>");
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
		      insertAndParseNewChildNode( *curNode,
						  &*partStr,
						  subject, true );
		      //mReader->queueHtml("<br><hr><br>");
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
		    insertAndParseNewChildNode( *curNode,
						&*partStr,
						"Digest Footer", true );
		    bDone = true;
		  }
		}
	      }
	    }
	    if( !bDone )
	      writeBodyString( cstr, curNode->trueFromAddress(),
			       codecFor( curNode ), result );
            if( bDrawFrame ) {
              htmlWriter()->queue( "</td></tr></table>" );
            }
            mReader->mIsFirstTextPart = false;
	  }
	  mResultString = cstr;
	  bDone = true;
	}
    }
      break;
    }
    return bDone;
  }

  bool ObjectTreeParser::processMultiPartType( int subtype, partNode * curNode,
					       ProcessResult & result ) {
    bool bDone = false;
    kdDebug(5006) << "* multipart *" << endl;
    switch( subtype ){
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
	    ObjectTreeParser otp( *this );
	    otp.setShowOnlyOneMimePart( false );
	    otp.parseObjectTree( dataCal );
	    mResultString += otp.resultString();
	    bDone = true;
	  }else {
	    partNode* dataTNEF =
	      curNode->mChild->findType( DwMime::kTypeApplication, DwMime::kSubtypeMsTNEF, false, true );
	    if( dataTNEF ){
	      // encoded Kroupware message found,
	      // we ignore the plain text but process the MS-TNEF part.
	      dataPlain->mWasProcessed = true;
	      ObjectTreeParser otp( *this );
	      otp.setShowOnlyOneMimePart( false );
	      otp.parseObjectTree( dataTNEF );
	      mResultString += otp.resultString();
	      bDone = true;
	    }
	  }
	}
	if( !bDone ) {
	  ObjectTreeParser otp( *this );
	  otp.setShowOnlyOneMimePart( false );
	  otp.parseObjectTree( curNode->mChild );
	  mResultString += otp.resultString();
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

	if( !mReader || (mReader->htmlMail() && dataHtml) ) {
	  if( dataPlain )
	    dataPlain->mWasProcessed = true;
	  ObjectTreeParser otp( *this );
	  otp.setShowOnlyOneMimePart( false );
	  otp.parseObjectTree( dataHtml );
	  mResultString += otp.resultString();
	}
	else if( !mReader || (!mReader->htmlMail() && dataPlain) ) {
	  if( dataHtml )
	    dataHtml->mWasProcessed = true;
	  ObjectTreeParser otp( *this );
	  otp.setShowOnlyOneMimePart( false );
	  otp.parseObjectTree( dataPlain );
	  mResultString += otp.resultString();
	}
	else {
	  ObjectTreeParser otp( *this );
	  otp.setShowOnlyOneMimePart( false );
	  otp.parseObjectTree( curNode->mChild );
	  mResultString += otp.resultString();
	}
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
      CryptPlugWrapper* oldUseThisCryptPlug = cryptPlugWrapper();


      // ATTENTION: We currently do _not_ support "multipart/signed" with _multiple_ signatures.
      //            Instead we expect to find two objects: one object containing the signed data
      //            and another object containing exactly one signature, this is determined by
      //            looking for an "application/pgp-signature" object.
      if( !curNode->mChild )
	kdDebug(5006) << "       SORRY, signed has NO children" << endl;
      else {
	kdDebug(5006) << "       signed has children" << endl;

	// ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
	partNode* data = 0;
	partNode* sign;
	sign = curNode->mChild->findType( DwMime::kTypeApplication,
                                          DwMime::kSubtypePgpSignature,
                                          false, true );
	if( sign ) {
	  kdDebug(5006) << "       OpenPGP signature found" << endl;
	  data = curNode->mChild->findTypeNot( DwMime::kTypeApplication,
                                               DwMime::kSubtypePgpSignature,
                                               false, true );
	  if( data ){
	    curNode->setCryptoType( partNode::CryptoTypeOpenPgpMIME );
            setCryptPlugWrapper( kernel->cryptPlugList()->findForLibName( "openpgp" ) );
	  }
	}
	else {
	  sign = curNode->mChild->findType( DwMime::kTypeApplication,
                                            DwMime::kSubtypePkcs7Signature,
                                            false, true );
	  if( sign ) {
	    kdDebug(5006) << "       S/MIME signature found" << endl;
	    data = curNode->mChild->findTypeNot( DwMime::kTypeApplication,
                                                 DwMime::kSubtypePkcs7Signature,
                                                 false, true );
	    if( data ){
	      curNode->setCryptoType( partNode::CryptoTypeSMIME );
              setCryptPlugWrapper( kernel->cryptPlugList()->findForLibName( "smime" ) );
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
          curNode->setSignatureState( KMMsgFullySigned );
	}

	if( !includeSignatures() ) {
	  if( !data )
	    data = curNode->mChild;
	  QCString cstr( data->msgPart().bodyDecoded() );
	  if( mReader )
	    writeBodyString( cstr, curNode->trueFromAddress(),
			     codecFor( data ), result );
	  mResultString += cstr;
	  bDone = true;
	} else if( sign && data ) {
	  // Set the signature node to done to prevent it from being processed
	  // by parseObjectTree( data ) called from writeOpaqueOrMultipartSignedData().
	  sign->mWasProcessed = true;
	  writeOpaqueOrMultipartSignedData( data,
					    *sign,
					    curNode->trueFromAddress() );
	  bDone = true;
	}
      }
      setCryptPlugWrapper( oldUseThisCryptPlug );
    }
      break;
    case DwMime::kSubtypeEncrypted: {
      kdDebug(5006) << "encrypted" << endl;
      CryptPlugWrapper* oldUseThisCryptPlug = cryptPlugWrapper();
      if( keepEncryptions() ) {
        curNode->setEncryptionState( KMMsgFullyEncrypted );
	QCString cstr( curNode->msgPart().bodyDecoded() );
	if( mReader )
	  writeBodyString ( cstr, curNode->trueFromAddress(),
			    codecFor( curNode ), result );
	mResultString += cstr;
	bDone = true;
      } else if( curNode->mChild ) {

	/*
	  ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
	*/
	partNode* data =
	  curNode->mChild->findType( DwMime::kTypeApplication,
                                     DwMime::kSubtypeOctetStream,
                                     false, true );
	if( data ) {
	  curNode->setCryptoType( partNode::CryptoTypeOpenPgpMIME );
          setCryptPlugWrapper( kernel->cryptPlugList()->findForLibName( "openpgp" ) );
	}
	if( !data ) {
	  data = curNode->mChild->findType( DwMime::kTypeApplication,
                                            DwMime::kSubtypePkcs7Mime,
                                            false, true );
	  if( data ) {
	    curNode->setCryptoType( partNode::CryptoTypeSMIME );
            setCryptPlugWrapper( kernel->cryptPlugList()->findForLibName( "smime" ) );
	  }
	}
	/*
	  ---------------------------------------------------------------------------------------------------------------
	*/

	if( data ) {
	  if( data->mChild ) {
	    kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
	    ObjectTreeParser otp( *this );
	    otp.setShowOnlyOneMimePart( false );
	    otp.parseObjectTree( data->mChild );
	    mResultString += otp.resultString();
	    bDone = true;
	    kdDebug(5006) << "\n----->  Returning from parseObjectTree( curNode->mChild )\n" << endl;
	  }
	  else {
	    kdDebug(5006) << "\n----->  Initially processing encrypted data\n" << endl;
	    PartMetaData messagePart;
            curNode->setEncryptionState( KMMsgFullyEncrypted );
	    QCString decryptedData;
	    bool signatureFound;
	    struct CryptPlugWrapper::SignatureMetaData sigMeta;
	    sigMeta.status              = 0;
	    sigMeta.extended_info       = 0;
	    sigMeta.extended_info_count = 0;
	    sigMeta.nota_xml            = 0;
	    bool passphraseError;

	    bool bOkDecrypt = okDecryptMIME( *data,
					     decryptedData,
					     signatureFound,
					     sigMeta,
					     true,
					     passphraseError,
					     messagePart.errorText );

            // paint the frame
            if( mReader ) {
              messagePart.isDecryptable = bOkDecrypt;
              messagePart.isEncrypted = true;
              messagePart.isSigned = false;
              htmlWriter()->queue( mReader->writeSigstatHeader( messagePart,
                                                                cryptPlugWrapper(),
                                                                curNode->trueFromAddress() ) );
            }

	    if( bOkDecrypt ) {
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
	      if( signatureFound ) {
		writeOpaqueOrMultipartSignedData( 0,
						  *curNode,
						  curNode->trueFromAddress(),
						  false,
						  &decryptedData,
						  &sigMeta,
						  false );
                curNode->setSignatureState( KMMsgFullySigned );
	      } else {
		insertAndParseNewChildNode( *curNode,
					    &*decryptedData,
					    "encrypted data" );
	      }
            }
            else {
	      mResultString += decryptedData;
              if( mReader ) {
                // print the error message that was returned in decryptedData
                // (utf8-encoded)
                htmlWriter()->queue( QString::fromUtf8( decryptedData.data() ) );
              }
            }

            if( mReader )
              htmlWriter()->queue( mReader->writeSigstatFooter( messagePart ) );
	    data->mWasProcessed = true; // Set the data node to done to prevent it from being processed
	    bDone = true;
	  }
	}
      }
      setCryptPlugWrapper( oldUseThisCryptPlug );
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
	ObjectTreeParser otp( *this );
	otp.setShowOnlyOneMimePart( false );
	otp.parseObjectTree( curNode->mChild );
	mResultString += otp.resultString();
	bDone = true;
      }
    }

    return bDone;
  }

  bool ObjectTreeParser::processMessageType( int subtype, partNode * curNode,
					     ProcessResult & /*result*/ ) {
    bool bDone = false;
    kdDebug(5006) << "* message *" << endl;
    switch( subtype  ){
    case DwMime::kSubtypeRfc822: {
      kdDebug(5006) << "RfC 822" << endl;
      if( mReader
          && attachmentStrategy() != AttachmentStrategy::inlined()
          && ( attachmentStrategy() != AttachmentStrategy::smart()
               || !curNode->hasContentDispositionInline() )
          && !showOnlyOneMimePart() )
	break;

      if( curNode->mChild ) {
	kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
	ObjectTreeParser otp( mReader, cryptPlugWrapper() );
	otp.parseObjectTree( curNode->mChild );
	mResultString += otp.resultString();
	bDone = true;
	kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
      } else {
	kdDebug(5006) << "\n----->  Initially processing data of embedded RfC 822 message\n" << endl;
	// paint the frame
	PartMetaData messagePart;
	if( mReader ) {
	  messagePart.isEncrypted = false;
	  messagePart.isSigned = false;
	  messagePart.isEncapsulatedRfc822Message = true;
	  htmlWriter()->queue( mReader->writeSigstatHeader( messagePart,
							    cryptPlugWrapper(),
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
	if( mReader )
	  mReader->parseMsg( &rfc822message, true );
	// display the body of the encapsulated message
	insertAndParseNewChildNode( *curNode,
				    &*rfc822messageStr,
				    "encapsulated message" );
	if( mReader )
	  htmlWriter()->queue( mReader->writeSigstatFooter( messagePart ) );
	bDone = true;
      }
    }
      break;
    }

    return bDone;
  }

  bool ObjectTreeParser::processApplicationType( int subtype, partNode * curNode,
						 ProcessResult & result ) {
    bool bDone = false;
    kdDebug(5006) << "* application *" << endl;
    switch( subtype  ){
    case DwMime::kSubtypePostscript: {
      kdDebug(5006) << "postscript" << endl;
      // showing PostScript inline can be used for a DoS attack;
      // therefore it's disabled until KMail is fixed to not hang
      // while a PostScript attachment is rendered; IK 2003-02-20
      //result.setIsImage( true );
    }
      break;
    case DwMime::kSubtypeOctetStream: {
      kdDebug(5006) << "octet stream" << endl;
      if( curNode->mChild ) {
	kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
	ObjectTreeParser otp( mReader, cryptPlugWrapper() );
	otp.parseObjectTree( curNode->mChild );
	mResultString += otp.resultString();
	bDone = true;
	kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
      } else {
	kdDebug(5006) << "\n----->  Initially processing encrypted data\n" << endl;
	CryptPlugWrapper* oldUseThisCryptPlug = cryptPlugWrapper();
	if(    curNode->mRoot
	       && DwMime::kTypeMultipart    == curNode->mRoot->type()
	       && DwMime::kSubtypeEncrypted == curNode->mRoot->subType() ) {
          curNode->setEncryptionState( KMMsgFullyEncrypted );
	  curNode->setCryptoType( partNode::CryptoTypeOpenPgpMIME );
	  if( keepEncryptions() ) {
	    QCString cstr( curNode->msgPart().bodyDecoded() );
	    if( mReader )
	      writeBodyString( cstr, curNode->trueFromAddress(),
			       codecFor( curNode ), result );
	    mResultString += cstr;
	    bDone = true;
	  } else {
	    /*
	      ATTENTION: This code is to be replaced by the planned 'auto-detect' feature.
	    */
	    PartMetaData messagePart;
            setCryptPlugWrapper( kernel->cryptPlugList()->findForLibName( "openpgp" ) );
            QCString decryptedData;
            bool signatureFound;
            struct CryptPlugWrapper::SignatureMetaData sigMeta;
            sigMeta.status              = 0;
            sigMeta.extended_info       = 0;
            sigMeta.extended_info_count = 0;
            sigMeta.nota_xml            = 0;
            bool passphraseError;

            bool bOkDecrypt = okDecryptMIME( *curNode,
                                             decryptedData,
                                             signatureFound,
                                             sigMeta,
                                             true,
                                             passphraseError,
                                             messagePart.errorText );

            // paint the frame
            if( mReader ) {
              messagePart.isDecryptable = bOkDecrypt;
              messagePart.isEncrypted = true;
              messagePart.isSigned = false;
              htmlWriter()->queue( mReader->writeSigstatHeader( messagePart,
                                                                cryptPlugWrapper(),
                                                                curNode->trueFromAddress() ) );
            }

            if( bOkDecrypt ) {
              // fixing the missing attachments bug #1090-b
              insertAndParseNewChildNode( *curNode,
                                          &*decryptedData,
                                          "encrypted data" );
            }
            else {
              mResultString += decryptedData;
              if( mReader ) {
                // print the error message that was returned in decryptedData
                // (utf8-encoded)
                htmlWriter()->queue( QString::fromUtf8( decryptedData.data() ) );
              }
            }

            if( mReader )
              htmlWriter()->queue( mReader->writeSigstatFooter( messagePart ) );
            bDone = true;
          }
	}
	setCryptPlugWrapper( oldUseThisCryptPlug );
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
	ObjectTreeParser otp( mReader, cryptPlugWrapper() );
	otp.parseObjectTree( curNode->mChild );
	mResultString += otp.resultString();
	bDone = true;
	kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
      } else {
	kdDebug(5006) << "\n----->  Initially processing signed and/or encrypted data\n" << endl;
	curNode->setCryptoType( partNode::CryptoTypeSMIME );
	if( curNode->dwPart() && curNode->dwPart()->hasHeaders() ) {
	  CryptPlugWrapper* oldUseThisCryptPlug = cryptPlugWrapper();
		
          setCryptPlugWrapper( kernel->cryptPlugList()->findForLibName( "smime" ) );
	  if( cryptPlugWrapper() ) {
		  
	    DwHeaders& headers( curNode->dwPart()->Headers() );
	    QCString ctypStr( headers.ContentType().AsString().c_str() );
	    ctypStr.replace( "\"", "" );
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

	      if( okDecryptMIME( *curNode,
				 decryptedData,
				 signatureFound,
				 sigMeta,
				 false,
				 passphraseError,
				 messagePart.errorText ) ) {
		kdDebug(5006) << "pkcs7 mime  -  encryption found  -  enveloped (encrypted) data !" << endl;
		isEncrypted = true;
                curNode->setEncryptionState( KMMsgFullyEncrypted );
		signTestNode = 0;
		// paint the frame
		messagePart.isDecryptable = true;
		if( mReader )
		  htmlWriter()->queue( mReader->writeSigstatHeader( messagePart,
								    cryptPlugWrapper(),
								    curNode->trueFromAddress() ) );
		insertAndParseNewChildNode( *curNode,
					    &*decryptedData,
					    "encrypted data" );
		if( mReader )
		  htmlWriter()->queue( mReader->writeSigstatFooter( messagePart ) );
	      } else {

		if( passphraseError ) {
		  isEncrypted = true;
		  signTestNode = 0;
		}

		if( isEncrypted ) {
		  kdDebug(5006) << "pkcs7 mime  -  ERROR: COULD NOT DECRYPT enveloped data !" << endl;
		  // paint the frame
		  messagePart.isDecryptable = false;
		  if( mReader ) {
		    htmlWriter()->queue( mReader->writeSigstatHeader( messagePart,
								      cryptPlugWrapper(),
								      curNode->trueFromAddress() ) );
		    mReader->writePartIcon(&curNode->msgPart(), curNode->nodeId());
		    htmlWriter()->queue( mReader->writeSigstatFooter( messagePart ) );
		  }
		} else {
		  kdDebug(5006) << "pkcs7 mime  -  NO encryption found" << endl;
		}
	      }
	      if( isEncrypted )
                curNode->setEncryptionState( KMMsgFullyEncrypted );
	    }

	    // We now try signature verification if necessarry.
	    if( signTestNode ) {
	      if( isSigned )
		kdDebug(5006) << "pkcs7 mime     ==      S/MIME TYPE: opaque signed data" << endl;
	      else
		kdDebug(5006) << "pkcs7 mime  -  type unknown  -  opaque signed data ?" << endl;

	      bool sigFound = writeOpaqueOrMultipartSignedData( 0,
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
		signTestNode->setSignatureState( KMMsgFullySigned );
		if( signTestNode != curNode )
		  curNode->setSignatureState( KMMsgFullySigned );
	      } else {
		kdDebug(5006) << "pkcs7 mime  -  NO signature found   :-(" << endl;
	      }
	    }

	    if( isSigned || isEncrypted )
	      bDone = true;
	  }
	  setCryptPlugWrapper( oldUseThisCryptPlug );
	}
      }
    }
      break;
    case DwMime::kSubtypeMsTNEF: {
      kdDebug(5006) << "MS TNEF encoded" << endl;
      QString vPart( curNode->msgPart().bodyDecoded() );
      QByteArray theBody( curNode->msgPart().bodyDecodedBinary() );
      QString fname( byteArrayToTempFile( mReader,
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
	  = KMGroupware::msTNEFToHTML( mReader, vPart, fname,
				       mReader && mReader->mUseGroupware,
				       prefix, postfix );
	if( bVPartCreated && mReader && !showOnlyOneMimePart() ){
	  htmlWriter()->queue( prefix );
	  // ### writeBodyPart should already escape the html specials...
	  vPart.replace( '&',  "&amp;"  );
	  vPart.replace( '<',  "&lt;"   );
	  vPart.replace( '>',  "&gt;"   );
	  vPart.replace( '\"', "&quot;" );
	  writeBodyString( vPart.latin1(), curNode->trueFromAddress(),
			   codecFor( curNode ), result );
	  htmlWriter()->queue( postfix );
	}
      }
      mResultString = vPart.latin1();
      bDone = true;
    }
      break;
    }
    return bDone;
  }

  bool ObjectTreeParser::processImageType( int /*subtype*/, partNode * /*curNode*/,
					   ProcessResult & result ) {
    result.setIsImage( true );
    return false;
  }

  bool ObjectTreeParser::processAudioType( int /*subtype*/, partNode * curNode,
					   ProcessResult & /*result*/ ) {
    // We always show audio as icon.
    if( mReader && ( attachmentStrategy() != AttachmentStrategy::hidden()
		     || showOnlyOneMimePart() ) )
      mReader->writePartIcon(&curNode->msgPart(), curNode->nodeId());
    return true;
  }

  bool ObjectTreeParser::processVideoType( int /*subtype*/, partNode * /*curNode*/,
					   ProcessResult & /*result*/ ) {
    return false;
  }

  bool ObjectTreeParser::processModelType( int /*subtype*/, partNode * /*curNode*/,
					   ProcessResult & /*result*/ ) {
    return false;
  }


  void ObjectTreeParser::writeBodyString( const QCString & bodyString,
					  const QString & fromAddress,
					  const QTextCodec * codec,
					  ProcessResult & result ) {
    assert( mReader ); assert( codec );
    KMMsgSignatureState inlineSignatureState = result.inlineSignatureState();
    KMMsgEncryptionState inlineEncryptionState = result.inlineEncryptionState();
    mReader->writeBodyStr( bodyString, codec, fromAddress,
			   inlineSignatureState, inlineEncryptionState );
    result.setInlineSignatureState( inlineSignatureState );
    result.setInlineEncryptionState( inlineEncryptionState );
  }

  const QTextCodec * ObjectTreeParser::codecFor( partNode * node ) const {
    assert( node );
    if ( mReader && !mReader->mAutoDetectEncoding )
      return mReader->mCodec;
    return node->msgPart().codec();
  }

#ifndef NDEBUG
  void ObjectTreeParser::dumpToFile( const char * filename, const char * start,
				     size_t len ) {
    if( !mReader || !mReader->mDebugReaderCrypto )
      return;

    assert( filename );

    QFile f( filename );
    if( f.open( IO_WriteOnly ) ) {
      if( start ) {
	QDataStream ds( &f );
	ds.writeRawBytes( start, len );
      }
      f.close();  // If data is 0 we just create a zero length file.
    }
  }
#endif // !NDEBUG


}; // namespace KMail
