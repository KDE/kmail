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

#include <config.h>

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
#include "htmlstatusbar.h"
#include "csshelper.h"

// other module headers
#include <mimelib/enum.h>
#include <mimelib/bodypart.h>
#include <mimelib/string.h>
#include <mimelib/text.h>

#include <kpgpblock.h>
#include <kpgp.h>
#include <linklocator.h>

#include <cryptplugwrapperlist.h>

// other KDE headers
#include <kdebug.h>
#include <klocale.h>
#include <khtml_part.h>
#include <ktempfile.h>

// other Qt headers
#include <qtextcodec.h>
#include <qfile.h>
#include <qapplication.h>

// other headers
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


namespace KMail {

  // A small class that eases temporary CryptPlugWrapper changes:
  class ObjectTreeParser::CryptPlugWrapperSaver {
    ObjectTreeParser * otp;
    CryptPlugWrapper * wrapper;
  public:
    CryptPlugWrapperSaver( ObjectTreeParser * _otp, CryptPlugWrapper * _w )
      : otp( _otp ), wrapper( _otp ? _otp->cryptPlugWrapper() : 0 )
    {
      if ( otp )
	otp->setCryptPlugWrapper( _w );
    }

    ~CryptPlugWrapperSaver() {
      if ( otp )
	otp->setCryptPlugWrapper( wrapper );
    }
  };


  ObjectTreeParser::ObjectTreeParser( KMReaderWin * reader, CryptPlugWrapper * wrapper,
				      bool showOnlyOneMimePart, bool keepEncryptions,
				      bool includeSignatures,
				      const AttachmentStrategy * strategy,
				      HtmlWriter * htmlWriter,
				      CSSHelper * cssHelper )
    : mReader( reader ),
      mCryptPlugWrapper( wrapper ),
      mShowOnlyOneMimePart( showOnlyOneMimePart ),
      mKeepEncryptions( keepEncryptions ),
      mIncludeSignatures( includeSignatures ),
      mIsFirstTextPart( true ),
      mAttachmentStrategy( strategy ),
      mHtmlWriter( htmlWriter ),
      mCSSHelper( cssHelper )
  {
    if ( !attachmentStrategy() )
      mAttachmentStrategy = reader ? reader->attachmentStrategy()
	                           : AttachmentStrategy::smart();
    if ( reader && !this->htmlWriter() )
      mHtmlWriter = reader->htmlWriter();
    if ( reader && !this->cssHelper() )
      mCSSHelper = reader->mCSSHelper;
  }
  
  ObjectTreeParser::ObjectTreeParser( const ObjectTreeParser & other )
    : mReader( other.mReader ),
      mCryptPlugWrapper( other.cryptPlugWrapper() ),
      mShowOnlyOneMimePart( other.showOnlyOneMimePart() ),
      mKeepEncryptions( other.keepEncryptions() ),
      mIncludeSignatures( other.includeSignatures() ),
      mIsFirstTextPart( other.mIsFirstTextPart ),
      mAttachmentStrategy( other.attachmentStrategy() ),
      mHtmlWriter( other.htmlWriter() ),
      mCSSHelper( other.cssHelper() )
  {

  }

  ObjectTreeParser::~ObjectTreeParser() {}

  void ObjectTreeParser::insertAndParseNewChildNode( partNode& startNode,
						     const char* content,
						     const char* cntDesc,
						     bool append )
  {
    //  DwBodyPart* myBody = new DwBodyPart( DwString( content ), node.dwPart() );
    DwBodyPart* myBody = new DwBodyPart( DwString( content ), 0 );
    myBody->Parse();

    if ( myBody->hasHeaders() ) {
      DwText& desc = myBody->Headers().ContentDescription();
      desc.FromString( cntDesc );
      desc.SetModified();
      //desc.Assemble();
      myBody->Headers().Parse();
    }

    if ( ( !myBody->Body().FirstBodyPart() || 
           myBody->Body().AsString().length() == 0 ) &&
         startNode.dwPart() &&
         startNode.dwPart()->Body().Message() &&
         startNode.dwPart()->Body().Message()->Body().FirstBodyPart() ) 
    {
      // if encapsulated imap messages are loaded the content-string is not complete
      // so we need to keep the child dwparts by copying them to the new dwpart
      kdDebug(5006) << "copy parts" << endl;
      myBody->Body().AddBodyPart( 
          startNode.dwPart()->Body().Message()->Body().FirstBodyPart() );
      myBody->Body().FromString( 
          startNode.dwPart()->Body().Message()->Body().FirstBodyPart()->Body().AsString() );
    }

    partNode* parentNode = &startNode;
    partNode* newNode = new partNode(false, myBody);
    if ( append && parentNode->mChild ) {
      parentNode = parentNode->mChild;
      while( parentNode->mNext )
        parentNode = parentNode->mNext;
      newNode = parentNode->setNext( newNode );
    } else
      newNode = parentNode->setFirstChild( newNode );

    newNode->buildObjectTree( false );

    if ( startNode.mimePartTreeItem() ) {
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
    if ( mReader && kernel->groupware().isEnabled() )
      emit mReader->signalGroupwareShow( false );

    if ( showOnlyOneMimePart() && mReader ) {
      htmlWriter()->reset();

      mReader->mColorBar->hide();

      // start the new viewer content
      htmlWriter()->begin();
      htmlWriter()->write( cssHelper()->htmlHead( mReader->isFixedFont() ) );
    }
    if (node && (showOnlyOneMimePart() || (mReader && /*mReader->mShowCompleteMessage &&*/ !node->mRoot ))) {
      if ( showOnlyOneMimePart() ) {
	// set this node and all it's children and their children to 'not yet processed'
	node->mWasProcessed = false;
	if ( node->mChild )
	  node->mChild->setProcessed( false );
      } else
	// set this node and all it's siblings and all it's childrens to 'not yet processed'
	node->setProcessed( false );
    }

    if ( node ) {
      ProcessResult processResult;
      partNode* curNode = node;

      // process all mime parts that are not covered by one of the CRYPTPLUGs
      if ( !curNode->mWasProcessed ) {
	bool bDone = false;

	switch ( curNode->type() ){
	case DwMime::kTypeText:
	  bDone = processTextType( curNode->subType(), curNode,
				   processResult );
	  break;
	case DwMime::kTypeMultipart:
	  bDone = processMultiPartType( curNode->subType(), curNode,
					processResult );
	  break;
	case DwMime::kTypeMessage:
	  bDone = processMessageType( curNode->subType(), curNode,
				      processResult );
	  break;
	case DwMime::kTypeApplication:
	  bDone = processApplicationType( curNode->subType(), curNode,
					  processResult );
	  break;
	case DwMime::kTypeImage:
	  bDone = processImageType( curNode->subType(), curNode,
				    processResult );
	  break;
	case DwMime::kTypeAudio:
	  bDone = processAudioType( curNode->subType(), curNode,
				    processResult );
	  break;
	case DwMime::kTypeVideo:
	  bDone = processVideoType( curNode->subType(), curNode,
				    processResult );
	  break;
	case DwMime::kTypeModel:
	  bDone = processModelType( curNode->subType(), curNode,
				    processResult );
	  break;
	}

	if ( !bDone
            && mReader
            && ( attachmentStrategy() != AttachmentStrategy::hidden()
                 || showOnlyOneMimePart()
                 || !curNode->mRoot /* message is an attachment */ ) ) {
	  bool asIcon = true;
	  if ( showOnlyOneMimePart() ) {
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
	  if ( !processResult.isImage()
              && curNode->type() != DwMime::kTypeText )
            asIcon = true;
	  if ( asIcon ) {
	    if ( attachmentStrategy() != AttachmentStrategy::hidden() 
                || showOnlyOneMimePart() )
	      writePartIcon( &curNode->msgPart(), curNode->nodeId() );
	  } else if ( processResult.isImage() ) {
	    writePartIcon( &curNode->msgPart(), curNode->nodeId(), true );
	  } else {
	    QCString cstr( curNode->msgPart().bodyDecoded() );
	    writeBodyString( cstr, curNode->trueFromAddress(),
			     codecFor( curNode ), processResult );
	  }
	}
	curNode->mWasProcessed = true;
      }
      // parse the siblings (children are parsed in the 'multipart' case terms)
      if ( !showOnlyOneMimePart() && curNode && curNode->mNext )
	parseObjectTree( curNode->mNext );

      // adjust signed/encrypted flags if inline PGP was found
      if ( ( processResult.inlineSignatureState()  != KMMsgNotSigned ) ||
          ( processResult.inlineEncryptionState() != KMMsgNotEncrypted ) ) {
	if (    partNode::CryptoTypeUnknown == curNode->cryptoType()
	       || partNode::CryptoTypeNone    == curNode->cryptoType() ){
	  curNode->setCryptoType( partNode::CryptoTypeInlinePGP );
	}
        curNode->setSignatureState( processResult.inlineSignatureState() );
        curNode->setEncryptionState( processResult.inlineEncryptionState() );
      }
      if ( partNode::CryptoTypeUnknown == curNode->cryptoType() )
	curNode->setCryptoType( partNode::CryptoTypeNone );
    }

    if ( mReader && showOnlyOneMimePart() ) {
      htmlWriter()->queue("</body></html>");
      htmlWriter()->flush();
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
    if ( !cryptPlug )
      cryptPlug = kernel->cryptPlugList()->active();

    QString cryptPlugLibName;
    QString cryptPlugDisplayName;
    if ( cryptPlug ) {
      cryptPlugLibName = cryptPlug->libName();
      if ( 0 <= cryptPlugLibName.find( "openpgp", 0, false ) )
        cryptPlugDisplayName = "OpenPGP";
      else if ( 0 <= cryptPlugLibName.find( "smime", 0, false ) )
        cryptPlugDisplayName = "S/MIME";
    }

#ifndef NDEBUG
    if ( !doCheck )
      kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: showing OpenPGP (Encrypted+Signed) data" << endl;
    else
      if ( data )
        kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: processing Multipart Signed data" << endl;
      else
        kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: processing Opaque Signed data" << endl;
#endif

    if ( doCheck && cryptPlug ) {
      kdDebug(5006) << "ObjectTreeParser::writeOpaqueOrMultipartSignedData: going to call CRYPTPLUG "
                    << cryptPlugLibName << endl;

      // check whether the crypto plug-in is usable
      if ( cryptPlug->initStatus( 0 ) != CryptPlugWrapper::InitStatus_Ok ) {
        cryptPlugError = NOT_INITIALIZED;
        cryptPlug = 0;
      }
      else if ( !cryptPlug->hasFeature( Feature_VerifySignatures ) ) {
        cryptPlugError = CANT_VERIFY_SIGNATURES;
        cryptPlug = 0;
      }
    }

    QCString cleartext;
    char* new_cleartext = 0;
    QByteArray signaturetext;
    bool signatureIsBinary = false;
    int signatureLen = 0;

    if ( doCheck && cryptPlug ) {
      if ( data )
        cleartext = data->dwPart()->AsString().c_str();

      dumpToFile( "dat_01_reader_signedtext_before_canonicalization",
                  cleartext.data(), cleartext.length() );

      if ( data && ( ( cryptPlugDisplayName == "OpenPGP" ) ||
                    ( cryptPlugDisplayName == "S/MIME" ) ) ) {
        // replace simple LFs by CRLSs
        // according to RfC 2633, 3.1.1 Canonicalization
//        int posLF = cleartext.find( '\n' );
//        if ( ( 0 < posLF ) && ( '\r' != cleartext[posLF - 1] ) ) {
          kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
          cleartext = KMMessage::lf2crlf( cleartext );
          kdDebug(5006) << "                                                       done." << endl;
//        }
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
      if ( signatureIsBinary )
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
    if ( doCheck ){
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

    if ( !doCheck ||
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

    if ( doCheck )
      kdDebug(5006) << "\nObjectTreeParser::writeOpaqueOrMultipartSignedData: returned from CRYPTPLUG" << endl;

    if ( sigMeta->status && *sigMeta->status )
      messagePart.status = QString::fromUtf8( sigMeta->status );
    messagePart.status_code = sigMeta->status_code;

    // only one signature supported
    if ( sigMeta->extended_info_count != 0 ) {
      kdDebug(5006) << "\nObjectTreeParser::writeOpaqueOrMultipartSignedData: found extended sigMeta info" << endl;

      CryptPlugWrapper::SignatureMetaDataExtendedInfo& ext = sigMeta->extended_info[0];

      // save extended signature status flags
      messagePart.sigStatusFlags = ext.sigStatusFlags;

      if ( messagePart.status.isEmpty()
          && ext.status_text
          && *ext.status_text )
        messagePart.status = QString::fromUtf8( ext.status_text );
      if ( ext.keyid && *ext.keyid )
        messagePart.keyId = ext.keyid;
      if ( messagePart.keyId.isEmpty() )
        messagePart.keyId = ext.fingerprint; // take fingerprint if no id found (e.g. for S/MIME)
      // ### Ugh. We depend on two enums being in sync:
      messagePart.keyTrust = (Kpgp::Validity)ext.validity;
      if ( ext.userid && *ext.userid )
        messagePart.signer = QString::fromUtf8( ext.userid );
      for( int iMail = 0; iMail < ext.emailCount; ++iMail )
        // The following if /should/ always result in TRUE but we
        // won't trust implicitely the plugin that gave us these data.
        if ( ext.emailList[ iMail ] && *ext.emailList[ iMail ] )
          messagePart.signerMailAddresses.append( QString::fromUtf8( ext.emailList[ iMail ] ) );
      if ( ext.creation_time )
        messagePart.creationTime = *ext.creation_time;
      if (     70 > messagePart.creationTime.tm_year
          || 200 < messagePart.creationTime.tm_year
	  ||   1 > messagePart.creationTime.tm_mon
          ||  12 < messagePart.creationTime.tm_mon
          ||   1 > messagePart.creationTime.tm_mday
          ||  31 < messagePart.creationTime.tm_mday ) {
        messagePart.creationTime.tm_year = 0;
        messagePart.creationTime.tm_mon  = 1;
        messagePart.creationTime.tm_mday = 1;
      }
      if ( messagePart.signer.isEmpty() ) {
        if ( ext.name && *ext.name )
          messagePart.signer = QString::fromUtf8( ext.name );
        if ( messagePart.signerMailAddresses.count() ) {
          if ( !messagePart.signer.isEmpty() )
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
    if ( !doCheck || !data ){
      if ( cleartextData || new_cleartext ) {
        if ( mReader )
          htmlWriter()->queue( writeSigstatHeader( messagePart,
						   cryptPlug,
						   fromAddress ) );
        bIsOpaqueSigned = true;

#ifndef NDEBUG
        if ( doCheck ) {
          kdDebug(5006) << "\n\nN E W    C O N T E N T = \""
                        << new_cleartext
                        << "\"  <--  E N D    O F    N E W    C O N T E N T\n\n"
                        << endl;
        }
#endif
        CryptPlugWrapperSaver cpws( this, cryptPlug );
        insertAndParseNewChildNode( sign,
                                    doCheck ? new_cleartext
                                            : cleartextData->data(),
                                    "opaqued signed data" );
        if ( doCheck )
          delete new_cleartext;

        if ( mReader )
          htmlWriter()->queue( writeSigstatFooter( messagePart ) );

      }
      else if ( !hideErrors ) {
        QString txt;
        txt = "<hr><b><h2>";
        txt.append( i18n( "The crypto engine returned no cleartext data!" ) );
        txt.append( "</h2></b>" );
        txt.append( "<br>&nbsp;<br>" );
        txt.append( i18n( "Status: " ) );
        if ( sigMeta->status && *sigMeta->status ) {
          txt.append( "<i>" );
          txt.append( sigMeta->status );
          txt.append( "</i>" );
        }
        else
          txt.append( unknown );
        if ( mReader )
          htmlWriter()->queue(txt);
      }
    }
    else {
      if ( mReader ) {
        if ( !cryptPlug ) {
          QString errorMsg;
          switch ( cryptPlugError ) {
          case NOT_INITIALIZED:
            errorMsg = i18n( "Crypto plug-in \"%1\" is not initialized." )
                       .arg( cryptPlugLibName );
            break;
          case CANT_VERIFY_SIGNATURES:
            errorMsg = i18n( "Crypto plug-in \"%1\" can't verify signatures." )
                       .arg( cryptPlugLibName );
            break;
          case NO_PLUGIN:
            if ( cryptPlugDisplayName.isEmpty() )
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

        htmlWriter()->queue( writeSigstatHeader( messagePart,
						 cryptPlug,
						 fromAddress ) );
      }

      ObjectTreeParser otp( mReader, cryptPlug );
      otp.parseObjectTree( data );
      mResultString += otp.resultString();
	
      if ( mReader )
        htmlWriter()->queue( writeSigstatFooter( messagePart ) );
    }

    if ( cryptPlug )
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
  if ( cryptPlug )
    cryptPlugLibName = cryptPlug->libName();

  // check whether the crypto plug-in is usable
  if ( cryptPlug ) {
    if ( cryptPlug->initStatus( 0 ) != CryptPlugWrapper::InitStatus_Ok ) {
      cryptPlugError = NOT_INITIALIZED;
      cryptPlug = 0;
    }
    else if ( !cryptPlug->hasFeature( Feature_DecryptMessages ) ) {
      cryptPlugError = CANT_DECRYPT;
      cryptPlug = 0;
    }
  }

  if ( cryptPlug ) {
    QByteArray ciphertext( data.msgPart().bodyDecodedBinary() );
    QCString cipherStr( ciphertext.data(), ciphertext.size() + 1 );
    bool cipherIsBinary = (-1 == cipherStr.find("BEGIN ENCRYPTED MESSAGE", 0, false) ) &&
                          (-1 == cipherStr.find("BEGIN PGP ENCRYPTED MESSAGE", 0, false) ) &&
                          (-1 == cipherStr.find("BEGIN PGP MESSAGE", 0, false) );
    int cipherLen = ciphertext.size();

    dumpToFile( "dat_04_reader.encrypted", ciphertext.data(), ciphertext.size() );

#ifndef NDEBUG
    QCString deb;
    deb =  "\n\nE N C R Y P T E D    D A T A = ";
    if ( cipherIsBinary )
      deb += "[binary data]";
    else {
      deb += "\"";
      deb += cipherStr;
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
    bDecryptionOk = cryptPlug->decryptAndCheckMessage( cipherStr.data(),
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
    if ( bDecryptionOk )
      decryptedData = cleartext;
    else if ( mReader && showWarning ) {
      decryptedData = "<div style=\"font-size:x-large; text-align:center;"
                      "padding:20pt;\">"
                    + i18n("Undecryptable data not shown.").utf8()
                    + "</div>";
      if ( !passphraseError )
        aErrorText = i18n("Crypto plug-in \"%1\" could not decrypt the data.")
                       .arg( cryptPlugLibName )
                   + "<br />"
                   + i18n("Error: %1").arg( aErrorText );
    }
    delete errTxt;
    delete[] cleartext;
  }
  else {
    decryptedData = "<div style=\"text-align:center; padding:20pt;\">"
                  + i18n("Undecryptable data not shown.").utf8()
                  + "</div>";
    switch ( cryptPlugError ) {
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

  if ( bOk )
  {
    QString fileName( orgName );
    if ( reader )
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
    if ( reader )
      reader->mTempFiles.append(fname);
  }
  return bOk ? fname : QString::null;
}

  bool ObjectTreeParser::processTextType( int subtype, partNode * curNode,
					  ProcessResult & result ) {
    kdDebug(5006) << "* text *" << endl;
    switch ( subtype ){
    case DwMime::kSubtypeHtml:
      if ( mReader )
	kdDebug(5006) << "html, attachmentstrategy = "
		      << attachmentStrategy()->name() << endl;
      else
	kdDebug(5006) << "html" << endl;
      return processTextHtmlSubtype( curNode, result );
    case DwMime::kSubtypeVCal:
      kdDebug(5006) << "calendar" << endl;
      return processTextVCalSubtype( curNode, result );
    case DwMime::kSubtypeXVCard:
      kdDebug(5006) << "v-card" << endl;
      return processTextVCardSubtype( curNode, result );
    case DwMime::kSubtypeRtf:
      kdDebug(5006) << "rtf" << endl;
      return processTextRtfSubtype( curNode, result );
    case DwMime::kSubtypeRichtext:
      kdDebug(5006) << "rich text" << endl;
      // fall through: richtext is the predecessor to enriched:
    case DwMime::kSubtypeEnriched:
      kdDebug(5006) << "enriched " << endl;
      return processTextEnrichedSubtype( curNode, result );
    case DwMime::kSubtypePlain:
      kdDebug(5006) << "plain " << endl;
      // fall through: plain is the default subtype of text/
    default:
      kdDebug(5006) << "default " << endl;
      return processTextPlainSubtype( curNode, result );
    }
  }

  bool ObjectTreeParser::processTextHtmlSubtype( partNode * curNode, ProcessResult & ) {
    QCString cstr( curNode->msgPart().bodyDecoded() );

    mResultString = cstr;
    if ( !mReader )
      return true;

    if ( mIsFirstTextPart
	|| attachmentStrategy() == AttachmentStrategy::inlined()
	|| ( attachmentStrategy() == AttachmentStrategy::smart()
	     && curNode->hasContentDispositionInline() )
	|| showOnlyOneMimePart() )
    {
      mIsFirstTextPart = false;
      if ( mReader->htmlMail() ) {
	// ---Sven's strip </BODY> and </HTML> from end of attachment start-
	// We must fo this, or else we will see only 1st inlined html
	// attachment.  It is IMHO enough to search only for </BODY> and
	// put \0 there.
	int i = cstr.findRev("</body>", -1, false); //case insensitive
	if ( 0 <= i )
	  cstr.truncate(i);
	else // just in case - search for </html>
	{
	  i = cstr.findRev("</html>", -1, false); //case insensitive
	  if ( 0 <= i ) cstr.truncate(i);
	}
	// ---Sven's strip </BODY> and </HTML> from end of attachment end-
      } else {
	htmlWriter()->queue( "<div class=\"htmlWarn\">\n" );
	htmlWriter()->queue( i18n("<b>Note:</b> This is an HTML message. For "
				  "security reasons, only the raw HTML code "
				  "is shown. If you trust the sender of this "
				  "message then you can activate formatted "
				  "HTML display for this message "
				  "<a href=\"kmail:showHTML\">by clicking here</a>.") );
	htmlWriter()->queue( "</div><br><br>" );
      }
      htmlWriter()->queue( codecFor( curNode )->toUnicode( mReader->htmlMail() ? cstr : KMMessage::html2source( cstr )));
      mReader->mColorBar->setHtmlMode();
      return true;
    }
    return false;
  }

  bool ObjectTreeParser::processTextVCalSubtype( partNode * curNode, ProcessResult & ) {
    bool bDone = false;
    DwMediaType ct = curNode->dwPart()->Headers().ContentType();
    DwParameter * param = ct.FirstParameter();
    QCString method( "" );
    while( param && !bDone ) {
      if ( DwStrcasecmp(param->Attribute(), "method") == 0 ){
	// Method parameter found, here we are!
	bDone = true;
	method = QCString( param->Value().c_str() ).lower();
	kdDebug(5006) << "         method=" << method << endl;
	if ( method == "request" || // an invitation to a meeting *or*
	    method == "reply" ||   // a reply to an invitation we sent
	    method == "cancel" ) { // Outlook uses this when cancelling
	  QCString vCalC( curNode->msgPart().bodyDecoded() );
	  QString vCal( curNode->msgPart().bodyToUnicode() );
	  if ( mReader ){
	    QByteArray theBody( curNode->msgPart().bodyDecodedBinary() );
	    QString fname( byteArrayToTempFile( mReader,
						"groupware",
						"vCal_request.raw",
						theBody ) );
	    if ( !fname.isEmpty() && !showOnlyOneMimePart() ){
	      QString prefix;
	      QString postfix;
	      // We let KMGroupware do most of our 'print formatting':
	      // generates text preceding to and following to the vCal
	      if ( kernel->groupware().vPartToHTML( KMGroupware::NoUpdateCounter,
						    vCal, fname, prefix,
						    postfix ) )
	      {
		htmlWriter()->queue( prefix );
		htmlWriter()->queue( quotedHTML( vCal ) );
		htmlWriter()->queue( postfix );
	      }
	    }
	  }
	  mResultString = vCalC;
	}
      }
      param = param->Next();
    }
    return bDone;
  }

  bool ObjectTreeParser::processTextVCardSubtype( partNode *, ProcessResult & result ) {
    // do nothing: X-VCard is handled in parseMsg(KMMessage* aMsg)
    //             _before_ calling parseObjectTree()
    // It doesn't make sense to display raw vCards inline
    result.setNeverDisplayInline( true );
    return false;
  }

  bool ObjectTreeParser::processTextRtfSubtype( partNode *, ProcessResult & result ) {
    // RTF shouldn't be displayed inline
    result.setNeverDisplayInline( true );
    return false;
  }

  bool ObjectTreeParser::processTextEnrichedSubtype( partNode * node, ProcessResult & result ) {
    return processTextPlainSubtype( node, result );
  }

  bool ObjectTreeParser::processTextPlainSubtype( partNode * curNode, ProcessResult & result ) {
    bool bDone = false;
    QCString cstr( curNode->msgPart().bodyDecoded() );
    QString label = curNode->msgPart().fileName().stripWhiteSpace();
    if ( label.isEmpty() )
      label = curNode->msgPart().name().stripWhiteSpace();
    //resultingRawData += cstr;
    if ( !mReader
	|| mIsFirstTextPart
	|| attachmentStrategy() == AttachmentStrategy::inlined()
	|| ( attachmentStrategy() == AttachmentStrategy::smart()
	     && ( curNode->hasContentDispositionInline() || label.isEmpty() ) )
	|| showOnlyOneMimePart() )
    {
      if ( mReader ) {
	bool bDrawFrame = !mIsFirstTextPart
                          && !showOnlyOneMimePart()
                          && !label.isEmpty();
	if ( bDrawFrame ) {
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
	  if ( !fileName.isEmpty() )
	    htmlStr += "<a href=\"" + QString("file:")
	      + KURL::encode_string( fileName ) + "\">"
	      + label + "</a>";
	  else
	    htmlStr += label;
	  if ( !comment.isEmpty() )
	    htmlStr += "<br>" + comment;
	  htmlStr += "</td></tr><tr class=\"textAtmB\"><td>";

	  htmlWriter()->queue( htmlStr );
	}
	// process old style not-multipart Mailman messages to
	// enable verification of the embedded messages' signatures
	if ( /* subtype == DwMime::kSubtypePlain && */
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
	      if ( -1 < nextDelim ){
		kdDebug(5006) << "        processing old style Mailman digest" << endl;
		//if ( curNode->mRoot )
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
		  if ( -1 < thisEoL )
		    thisDelim = thisEoL+1;
		  else{
		    thisEoL = cstr.find("\n_____________", thisDelim, false);
		    if ( -1 < thisEoL )
		      thisDelim = thisEoL+1;
		  }
		  thisEoL = cstr.find('\n', thisDelim);
		  if ( -1 < thisEoL )
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
		  if ( -1 < subPos ){
		    subject = partStr.mid(subPos+subSearch.length());
		    thisEoL = subject.find('\n');
		    if ( -1 < thisEoL )
		      subject.truncate( thisEoL );
		  }
		  kdDebug(5006) << "        embedded message found: \"" << subject << "\"" << endl;
		  insertAndParseNewChildNode( *curNode,
					      &*partStr,
					      subject, true );
		  //mReader->queueHtml("<br><hr><br>");
		  thisDelim = nextDelim+1;
		  nextDelim = cstr.find(delim1, thisDelim, false);
		  if ( -1 == nextDelim )
		    nextDelim = cstr.find(delim2, thisDelim, false);
		  if ( -1 == nextDelim )
		    nextDelim = cstr.find(delimZ1, thisDelim, false);
		  if ( -1 == nextDelim )
		    nextDelim = cstr.find(delimZ2, thisDelim, false);
		}
		// reset curent node's Content-Type
		curNode->setType(    DwMime::kTypeText );
		curNode->setSubType( DwMime::kSubtypePlain );
		int thisEoL = cstr.find("_____________", thisDelim);
		if ( -1 < thisEoL ){
		  thisDelim = thisEoL;
		  thisEoL = cstr.find('\n', thisDelim);
		  if ( -1 < thisEoL )
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
	if ( !bDone )
	  writeBodyString( cstr, curNode->trueFromAddress(),
			   codecFor( curNode ), result );
	if ( bDrawFrame ) {
	  htmlWriter()->queue( "</td></tr></table>" );
	}
	mIsFirstTextPart = false;
      }
      mResultString = cstr;
      bDone = true;
    }
    return bDone;
  }

  bool ObjectTreeParser::processMultiPartType( int subtype, partNode * curNode,
					       ProcessResult & result ) {
    bool bDone = false;
    kdDebug(5006) << "* multipart *" << endl;
    switch ( subtype ){
    case DwMime::kSubtypeMixed:
      kdDebug(5006) << "mixed" << endl;
      bDone = processMultiPartMixedSubtype( curNode, result );
      break;
    case DwMime::kSubtypeAlternative:
      kdDebug(5006) << "alternative" << endl;
      bDone = processMultiPartAlternativeSubtype( curNode, result );
      break;
    case DwMime::kSubtypeDigest:
      kdDebug(5006) << "digest" << endl;
      break;
    case DwMime::kSubtypeParallel:
      kdDebug(5006) << "parallel" << endl;
      break;
    case DwMime::kSubtypeSigned:
      kdDebug(5006) << "signed" << endl;
      bDone = processMultiPartSignedSubtype( curNode, result );
      break;
    case DwMime::kSubtypeEncrypted:
      kdDebug(5006) << "encrypted" << endl;
      bDone = processMultiPartEncryptedSubtype( curNode, result );
      break;
    default:
      kdDebug(5006) << "(  unknown subtype  )" << endl;
      break;
    }
    //  Multipart object not processed yet?  Just parse the children!
    if ( !bDone ){
      if ( curNode && curNode->mChild ) {
	ObjectTreeParser otp( *this );
	otp.setShowOnlyOneMimePart( false );
	otp.parseObjectTree( curNode->mChild );
	mResultString += otp.resultString();
	bDone = true;
      }
    }

    return bDone;
  }

  bool ObjectTreeParser::processMultiPartMixedSubtype( partNode * curNode, ProcessResult & ) {
    bool bDone = false;
    if ( curNode->mChild ){

      // Might be a Kroupware message,
      // let's look for the parts contained in the mixture:
      partNode* dataPlain =
	curNode->mChild->findType( DwMime::kTypeText, DwMime::kSubtypePlain, false, true );
      if ( dataPlain ) {
	partNode* dataCal =
	  curNode->mChild->findType( DwMime::kTypeText, DwMime::kSubtypeVCal, false, true );
	if ( dataCal ){
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
	  if ( dataTNEF ){
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
      if ( !bDone ) {
	ObjectTreeParser otp( *this );
	otp.setShowOnlyOneMimePart( false );
	otp.parseObjectTree( curNode->mChild );
	mResultString += otp.resultString();
	bDone = true;
      }
    }
    return bDone;
  }

  bool ObjectTreeParser::processMultiPartAlternativeSubtype( partNode * curNode, ProcessResult & ) {
    if ( !curNode->mChild )
      return false;

    partNode* dataHtml =
      curNode->mChild->findType( DwMime::kTypeText, DwMime::kSubtypeHtml, false, true );
    partNode* dataPlain =
      curNode->mChild->findType( DwMime::kTypeText, DwMime::kSubtypePlain, false, true );

    if ( !mReader || (mReader->htmlMail() && dataHtml) ) {
      if ( dataPlain )
	dataPlain->mWasProcessed = true;
      ObjectTreeParser otp( *this );
      otp.setShowOnlyOneMimePart( false );
      otp.parseObjectTree( dataHtml );
      mResultString += otp.resultString();
    } else if ( !mReader || (!mReader->htmlMail() && dataPlain) ) {
      if ( dataHtml )
	dataHtml->mWasProcessed = true;
      ObjectTreeParser otp( *this );
      otp.setShowOnlyOneMimePart( false );
      otp.parseObjectTree( dataPlain );
      mResultString += otp.resultString();
    } else {
      ObjectTreeParser otp( *this );
      otp.setShowOnlyOneMimePart( false );
      otp.parseObjectTree( curNode->mChild );
      mResultString += otp.resultString();
    }
    return true;
  }

  bool ObjectTreeParser::processMultiPartDigestSubtype( partNode * node, ProcessResult & result ) {
    return processMultiPartMixedSubtype( node, result );
  }

  bool ObjectTreeParser::processMultiPartParallelSubtype( partNode * node, ProcessResult & result ) {
    return processMultiPartMixedSubtype( node, result );
  }

  bool ObjectTreeParser::processMultiPartSignedSubtype( partNode * curNode,
							ProcessResult & result ) {
    if ( !curNode->mChild ) {
      kdDebug(5006) << "       SORRY, signed has NO children" << endl;
      return false;
    }

    CryptPlugWrapper * oldUseThisCryptPlug = cryptPlugWrapper();

    // ATTENTION: We currently do _not_ support "multipart/signed" with _multiple_ signatures.
    //            Instead we expect to find two objects: one object containing the signed data
    //            and another object containing exactly one signature, this is determined by
    //            looking for an "application/pgp-signature" object.
    kdDebug(5006) << "       signed has children" << endl;

    // ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
    partNode* data = 0;
    partNode* sign = curNode->mChild->findType( DwMime::kTypeApplication,
						DwMime::kSubtypePgpSignature,
						false, true );
    if ( sign ) {
      kdDebug(5006) << "       OpenPGP signature found" << endl;
      data = curNode->mChild->findTypeNot( DwMime::kTypeApplication,
					   DwMime::kSubtypePgpSignature,
					   false, true );
      if ( data ){
	curNode->setCryptoType( partNode::CryptoTypeOpenPgpMIME );
	setCryptPlugWrapper( kernel->cryptPlugList()->findForLibName( "openpgp" ) );
      }
    } else {
      sign = curNode->mChild->findType( DwMime::kTypeApplication,
					DwMime::kSubtypePkcs7Signature,
					false, true );
      if ( sign ) {
	kdDebug(5006) << "       S/MIME signature found" << endl;
	data = curNode->mChild->findTypeNot( DwMime::kTypeApplication,
					     DwMime::kSubtypePkcs7Signature,
					     false, true );
	if ( data ){
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

    if ( sign && data ) {
      kdDebug(5006) << "       signed has data + signature" << endl;
      curNode->setSignatureState( KMMsgFullySigned );
    }

    bool bDone = false;
    if ( !includeSignatures() ) {
      if ( !data )
	data = curNode->mChild;
      QCString cstr( data->msgPart().bodyDecoded() );
      if ( mReader )
	writeBodyString( cstr, curNode->trueFromAddress(),
			 codecFor( data ), result );
      mResultString += cstr;
      bDone = true;
    } else if ( sign && data ) {
      // Set the signature node to done to prevent it from being processed
      // by parseObjectTree( data ) called from writeOpaqueOrMultipartSignedData().
      sign->mWasProcessed = true;
      writeOpaqueOrMultipartSignedData( data,
					*sign,
					curNode->trueFromAddress() );
      bDone = true;
    }
    setCryptPlugWrapper( oldUseThisCryptPlug );
    return bDone;
  }

  bool ObjectTreeParser::processMultiPartEncryptedSubtype( partNode * curNode,
							   ProcessResult & result ) {
    if ( keepEncryptions() ) {
      curNode->setEncryptionState( KMMsgFullyEncrypted );
      QCString cstr( curNode->msgPart().bodyDecoded() );
      if ( mReader )
	writeBodyString ( cstr, curNode->trueFromAddress(),
			  codecFor( curNode ), result );
      mResultString += cstr;
      return true;
    }

    if ( !curNode->mChild )
      return false;

    bool bDone = false;
    CryptPlugWrapper* oldUseThisCryptPlug = cryptPlugWrapper();

    /*
      ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
    */
    partNode* data =
      curNode->mChild->findType( DwMime::kTypeApplication,
				 DwMime::kSubtypeOctetStream,
				 false, true );
    if ( data ) {
      curNode->setCryptoType( partNode::CryptoTypeOpenPgpMIME );
      setCryptPlugWrapper( kernel->cryptPlugList()->findForLibName( "openpgp" ) );
    }
    if ( !data ) {
      data = curNode->mChild->findType( DwMime::kTypeApplication,
					DwMime::kSubtypePkcs7Mime,
					false, true );
      if ( data ) {
	curNode->setCryptoType( partNode::CryptoTypeSMIME );
	setCryptPlugWrapper( kernel->cryptPlugList()->findForLibName( "smime" ) );
      }
    }
    /*
      ---------------------------------------------------------------------------------------------------------------
    */

    if ( data ) {
      if ( data->mChild ) {
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
	if ( mReader ) {
	  messagePart.isDecryptable = bOkDecrypt;
	  messagePart.isEncrypted = true;
	  messagePart.isSigned = false;
	  htmlWriter()->queue( writeSigstatHeader( messagePart,
						   cryptPlugWrapper(),
						   curNode->trueFromAddress() ) );
	}

	if ( bOkDecrypt ) {
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
	  if ( signatureFound ) {
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
	} else {
	  mResultString += decryptedData;
	  if ( mReader ) {
	    // print the error message that was returned in decryptedData
	    // (utf8-encoded)
	    htmlWriter()->queue( QString::fromUtf8( decryptedData.data() ) );
	  }
	}

	if ( mReader )
	  htmlWriter()->queue( writeSigstatFooter( messagePart ) );
	data->mWasProcessed = true; // Set the data node to done to prevent it from being processed
	bDone = true;
      }
    }
    setCryptPlugWrapper( oldUseThisCryptPlug );
    return bDone;    
  }

  bool ObjectTreeParser::processMessageType( int subtype, partNode * curNode,
					     ProcessResult & result ) {
    kdDebug(5006) << "* message *" << endl;
    switch ( subtype ) {
    case DwMime::kSubtypeRfc822:
      kdDebug(5006) << "RfC 822" << endl;
      return processMessageRfc822Subtype( curNode, result );
    default:
      return false;
    }
  }

  bool ObjectTreeParser::processMessageRfc822Subtype( partNode * curNode, ProcessResult & ) {
    if ( mReader
	 && !attachmentStrategy()->inlineNestedMessages()
	 && !showOnlyOneMimePart() )
      return false;

    if ( curNode->mChild ) {
      kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
      ObjectTreeParser otp( mReader, cryptPlugWrapper() );
      otp.parseObjectTree( curNode->mChild );
      mResultString += otp.resultString();
      kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
      return true;
    }
    kdDebug(5006) << "\n----->  Initially processing data of embedded RfC 822 message\n" << endl;
    // paint the frame
    PartMetaData messagePart;
    if ( mReader ) {
      messagePart.isEncrypted = false;
      messagePart.isSigned = false;
      messagePart.isEncapsulatedRfc822Message = true;
      htmlWriter()->queue( writeSigstatHeader( messagePart,
					       cryptPlugWrapper(),
					       curNode->trueFromAddress() ) );
    }
    QCString rfc822messageStr( curNode->msgPart().bodyDecoded() );
    // display the headers of the encapsulated message
    DwMessage* rfc822DwMessage = 0; // will be deleted by c'tor of rfc822headers
    if ( curNode->dwPart()->Body().Message() )
      rfc822DwMessage = new DwMessage( *(curNode->dwPart()->Body().Message()) );
    else
    {
      rfc822DwMessage = new DwMessage();
      rfc822DwMessage->FromString( rfc822messageStr );
      rfc822DwMessage->Parse();
    }
    KMMessage rfc822message( rfc822DwMessage );
    curNode->setFromAddress( rfc822message.from() );
    kdDebug(5006) << "\n----->  Store RfC 822 message header \"From: " << rfc822message.from() << "\"\n" << endl;
    if ( mReader )
      htmlWriter()->queue( mReader->writeMsgHeader( &rfc822message ) );
      //mReader->parseMsgHeader( &rfc822message );
    // display the body of the encapsulated message
    insertAndParseNewChildNode( *curNode,
				&*rfc822messageStr,
				"encapsulated message" );
    if ( mReader )
      htmlWriter()->queue( writeSigstatFooter( messagePart ) );
    return true;
  }

  bool ObjectTreeParser::processApplicationType( int subtype, partNode * curNode,
						 ProcessResult & result ) {
    kdDebug(5006) << "* application *" << endl;
    switch ( subtype  ){
    case DwMime::kSubtypePgpClearsigned:
      kdDebug(5006) << "pgp" << endl;
      // treat obsolete app/pgp subtype as t/p:
      return processTextPlainSubtype( curNode, result );
    case DwMime::kSubtypePostscript:
      kdDebug(5006) << "postscript" << endl;
      return processApplicationPostscriptSubtype( curNode, result );
    case DwMime::kSubtypeOctetStream:
      kdDebug(5006) << "octet stream" << endl;
      return processApplicationOctetStreamSubtype( curNode, result );
    case DwMime::kSubtypePkcs7Mime:
      kdDebug(5006) << "pkcs7 mime" << endl;
      return processApplicationPkcs7MimeSubtype( curNode, result );
    case DwMime::kSubtypeMsTNEF:
      kdDebug(5006) << "MS TNEF encoded" << endl;
      return processApplicationMsTnefSubtype( curNode, result );
    default:
      return false;
    }
  }

  bool ObjectTreeParser::processApplicationPostscriptSubtype( partNode *, ProcessResult & ) {
    // showing PostScript inline can be used for a DoS attack;
    // therefore it's disabled until KMail is fixed to not hang
    // while a PostScript attachment is rendered; IK 2003-02-20
    //result.setIsImage( true );
    return false;
  }

  bool ObjectTreeParser::processApplicationOctetStreamSubtype( partNode * curNode, ProcessResult & result ) {
    if ( curNode->mChild ) {
      kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
      ObjectTreeParser otp( mReader, cryptPlugWrapper() );
      otp.parseObjectTree( curNode->mChild );
      mResultString += otp.resultString();
      kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
      return true;
    }

    kdDebug(5006) << "\n----->  Initially processing encrypted data\n" << endl;
    CryptPlugWrapper* oldUseThisCryptPlug = cryptPlugWrapper();
    if (    curNode->mRoot
	    && DwMime::kTypeMultipart    == curNode->mRoot->type()
	    && DwMime::kSubtypeEncrypted == curNode->mRoot->subType() ) {
      curNode->setEncryptionState( KMMsgFullyEncrypted );
      curNode->setCryptoType( partNode::CryptoTypeOpenPgpMIME );
      if ( keepEncryptions() ) {
	QCString cstr( curNode->msgPart().bodyDecoded() );
	if ( mReader )
	  writeBodyString( cstr, curNode->trueFromAddress(),
			   codecFor( curNode ), result );
	mResultString += cstr;
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
	if ( mReader ) {
	  messagePart.isDecryptable = bOkDecrypt;
	  messagePart.isEncrypted = true;
	  messagePart.isSigned = false;
	  htmlWriter()->queue( writeSigstatHeader( messagePart,
						   cryptPlugWrapper(),
						   curNode->trueFromAddress() ) );
	}

	if ( bOkDecrypt ) {
	  // fixing the missing attachments bug #1090-b
	  insertAndParseNewChildNode( *curNode,
				      &*decryptedData,
				      "encrypted data" );
	} else {
	  mResultString += decryptedData;
	  if ( mReader ) {
	    // print the error message that was returned in decryptedData
	    // (utf8-encoded)
	    htmlWriter()->queue( QString::fromUtf8( decryptedData.data() ) );
	  }
	}

	if ( mReader )
	  htmlWriter()->queue( writeSigstatFooter( messagePart ) );
      }
    }
    setCryptPlugWrapper( oldUseThisCryptPlug );
    return true;
  }

  bool ObjectTreeParser::processApplicationPkcs7MimeSubtype( partNode * curNode, ProcessResult & ) {
    if ( curNode->mChild ) {
      kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
      ObjectTreeParser otp( mReader, cryptPlugWrapper() );
      otp.parseObjectTree( curNode->mChild );
      mResultString += otp.resultString();
      kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
      return true;
    }

    kdDebug(5006) << "\n----->  Initially processing signed and/or encrypted data\n" << endl;
    curNode->setCryptoType( partNode::CryptoTypeSMIME );
    if ( !curNode->dwPart() || !curNode->dwPart()->hasHeaders() )
      return false;

    CryptPlugWrapper * smimeCrypto = kernel->cryptPlugList()->findForLibName( "smime" );
    if ( !smimeCrypto )
      return false;
    CryptPlugWrapperSaver cpws( this, smimeCrypto );

    DwHeaders & headers( curNode->dwPart()->Headers() );
    QCString ctypStr( headers.ContentType().AsString().c_str() );
    ctypStr.replace( "\"", "" );
    bool isSigned    = 0 <= ctypStr.find("smime-type=signed-data",    0, false);
    bool isEncrypted = 0 <= ctypStr.find("smime-type=enveloped-data", 0, false);

    // Analyze "signTestNode" node to find/verify a signature.
    // If zero this verification was successfully done after
    // decrypting via recursion by insertAndParseNewChildNode().
    partNode* signTestNode = isEncrypted ? 0 : curNode;


    // We try decrypting the content
    // if we either *know* that it is an encrypted message part
    // or there is neither signed nor encrypted parameter.
    if ( !isSigned ) {
      if ( isEncrypted )
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

      if ( okDecryptMIME( *curNode,
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
	if ( mReader )
	  htmlWriter()->queue( writeSigstatHeader( messagePart,
						   cryptPlugWrapper(),
						   curNode->trueFromAddress() ) );
	insertAndParseNewChildNode( *curNode,
				    &*decryptedData,
				    "encrypted data" );
	if ( mReader )
	  htmlWriter()->queue( writeSigstatFooter( messagePart ) );
      } else {

	if ( passphraseError ) {
	  isEncrypted = true;
	  signTestNode = 0;
	}

	if ( isEncrypted ) {
	  kdDebug(5006) << "pkcs7 mime  -  ERROR: COULD NOT DECRYPT enveloped data !" << endl;
	  // paint the frame
	  messagePart.isDecryptable = false;
	  if ( mReader ) {
	    htmlWriter()->queue( writeSigstatHeader( messagePart,
						     cryptPlugWrapper(),
						     curNode->trueFromAddress() ) );
	    writePartIcon( &curNode->msgPart(), curNode->nodeId() );
	    htmlWriter()->queue( writeSigstatFooter( messagePart ) );
	  }
	} else {
	  kdDebug(5006) << "pkcs7 mime  -  NO encryption found" << endl;
	}
      }
      if ( isEncrypted )
	curNode->setEncryptionState( KMMsgFullyEncrypted );
    }

    // We now try signature verification if necessarry.
    if ( signTestNode ) {
      if ( isSigned )
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
      if ( sigFound ) {
	if ( !isSigned ) {
	  kdDebug(5006) << "pkcs7 mime  -  signature found  -  opaque signed data !" << endl;
	  isSigned = true;
	}
	signTestNode->setSignatureState( KMMsgFullySigned );
	if ( signTestNode != curNode )
	  curNode->setSignatureState( KMMsgFullySigned );
      } else {
	kdDebug(5006) << "pkcs7 mime  -  NO signature found   :-(" << endl;
      }
    }

    return isSigned || isEncrypted;
  }

  bool ObjectTreeParser::processApplicationMsTnefSubtype( partNode * curNode, ProcessResult & result ) {
    QString vPart( curNode->msgPart().bodyDecoded() );
    QByteArray theBody( curNode->msgPart().bodyDecodedBinary() );
    QString fname( byteArrayToTempFile( mReader,
					"groupware",
					"msTNEF.raw",
					theBody ) );
    if ( !fname.isEmpty() ){
      QString prefix;
      QString postfix;
      // We let KMGroupware do most of our 'print formatting':
      // 1. decodes the TNEF data and produces a vPart
      //    or preserves the old data (if no vPart can be created)
      // 2. generates text preceding to / following to the vPart
      bool bVPartCreated
	= kernel->groupware().msTNEFToHTML( mReader, vPart, fname,
					    prefix, postfix );
      if ( bVPartCreated && mReader && !showOnlyOneMimePart() ){
	htmlWriter()->queue( prefix );
	writeBodyString( vPart.latin1(), curNode->trueFromAddress(),
			 codecFor( curNode ), result );
	htmlWriter()->queue( postfix );
      }
    }
    mResultString = vPart.latin1();
    return true;
  }

  bool ObjectTreeParser::processImageType( int /*subtype*/, partNode * /*curNode*/,
					   ProcessResult & result ) {
    result.setIsImage( true );
    return false;
  }

  bool ObjectTreeParser::processAudioType( int /*subtype*/, partNode * curNode,
					   ProcessResult & /*result*/ ) {
    // We always show audio as icon.
    if ( mReader && ( attachmentStrategy() != AttachmentStrategy::hidden()
		     || showOnlyOneMimePart() ) )
      writePartIcon( &curNode->msgPart(), curNode->nodeId() );
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
    writeBodyStr( bodyString, codec, fromAddress,
		  inlineSignatureState, inlineEncryptionState );
    result.setInlineSignatureState( inlineSignatureState );
    result.setInlineEncryptionState( inlineEncryptionState );
  }

  void ObjectTreeParser::writePartIcon( KMMessagePart * msgPart, int partNum, bool inlineImage ) {
    if ( !mReader || !msgPart )
      return;

    kdDebug(5006) << "writePartIcon: PartNum: " << partNum << endl;

    QString label = msgPart->fileName();
    if( label.isEmpty() )
      label = msgPart->name();
    if( label.isEmpty() )
      label = "unnamed";
    label = KMMessage::quoteHtmlChars( label, true );

    QString comment = msgPart->contentDescription();
    comment = KMMessage::quoteHtmlChars( comment, true );

    QString fileName = mReader->writeMessagePartToTempFile( msgPart, partNum );

    QString href = fileName.isEmpty() ?
      "part://" + QString::number( partNum + 1 ) :
      "file:" + KURL::encode_string( fileName ) ;
    
    QString iconName;
    if( inlineImage )
      iconName = href;
    else {
      iconName = msgPart->iconName();
      if( iconName.right( 14 ) == "mime_empty.png" ) {
	msgPart->magicSetType();
	iconName = msgPart->iconName();
      }
    }

    if( inlineImage )
      // show the filename of the image below the embedded image
      htmlWriter()->queue( "<div><a href=\"" + href + "\">"
			   "<img src=\"" + iconName + "\" border=\"0\"></a>"
			   "</div>"
			   "<div><a href=\"" + href + "\">" + label + "</a>"
			   "</div>"
			   "<div>" + comment + "</div><br>" );
    else
      // show the filename next to the image
      htmlWriter()->queue( "<div><a href=\"" + href + "\"><img src=\"" +
			   iconName + "\" border=\"0\">" + label +
			   "</a></div>"
			   "<div>" + comment + "</div><br>" );
  }

#define SIG_FRAME_COL_UNDEF  99
#define SIG_FRAME_COL_RED    -1
#define SIG_FRAME_COL_YELLOW  0
#define SIG_FRAME_COL_GREEN   1
QString ObjectTreeParser::sigStatusToString( CryptPlugWrapper* cryptPlug,
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
        }
        else if( 0 <= cryptPlug->libName().find( "gpgme-smime", 0, false ) ) {
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
                // if a system error occurred
                // we cannot trust any information
                // that was given back by the plug-in
                showKeyInfos = false;
                frameColor = SIG_FRAME_COL_YELLOW;
            }
            if( CryptPlugWrapper::SigStatus_NUMERICAL_CODE & statusFlags ) {
                result2 += i18n("Internal system error #%1 occurred.")
                        .arg( statusFlags - CryptPlugWrapper::SigStatus_NUMERICAL_CODE );
                // if an unsupported internal error occurred
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


QString ObjectTreeParser::writeSigstatHeader( PartMetaData & block,
					      CryptPlugWrapper * cryptPlug,
					      const QString & fromAddress )
{
    bool isSMIME = cryptPlug && (0 <= cryptPlug->libName().find( "smime",   0, false ));
    QString signer = block.signer;

    QString htmlStr;
    QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );
    QString cellPadding("cellpadding=\"1\"");

    if( block.isEncapsulatedRfc822Message )
    {
        htmlStr += "<table cellspacing=\"1\" "+cellPadding+" class=\"rfc822\">"
            "<tr class=\"rfc822H\"><td dir=\"" + dir + "\">";
        htmlStr += i18n("Encapsulated message");
        htmlStr += "</td></tr><tr class=\"rfc822B\"><td>";
    }

    if( block.isEncrypted )
    {
        htmlStr += "<table cellspacing=\"1\" "+cellPadding+" class=\"encr\">"
            "<tr class=\"encrH\"><td dir=\"" + dir + "\">";
        if( block.isDecryptable )
            htmlStr += i18n("Encrypted message");
        else {
            htmlStr += i18n("Encrypted message (decryption not possible)");
            if( !block.errorText.isEmpty() )
                htmlStr += "<br />" + i18n("Reason: %1").arg( block.errorText );
        }
        htmlStr += "</td></tr><tr class=\"encrB\"><td>";
    }

    if( block.isSigned ) {
        QStringList& blockAddrs( block.signerMailAddresses );
        // note: At the moment frameColor and showKeyInfos are
        //       used for CMS only but not for PGP signatures
        // pending(khz): Implement usage of these for PGP sigs as well.
        int frameColor = SIG_FRAME_COL_UNDEF;
        bool showKeyInfos;
        bool onlyShowKeyURL = false;
        bool cannotCheckSignature = true;
        QString statusStr = sigStatusToString( cryptPlug,
                                               block.status_code,
                                               block.sigStatusFlags,
                                               frameColor,
                                               showKeyInfos );
        // if needed fallback to english status text
        // that was reported by the plugin
        if( statusStr.isEmpty() )
            statusStr = block.status;
        if( block.technicalProblem )
            frameColor = SIG_FRAME_COL_YELLOW;

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
        //       always show the URL.    (khz, 2002/06/27)
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


        // temporary hack: always show key infos!
        showKeyInfos = true;

        // Sorry for using 'black' as null color but .isValid()
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
            switch( frameColor ) {
                case SIG_FRAME_COL_RED:
                    block.signClass = "signErr";//"signCMSRed";
                    onlyShowKeyURL = true;
                    break;
                case SIG_FRAME_COL_YELLOW:
                    if( block.technicalProblem )
                        block.signClass = "signWarn";
                    else
                        block.signClass = "signOkKeyBad";//"signCMSYellow";
                    break;
                case SIG_FRAME_COL_GREEN:
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
                    break;
            }

            htmlStr += "<table cellspacing=\"1\" "+cellPadding+" "
                "class=\"" + block.signClass + "\">"
                "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
            if( block.technicalProblem ) {
                htmlStr += block.errorText;
            }
            else if( showKeyInfos ) {
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
                        signer = KMMessage::emailAddrAsAnchor( signer );
                        if( blockAddrs.count() ){
                            QString address = KMMessage::encodeMailtoUrl( blockAddrs.first() );
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
                                    htmlStr += i18n( "Message was signed by %3 with key %1, created %2." )
                                            .arg( keyWithWithoutURL )
                                            .arg( created.toString( Qt::LocalDate ) )
                                            .arg( signer );
                            }
                        }
                        else {
                            if( signer.isEmpty() || onlyShowKeyURL )
                                htmlStr += i18n( "Message was signed with key %1." )
                                        .arg( keyWithWithoutURL );
                            else
                                htmlStr += i18n( "Message was signed by %2 with key %1." )
                                        .arg( keyWithWithoutURL )
                                        .arg( signer );
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

            if( block.signer.isEmpty() || block.technicalProblem ) {
                block.signClass = "signWarn";
                htmlStr += "<table cellspacing=\"1\" "+cellPadding+" "
                    "class=\"" + block.signClass + "\">"
                    "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
                if( block.technicalProblem ) {
                    htmlStr += block.errorText;
                }
                else {
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
                }
                htmlStr += "</td></tr><tr class=\"" + block.signClass + "B\"><td>";
            }
            else
            {
                // HTMLize the signer's user id and create mailto: link
                signer = KMMessage::quoteHtmlChars( signer, true );
                signer = "<a href=\"mailto:" + signer + "\">" + signer + "</a>";

                if (block.isGoodSignature) {
                    if( block.keyTrust < Kpgp::KPGP_VALIDITY_MARGINAL )
                        block.signClass = "signOkKeyBad";
                    else
                        block.signClass = "signOkKeyOk";
                    htmlStr += "<table cellspacing=\"1\" "+cellPadding+" "
                        "class=\"" + block.signClass + "\">"
                        "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
                    if( !block.keyId.isEmpty() )
                        htmlStr += i18n( "Message was signed by %2 (Key ID: %1)." )
                                   .arg( keyWithWithoutURL )
                                   .arg( signer );
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
                    htmlStr += "<table cellspacing=\"1\" "+cellPadding+" "
                        "class=\"" + block.signClass + "\">"
                        "<tr class=\"" + block.signClass + "H\"><td dir=\"" + dir + "\">";
                    if( !block.keyId.isEmpty() )
                        htmlStr += i18n( "Message was signed by %2 (Key ID: %1)." )
                        .arg( keyWithWithoutURL )
                        .arg( signer );
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

QString ObjectTreeParser::writeSigstatFooter( PartMetaData& block )
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
void ObjectTreeParser::writeBodyStr( const QCString& aStr, const QTextCodec *aCodec,
                                const QString& fromAddress )
{
  KMMsgSignatureState dummy1;
  KMMsgEncryptionState dummy2;
  writeBodyStr( aStr, aCodec, fromAddress, dummy1, dummy2 );
}

//-----------------------------------------------------------------------------
void ObjectTreeParser::writeBodyStr( const QCString& aStr, const QTextCodec *aCodec,
                                const QString& fromAddress,
                                KMMsgSignatureState&  inlineSignatureState,
                                KMMsgEncryptionState& inlineEncryptionState )
{
  bool goodSignature = false;
  Kpgp::Module* pgp = Kpgp::Module::getKpgp();
  assert(pgp != 0);
  bool isPgpMessage = false; // true if the message contains at least one
                             // PGP MESSAGE or one PGP SIGNED MESSAGE block
  QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );
  QString headerStr = QString("<div dir=\"%1\">").arg(dir);

  inlineSignatureState  = KMMsgNotSigned;
  inlineEncryptionState = KMMsgNotEncrypted;
  QPtrList<Kpgp::Block> pgpBlocks;
  QStrList nonPgpBlocks;
  if( Kpgp::Module::prepareMessageForDecryption( aStr, pgpBlocks, nonPgpBlocks ) )
  {
      bool isEncrypted = false, isSigned = false;
      bool fullySignedOrEncrypted = true;
      bool firstNonPgpBlock = true;
      bool couldDecrypt = false;
      QString signer;
      QCString keyId;
      QString decryptionError;
      Kpgp::Validity keyTrust = Kpgp::KPGP_VALIDITY_FULL;

      QPtrListIterator<Kpgp::Block> pbit( pgpBlocks );

      QStrListIterator npbit( nonPgpBlocks );

      QString htmlStr;
      for( ; *pbit != 0; ++pbit, ++npbit )
      {
	  // insert the next Non-OpenPGP block
	  QCString str( *npbit );
	  if( !str.isEmpty() ) {
	    htmlStr += quotedHTML( aCodec->toUnicode( str ) );
            kdDebug( 5006 ) << "Non-empty Non-OpenPGP block found: '" << str
                            << "'" << endl;
            // treat messages with empty lines before the first clearsigned
            // block as fully signed/encrypted
            if( firstNonPgpBlock ) {
              // check whether str only consists of \n
              for( QCString::ConstIterator c = str.begin(); *c; ++c ) {
                if( *c != '\n' ) {
                  fullySignedOrEncrypted = false;
                  break;
                }
              }
            }
            else {
              fullySignedOrEncrypted = false;
            }
          }
          firstNonPgpBlock = false;

	  //htmlStr += "<br>";

	  Kpgp::Block* block = *pbit;
	  if( ( block->type() == Kpgp::PgpMessageBlock ) ||
	      ( block->type() == Kpgp::ClearsignedBlock ) )
	  {
	      isPgpMessage = true;
	      if( block->type() == Kpgp::PgpMessageBlock )
	      {
		if ( mReader )
		  emit mReader->noDrag();
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

              if( isSigned )
                inlineSignatureState = KMMsgPartiallySigned;
	      if( isEncrypted )
                inlineEncryptionState = KMMsgPartiallyEncrypted;

	      PartMetaData messagePart;

	      messagePart.isSigned = isSigned;
	      messagePart.technicalProblem = false;
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
      if( !str.isEmpty() ) {
        htmlStr += quotedHTML( aCodec->toUnicode( str ) );
        // Even if the trailing Non-OpenPGP block isn't empty we still
        // consider the message part fully signed/encrypted because else
        // all inline signed mailing list messages would only be partially
        // signed because of the footer which is often added by the mailing
        // list software. IK, 2003-02-15
      }
      if( fullySignedOrEncrypted ) {
        if( inlineSignatureState == KMMsgPartiallySigned )
          inlineSignatureState = KMMsgFullySigned;
        if( inlineEncryptionState == KMMsgPartiallyEncrypted )
          inlineEncryptionState = KMMsgFullyEncrypted;
      }
      htmlWriter()->queue( htmlStr );
  }
  else
    htmlWriter()->queue( quotedHTML( aCodec->toUnicode( aStr ) ) );
}

QString ObjectTreeParser::quotedHTML(const QString& s)
{
  assert( mReader );
  assert( cssHelper() );

  QString htmlStr;
  const QString normalStartTag = cssHelper()->nonQuotedFontTag();
  QString quoteFontTag[3];
  for ( int i = 0 ; i < 3 ; ++i )
    quoteFontTag[i] = cssHelper()->quoteFontTag( i );
  const QString normalEndTag = "</div>";
  const QString quoteEnd = "</div>";

  unsigned int pos, beg;
  const unsigned int length = s.length();

  // skip leading empty lines
  for ( pos = 0; pos < length && s[pos] <= ' '; pos++ );
  while (pos > 0 && (s[pos-1] == ' ' || s[pos-1] == '\t')) pos--;
  beg = pos;

  int currQuoteLevel = -2; // -2 == no previous lines

  while (beg<length)
  {
    QString line;

    /* search next occurrence of '\n' */
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
        htmlStr += quoteFontTag[currQuoteLevel%3];
    }

    // don't write empty <div ...></div> blocks (they have zero height)
    // ignore ^M DOS linebreaks
    if( !line.replace('\015', "").isEmpty() )
    {
      if( line.isRightToLeft() )
        htmlStr += QString( "<div dir=\"rtl\">" );
      else
        htmlStr += QString( "<div dir=\"ltr\">" );
      htmlStr += LinkLocator::convertToHtml( line, true /* preserve blanks */);
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



  const QTextCodec * ObjectTreeParser::codecFor( partNode * node ) const {
    assert( node );
    if ( mReader && mReader->overrideCodec() )
      return mReader->overrideCodec();
    return node->msgPart().codec();
  }

#ifndef NDEBUG
  void ObjectTreeParser::dumpToFile( const char * filename, const char * start,
				     size_t len ) {
    assert( filename );

    QFile f( filename );
    if ( f.open( IO_WriteOnly ) ) {
      if ( start ) {
	QDataStream ds( &f );
	ds.writeRawBytes( start, len );
      }
      f.close();  // If data is 0 we just create a zero length file.
    }
  }
#endif // !NDEBUG


} // namespace KMail
