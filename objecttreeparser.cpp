/*  -*- mode: C++; c-file-style: "gnu" -*-
    objecttreeparser.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002-2004 Klarälvdalens Datakonsult AB
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

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

#include <config.h>

// my header file
#include "objecttreeparser.h"

// other KMail headers
#include "kmreaderwin.h"
#include "partNode.h"
#include "kmgroupware.h"
#include "kmkernel.h"
#include <libkdepim/kfileio.h>
#include <libemailfunctions/email.h>
#include "partmetadata.h"
#include "attachmentstrategy.h"
#include "interfaces/htmlwriter.h"
#include "htmlstatusbar.h"
#include "csshelper.h"
#include "bodypartformatter.h"
#include "bodypartformatterfactory.h"
#include "partnodebodypart.h"
#include "interfaces/bodypartformatter.h"

#include "cryptplugwrapperlist.h"
#include "cryptplugfactory.h"

// other module headers
#include <mimelib/enum.h>
#include <mimelib/bodypart.h>
#include <mimelib/string.h>
#include <mimelib/text.h>

#include <gpgmepp/importresult.h>

#include <kpgpblock.h>
#include <kpgp.h>
#include <linklocator.h>

// other KDE headers
#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <khtml_part.h>
#include <ktempfile.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kmessagebox.h>

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
    if ( append && parentNode->firstChild() ) {
      parentNode = parentNode->firstChild();
      while( parentNode->nextSibling() )
        parentNode = parentNode->nextSibling();
      parentNode->setNext( newNode );
    } else
      parentNode->setFirstChild( newNode );

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
    mRawReplyString += otp.rawReplyString();
    mTextualContent += otp.textualContent();
    if ( !otp.textualContentCharset().isEmpty() )
      mTextualContentCharset = otp.textualContentCharset();
    kdDebug(5006) << "\n     <-----  Finished parsing the MimePartTree in insertAndParseNewChildNode()\n" << endl;
  }


//-----------------------------------------------------------------------------

  void ObjectTreeParser::parseObjectTree( partNode * node ) {
    kdDebug(5006) << "ObjectTreeParser::parseObjectTree( "
                  << (node ? "node OK, " : "no node, ")
                  << "showOnlyOneMimePart: " << (showOnlyOneMimePart() ? "TRUE" : "FALSE")
                  << " )" << endl;

    if ( !node )
      return;

    // reset "processed" flags for...
    if ( showOnlyOneMimePart() ) {
      // ... this node and all descendants
      node->setProcessed( false, false );
      if ( partNode * child = node->firstChild() )
        child->setProcessed( false, true );
    } else if ( mReader && !node->parentNode() ) {
      // ...this node and all it's siblings and descendants
      node->setProcessed( false, true );
    }

    for ( ; node ; node = node->nextSibling() ) {
      if ( node->processed() )
        continue;

      ProcessResult processResult;

      if ( const Interface::BodyPartFormatter * formatter
           = BodyPartFormatterFactory::instance()->createFor( node->typeString(), node->subTypeString() ) ) {
        PartNodeBodyPart part( *node, codecFor( node ) );
        const Interface::BodyPartFormatter::Result result = formatter->format( &part, htmlWriter() );
        if ( mReader && node->bodyPartMemento() )
          if ( Interface::Observable * obs = node->bodyPartMemento()->asObservable() )
            obs->attach( mReader );
        switch ( result ) {
        case Interface::BodyPartFormatter::AsIcon:
          processResult.setNeverDisplayInline( true );
          // fall through:
        case Interface::BodyPartFormatter::Failed:
          defaultHandling( node, processResult );
          break;
        case Interface::BodyPartFormatter::Ok:
        case Interface::BodyPartFormatter::NeedContent:
          // FIXME: incomplete content handling
          ;
        }
      } else {
        const BodyPartFormatter * bpf
          = BodyPartFormatter::createFor( node->type(), node->subType() );
        kdFatal( !bpf, 5006 ) << "THIS SHOULD NO LONGER HAPPEN ("
                              << node->typeString() << '/' << node->subTypeString()
                              << ')' << endl;

        if ( !bpf->process( this, node, processResult ) )
          defaultHandling( node, processResult );
      }
      node->setProcessed( true, false );

      // adjust signed/encrypted flags if inline PGP was found
      processResult.adjustCryptoStatesOfNode( node );

      if ( showOnlyOneMimePart() )
        break;
    }
  }

  void ObjectTreeParser::defaultHandling( partNode * node, ProcessResult & result ) {
    // ### (mmutz) default handling should go into the respective
    // ### bodypartformatters.
    if ( !mReader )
      return;
    if ( attachmentStrategy() == AttachmentStrategy::hidden() &&
         !showOnlyOneMimePart() &&
         node->parentNode() /* message is not an attachment */ )
      return;

    bool asIcon = true;
    if ( showOnlyOneMimePart() )
      // ### (mmutz) this is wrong! If I click on an image part, I
      // want the equivalent of "view...", except for the extra
      // window!
      asIcon = !node->hasContentDispositionInline();
    else if ( !result.neverDisplayInline() )
      if ( const AttachmentStrategy * as = attachmentStrategy() )
        asIcon = as->defaultDisplay( node ) == AttachmentStrategy::AsIcon;
    // neither image nor text -> show as icon
    if ( !result.isImage()
         && node->type() != DwMime::kTypeText )
      asIcon = true;
    if ( asIcon ) {
      if ( attachmentStrategy() != AttachmentStrategy::hidden()
           || showOnlyOneMimePart() )
        writePartIcon( &node->msgPart(), node->nodeId() );
    } else if ( result.isImage() )
      writePartIcon( &node->msgPart(), node->nodeId(), true );
    else
      writeBodyString( node->msgPart().bodyDecoded(),
                       node->trueFromAddress(),
                       codecFor( node ), result );
    // end of ###
  }

  void ProcessResult::adjustCryptoStatesOfNode( partNode * node ) const {
    if ( ( inlineSignatureState()  != KMMsgNotSigned ) ||
         ( inlineEncryptionState() != KMMsgNotEncrypted ) ) {
      node->setSignatureState( inlineSignatureState() );
      node->setEncryptionState( inlineEncryptionState() );
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
                                                      CryptPlug::SignatureMetaData* paramSigMeta,
                                                      bool hideErrors )
  {
    bool bIsOpaqueSigned = false;
    enum { NO_PLUGIN, NOT_INITIALIZED, CANT_VERIFY_SIGNATURES }
      cryptPlugError = NO_PLUGIN;

    CryptPlugWrapper* cryptPlug = cryptPlugWrapper();
    if ( !cryptPlug )
      cryptPlug = CryptPlugFactory::instance()->active();

    QString cryptPlugLibName;
    QString cryptPlugDisplayName;
    if ( cryptPlug ) {
      cryptPlugLibName = cryptPlug->libName();
      if ( cryptPlug == CryptPlugFactory::instance()->openpgp() )
        cryptPlugDisplayName = "OpenPGP";
      else if ( cryptPlug == CryptPlugFactory::instance()->smime() )
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
      if ( data ) {
        cleartext = data->dwPart()->AsString().c_str();

        dumpToFile( "dat_01_reader_signedtext_before_canonicalization",
                    cleartext.data(), cleartext.length() );

        // replace simple LFs by CRLSs
        // according to RfC 2633, 3.1.1 Canonicalization
        kdDebug(5006) << "Converting LF to CRLF (see RfC 2633, 3.1.1 Canonicalization)" << endl;
        cleartext = KMMessage::lf2crlf( cleartext );
        kdDebug(5006) << "                                                       done." << endl;
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
    }

    CryptPlug::SignatureMetaData localSigMeta;
    if ( doCheck ){
      localSigMeta.status              = 0;
      localSigMeta.extended_info       = 0;
      localSigMeta.extended_info_count = 0;
    }
    CryptPlug::SignatureMetaData* sigMeta = doCheck ? &localSigMeta : paramSigMeta;

    const char* cleartextP = cleartext;
    PartMetaData messagePart;
    messagePart.isSigned = true;
    messagePart.technicalProblem = ( cryptPlug == 0 );
    messagePart.isGoodSignature = false;
    messagePart.isEncrypted = false;
    messagePart.isDecryptable = false;
    messagePart.keyTrust = Kpgp::KPGP_VALIDITY_UNKNOWN;
    messagePart.status = i18n("Wrong Crypto Plug-In.");

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

      CryptPlug::SignatureMetaDataExtendedInfo& ext = sigMeta->extended_info[0];

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
        if ( ext.emailList[ iMail ] && *ext.emailList[ iMail ] ) {
          QString email = QString::fromUtf8( ext.emailList[ iMail ] );
          // ### work around gpgme 0.3.x / cryptplug bug where the
          // ### email addresses are specified as angle-addr, not addr-spec:
          if ( email.startsWith( "<" ) && email.endsWith( ">" ) )
            email = email.mid( 1, email.length() - 2 );
          messagePart.signerMailAddresses.append( email );
        }
      if ( ext.creation_time )
        messagePart.creationTime = *ext.creation_time;
      if (     70 > messagePart.creationTime.tm_year
          || 200 < messagePart.creationTime.tm_year
          ||   0 > messagePart.creationTime.tm_mon
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
        if ( !messagePart.signerMailAddresses.empty() ) {
          if ( messagePart.signer.isEmpty() )
            messagePart.signer = messagePart.signerMailAddresses.front();
          else
            messagePart.signer += " <" + messagePart.signerMailAddresses.front() + '>';
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

    if ( !doCheck || !data ){
      if ( cleartextData || new_cleartext ) {
        if ( mReader )
          htmlWriter()->queue( writeSigstatHeader( messagePart,
                                                   cryptPlug,
                                                   fromAddress ) );
        bIsOpaqueSigned = true;

        CryptPlugWrapperSaver cpws( this, cryptPlug );
        insertAndParseNewChildNode( sign,
                                    doCheck ? new_cleartext
                                            : cleartextData->data(),
                                    "opaqued signed data" );
        if ( doCheck )
          free( new_cleartext );

        if ( mReader )
          htmlWriter()->queue( writeSigstatFooter( messagePart ) );

      }
      else if ( !hideErrors ) {
        QString txt;
        txt = "<hr><b><h2>";
        txt.append( i18n( "The crypto engine returned no cleartext data." ) );
        txt.append( "</h2></b>" );
        txt.append( "<br>&nbsp;<br>" );
        txt.append( i18n( "Status: " ) );
        if ( sigMeta->status && *sigMeta->status ) {
          txt.append( "<i>" );
          txt.append( sigMeta->status );
          txt.append( "</i>" );
        }
        else
          txt.append( i18n("(unknown)") );
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
            errorMsg = i18n( "Crypto plug-in \"%1\" cannot verify signatures." )
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
                                        "validity of the signature cannot be "
                                        "verified.<br />"
                                        "Reason: %1" )
                                  .arg( errorMsg );
        }

        htmlWriter()->queue( writeSigstatHeader( messagePart,
                                                 cryptPlug,
                                                 fromAddress ) );
      }

      ObjectTreeParser otp( mReader, cryptPlug, true );
      otp.parseObjectTree( data );
      mRawReplyString += otp.rawReplyString();
      mTextualContent += otp.textualContent();
      if ( !otp.textualContentCharset().isEmpty() )
	mTextualContentCharset = otp.textualContentCharset();

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
                                      CryptPlug::SignatureMetaData& sigMeta,
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
    cryptPlug = CryptPlugFactory::instance()->active();

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

  if ( cryptPlug && !kmkernel->contextMenuShown() ) {
    QByteArray ciphertext( data.msgPart().bodyDecodedBinary() );
    QCString cipherStr( ciphertext.data(), ciphertext.size() + 1 );
    bool cipherIsBinary = (-1 == cipherStr.find("BEGIN ENCRYPTED MESSAGE", 0, false) ) &&
                          (-1 == cipherStr.find("BEGIN PGP ENCRYPTED MESSAGE", 0, false) ) &&
                          (-1 == cipherStr.find("BEGIN PGP MESSAGE", 0, false) );
    int cipherLen = ciphertext.size();

    dumpToFile( "dat_04_reader.encrypted", ciphertext.data(), ciphertext.size() );

#ifdef MARCS_DEBUG
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
    bDecryptionOk = cryptPlug->decryptAndCheckMessage( ciphertext.data(),
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
                    + i18n("Encrypted data not shown.").utf8()
                    + "</div>";
      if ( !passphraseError )
        aErrorText = i18n("Crypto plug-in \"%1\" could not decrypt the data.")
                       .arg( cryptPlugLibName )
                   + "<br />"
                   + i18n("Error: %1").arg( aErrorText );
    }
    if ( errTxt )
      free( errTxt );
    if ( cleartext )
      free( cleartext );
  }
  else if ( !cryptPlug ) {
    decryptedData = "<div style=\"text-align:center; padding:20pt;\">"
                  + i18n("Encrypted data not shown.").utf8()
                  + "</div>";
    switch ( cryptPlugError ) {
    case NOT_INITIALIZED:
      aErrorText = i18n( "Crypto plug-in \"%1\" is not initialized." )
                     .arg( cryptPlugLibName );
      break;
    case CANT_DECRYPT:
      aErrorText = i18n( "Crypto plug-in \"%1\" cannot decrypt messages." )
                     .arg( cryptPlugLibName );
      break;
    case NO_PLUGIN:
      aErrorText = i18n( "No appropriate crypto plug-in was found." );
      break;
    }
  }
  else {
    // ### Workaround for bug 56693 (kmail freeze with the complete desktop
    // ### while pinentry-qt appears)
    QByteArray ciphertext( data.msgPart().bodyDecodedBinary() );
    QCString cipherStr( ciphertext.data(), ciphertext.size() + 1 );
    bool cipherIsBinary = (-1 == cipherStr.find("BEGIN ENCRYPTED MESSAGE", 0, false) ) &&
                          (-1 == cipherStr.find("BEGIN PGP ENCRYPTED MESSAGE", 0, false) ) &&
                          (-1 == cipherStr.find("BEGIN PGP MESSAGE", 0, false) );
    if ( !cipherIsBinary ) {
      decryptedData = cipherStr;
    }
    else {
      decryptedData = "<div style=\"font-size:x-large; text-align:center;"
                      "padding:20pt;\">"
                    + i18n("Encrypted data not shown.").utf8()
                    + "</div>";
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

    if (!KPIM::kByteArrayToFile(theBody, fname, false, false, false))
      bOk = false;
    if ( reader )
      reader->mTempFiles.append(fname);
  }
  return bOk ? fname : QString::null;
}

  bool ObjectTreeParser::processTextHtmlSubtype( partNode * curNode, ProcessResult & ) {
    QCString cstr( curNode->msgPart().bodyDecoded() );

    mRawReplyString = cstr;
    if ( curNode->isFirstTextPart() ) {
      mTextualContent += curNode->msgPart().bodyToUnicode();
      mTextualContentCharset = curNode->msgPart().charset();
    }

    if ( !mReader )
      return true;

    if ( curNode->isFirstTextPart() ||
         attachmentStrategy()->defaultDisplay( curNode ) == AttachmentStrategy::Inline ||
         showOnlyOneMimePart() )
    {
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

  bool ObjectTreeParser::processTextVCalSubtype( partNode * curNode,
                                                 ProcessResult & result ) {
    // It doesn't make sense to display raw vCals inline
    result.setNeverDisplayInline( true );

    // For special treatment of invitations etc. we need a reader window
    if ( !mReader )
      return false;

    QString iCal = curNode->msgPart().bodyToUnicode();
    QString html = kmkernel->groupware().vPartToHTML( iCal );
    if( !html.isEmpty() )
      htmlWriter()->queue( html );
    htmlWriter()->queue( quotedHTML( iCal ) );

    return true;
  }

} // namespace KMail

static bool isMailmanMessage( partNode * curNode ) {
  if ( !curNode->dwPart() || !curNode->dwPart()->hasHeaders() )
    return false;
  DwHeaders & headers = curNode->dwPart()->Headers();
  if ( headers.HasField("X-Mailman-Version") )
    return true;
  if ( headers.HasField("X-Mailer") &&
       0 == QCString( headers.FieldBody("X-Mailer").AsString().c_str() )
       .find("MAILMAN", 0, false) )
    return true;
  return false;
}

namespace KMail {

  bool ObjectTreeParser::processMailmanMessage( partNode * curNode ) {
    const QCString cstr = curNode->msgPart().bodyDecoded();

    //###
    const QCString delim1( "--__--__--\n\nMessage:");
    const QCString delim2( "--__--__--\r\n\r\nMessage:");
    const QCString delimZ2("--__--__--\n\n_____________");
    const QCString delimZ1("--__--__--\r\n\r\n_____________");
    QCString partStr, digestHeaderStr;
    int thisDelim = cstr.find(delim1, 0, false);
    if ( thisDelim == -1 )
      thisDelim = cstr.find(delim2, 0, false);
    if ( thisDelim == -1 ) {
      kdDebug(5006) << "        Sorry: Old style Mailman message but no delimiter found." << endl;
      return false;
    }

    int nextDelim = cstr.find(delim1, thisDelim+1, false);
    if ( -1 == nextDelim )
      nextDelim = cstr.find(delim2, thisDelim+1, false);
    if ( -1 == nextDelim )
      nextDelim = cstr.find(delimZ1, thisDelim+1, false);
    if ( -1 == nextDelim )
      nextDelim = cstr.find(delimZ2, thisDelim+1, false);
    if ( nextDelim < 0)
      return false;

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
    return true;
  }

  bool ObjectTreeParser::processTextPlainSubtype( partNode * curNode, ProcessResult & result ) {
    const QCString cstr = curNode->msgPart().bodyDecoded();
    if ( !mReader ) {
      mRawReplyString = cstr;
      if ( curNode->isFirstTextPart() ) {
	mTextualContent += curNode->msgPart().bodyToUnicode();
	mTextualContentCharset = curNode->msgPart().charset();
      }
      return true;
    }

    //resultingRawData += cstr;
    if ( !curNode->isFirstTextPart() &&
         attachmentStrategy()->defaultDisplay( curNode ) != AttachmentStrategy::Inline &&
         !showOnlyOneMimePart() )
      return false;

    mRawReplyString = cstr;
    if ( curNode->isFirstTextPart() ) {
      mTextualContent += curNode->msgPart().bodyToUnicode();
      mTextualContentCharset = curNode->msgPart().charset();
    }

    QString label = curNode->msgPart().fileName().stripWhiteSpace();
    if ( label.isEmpty() )
      label = curNode->msgPart().name().stripWhiteSpace();

    const bool bDrawFrame = !curNode->isFirstTextPart()
                          && !showOnlyOneMimePart()
                          && !label.isEmpty();
    if ( bDrawFrame ) {
      label = KMMessage::quoteHtmlChars( label, true );

      const QString comment =
        KMMessage::quoteHtmlChars( curNode->msgPart().contentDescription(), true );

      const QString fileName =
        mReader->writeMessagePartToTempFile( &curNode->msgPart(),
                                             curNode->nodeId() );

      const QString dir = QApplication::reverseLayout() ? "rtl" : "ltr" ;

      QString htmlStr = "<table cellspacing=\"1\" class=\"textAtm\">"
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
    if ( !isMailmanMessage( curNode ) ||
         !processMailmanMessage( curNode ) )
      writeBodyString( cstr, curNode->trueFromAddress(),
                       codecFor( curNode ), result );
    if ( bDrawFrame )
      htmlWriter()->queue( "</td></tr></table>" );

    return true;
  }

  void ObjectTreeParser::stdChildHandling( partNode * child ) {
    if ( !child )
      return;

    ObjectTreeParser otp( *this );
    otp.setShowOnlyOneMimePart( false );
    otp.parseObjectTree( child );
    mRawReplyString += otp.rawReplyString();
    mTextualContent += otp.textualContent();
    if ( !otp.textualContentCharset().isEmpty() )
      mTextualContentCharset = otp.textualContentCharset();
  }

  bool ObjectTreeParser::processMultiPartMixedSubtype( partNode * node, ProcessResult & ) {
    partNode * child = node->firstChild();
    if ( !child )
      return false;

#if 0
    // Might be a Kroupware message,
    // let's look for the parts contained in the mixture:
    partNode * dataPlain = child->findType( DwMime::kTypeText,
                                            DwMime::kSubtypePlain, false, true );

    // special treatment of vCal attachment (might be invitation or similar)
    partNode * dataCal = child->findType( DwMime::kTypeText,
                                          DwMime::kSubtypeVCal, false, true );
    if ( dataCal ) {
      ProcessResult dummy;
      if ( processTextVCalSubtype( dataCal, dummy ) ) {
        // Kroupware message found,
        // we can ignore the plain text part
        if ( dataPlain )
          dataPlain->setProcessed( true, false );
        return true;
      }
    }
#endif

#if 0
    // special treatment of TNEF attachment (might be invitation or similar)
    partNode * dataTNEF = child->findType( DwMime::kTypeApplication,
                                           DwMime::kSubtypeMsTNEF, false, true );
    if ( dataTNEF ) {
      ProcessResult dummy;
      if ( processApplicationMsTnefSubtype( dataTNEF, dummy ) ) {
        // encoded Kroupware message found,
        // we can ignore the plain text part
        if ( dataPlain )
          dataPlain->setProcessed( true, false );
        return true;
      }
    }
#endif

    // normal treatment of the parts in the mp/mixed container
    stdChildHandling( child );
    return true;
  }

  bool ObjectTreeParser::processMultiPartAlternativeSubtype( partNode * node, ProcessResult & ) {
    partNode * child = node->firstChild();
    if ( !child )
      return false;

    partNode * dataHtml = child->findType( DwMime::kTypeText,
                                           DwMime::kSubtypeHtml, false, true );
    partNode * dataPlain = child->findType( DwMime::kTypeText,
                                            DwMime::kSubtypePlain, false, true );

    if ( (mReader && mReader->htmlMail() && dataHtml) ||
         (dataHtml && dataPlain && dataPlain->msgPart().body().isEmpty()) ) {
      if ( dataPlain )
        dataPlain->setProcessed( true, false );
      stdChildHandling( dataHtml );
      return true;
    }

    if ( !mReader || (!mReader->htmlMail() && dataPlain) ) {
      if ( dataHtml )
        dataHtml->setProcessed( true, false );
      stdChildHandling( dataPlain );
      return true;
    }

    stdChildHandling( child );
    return true;
  }

  bool ObjectTreeParser::processMultiPartDigestSubtype( partNode * node, ProcessResult & result ) {
    return processMultiPartMixedSubtype( node, result );
  }

  bool ObjectTreeParser::processMultiPartParallelSubtype( partNode * node, ProcessResult & result ) {
    return processMultiPartMixedSubtype( node, result );
  }

  bool ObjectTreeParser::processMultiPartSignedSubtype( partNode * node, ProcessResult & ) {
    if ( node->childCount() != 2 ) {
      kdDebug(5006) << "mulitpart/signed must have exactly two child parts!" << endl
                    << "processing as multipart/mixed" << endl;
      if ( node->firstChild() )
        stdChildHandling( node->firstChild() );
      return node->firstChild();
    }

    partNode * signedData = node->firstChild();
    assert( signedData );

    partNode * signature = signedData->nextSibling();
    assert( signature );

    signature->setProcessed( true, true );

    if ( !includeSignatures() ) {
      stdChildHandling( signedData );
      return true;
    }

    // FIXME(marc) check here that the protocol parameter matches the
    // mimetype of "signature" (not required by the RFC, but practised
    // by all implementaions of security multiparts

    CryptPlugWrapper * cpw =
      CryptPlugFactory::instance()->createForProtocol( node->contentTypeParameter( "protocol" ) );

    if ( !cpw ) {
      signature->setProcessed( true, true );
      stdChildHandling( signedData );
      return true;
    }

    CryptPlugWrapperSaver saver( this, cpw );

    node->setSignatureState( KMMsgFullySigned );
    writeOpaqueOrMultipartSignedData( signedData, *signature,
                                      node->trueFromAddress() );
    return true;
  }

  bool ObjectTreeParser::processMultiPartEncryptedSubtype( partNode * node, ProcessResult & result ) {
    partNode * child = node->firstChild();
    if ( !child )
      return false;

    if ( keepEncryptions() ) {
      node->setEncryptionState( KMMsgFullyEncrypted );
      const QCString cstr = node->msgPart().bodyDecoded();
      if ( mReader )
        writeBodyString( cstr, node->trueFromAddress(),
                         codecFor( node ), result );
      mRawReplyString += cstr;
      return true;
    }

    CryptPlugWrapper * useThisCryptPlug = 0;

    /*
      ATTENTION: This code is to be replaced by the new 'auto-detect' feature. --------------------------------------
    */
    partNode * data = child->findType( DwMime::kTypeApplication,
                                       DwMime::kSubtypeOctetStream, false, true );
    if ( data ) {
      useThisCryptPlug = KMail::CryptPlugFactory::instance()->openpgp();
    }
    if ( !data ) {
      data = child->findType( DwMime::kTypeApplication,
                              DwMime::kSubtypePkcs7Mime, false, true );
      if ( data ) {
        useThisCryptPlug = KMail::CryptPlugFactory::instance()->smime();
      }
    }
    /*
      ---------------------------------------------------------------------------------------------------------------
    */

    if ( !data ) {
      stdChildHandling( child );
      return true;
    }

    CryptPlugWrapperSaver cpws( this, useThisCryptPlug );

    if ( partNode * dataChild = data->firstChild() ) {
      kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
      stdChildHandling( dataChild );
      kdDebug(5006) << "\n----->  Returning from parseObjectTree( curNode->mChild )\n" << endl;
      return true;
    }

    kdDebug(5006) << "\n----->  Initially processing encrypted data\n" << endl;
    PartMetaData messagePart;
    node->setEncryptionState( KMMsgFullyEncrypted );
    QCString decryptedData;
    bool signatureFound;
    CryptPlug::SignatureMetaData sigMeta;
    sigMeta.status              = 0;
    sigMeta.extended_info       = 0;
    sigMeta.extended_info_count = 0;
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
                                               node->trueFromAddress() ) );
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
                                          *node,
                                          node->trueFromAddress(),
                                          false,
                                          &decryptedData,
                                          &sigMeta,
                                          false );
        node->setSignatureState( KMMsgFullySigned );
      } else {
        insertAndParseNewChildNode( *node,
                                    &*decryptedData,
                                    "encrypted data" );
      }
    } else {
      mRawReplyString += decryptedData;
      if ( mReader ) {
        // print the error message that was returned in decryptedData
        // (utf8-encoded)
        htmlWriter()->queue( QString::fromUtf8( decryptedData.data() ) );
      }
    }

    if ( mReader )
      htmlWriter()->queue( writeSigstatFooter( messagePart ) );
    data->setProcessed( true, false ); // Set the data node to done to prevent it from being processed
    return true;
  }


  bool ObjectTreeParser::processMessageRfc822Subtype( partNode * node, ProcessResult & ) {
    if ( mReader
         && !attachmentStrategy()->inlineNestedMessages()
         && !showOnlyOneMimePart() )
      return false;

    if ( partNode * child = node->firstChild() ) {
      kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
      ObjectTreeParser otp( mReader, cryptPlugWrapper() );
      otp.parseObjectTree( child );
      mRawReplyString += otp.rawReplyString();
      mTextualContent += otp.textualContent();
      if ( !otp.textualContentCharset().isEmpty() )
	mTextualContentCharset = otp.textualContentCharset();
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
      QString filename =
        mReader->writeMessagePartToTempFile( &node->msgPart(),
                                            node->nodeId() );
      htmlWriter()->queue( writeSigstatHeader( messagePart,
                                               cryptPlugWrapper(),
                                               node->trueFromAddress(),
                                               filename ) );
    }
    QCString rfc822messageStr( node->msgPart().bodyDecoded() );
    // display the headers of the encapsulated message
    DwMessage* rfc822DwMessage = 0; // will be deleted by c'tor of rfc822headers
    if ( node->dwPart()->Body().Message() )
      rfc822DwMessage = new DwMessage( *(node->dwPart()->Body().Message()) );
    else
    {
      rfc822DwMessage = new DwMessage();
      rfc822DwMessage->FromString( rfc822messageStr );
      rfc822DwMessage->Parse();
    }
    KMMessage rfc822message( rfc822DwMessage );
    node->setFromAddress( rfc822message.from() );
    kdDebug(5006) << "\n----->  Store RfC 822 message header \"From: " << rfc822message.from() << "\"\n" << endl;
    if ( mReader )
      htmlWriter()->queue( mReader->writeMsgHeader( &rfc822message ) );
      //mReader->parseMsgHeader( &rfc822message );
    // display the body of the encapsulated message
    insertAndParseNewChildNode( *node,
                                &*rfc822messageStr,
                                "encapsulated message" );
    if ( mReader )
      htmlWriter()->queue( writeSigstatFooter( messagePart ) );
    return true;
  }


  bool ObjectTreeParser::processApplicationOctetStreamSubtype( partNode * node, ProcessResult & result ) {
    if ( partNode * child = node->firstChild() ) {
      kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
      ObjectTreeParser otp( mReader, cryptPlugWrapper() );
      otp.parseObjectTree( child );
      mRawReplyString += otp.rawReplyString();
      mTextualContent += otp.textualContent();
      if ( !otp.textualContentCharset().isEmpty() )
	mTextualContentCharset = otp.textualContentCharset();
      kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
      return true;
    }

    CryptPlugWrapper* oldUseThisCryptPlug = cryptPlugWrapper();
    if (    node->parentNode()
            && DwMime::kTypeMultipart    == node->parentNode()->type()
            && DwMime::kSubtypeEncrypted == node->parentNode()->subType() ) {
      kdDebug(5006) << "\n----->  Initially processing encrypted data\n" << endl;
      node->setEncryptionState( KMMsgFullyEncrypted );
      if ( keepEncryptions() ) {
        const QCString cstr = node->msgPart().bodyDecoded();
        if ( mReader )
          writeBodyString( cstr, node->trueFromAddress(),
                           codecFor( node ), result );
        mRawReplyString += cstr;
      } else {
        /*
          ATTENTION: This code is to be replaced by the planned 'auto-detect' feature.
        */
        PartMetaData messagePart;
        setCryptPlugWrapper( KMail::CryptPlugFactory::instance()->openpgp() );
        QCString decryptedData;
        bool signatureFound;
        CryptPlug::SignatureMetaData sigMeta;
        sigMeta.status              = 0;
        sigMeta.extended_info       = 0;
        sigMeta.extended_info_count = 0;
        bool passphraseError;

        bool bOkDecrypt = okDecryptMIME( *node,
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
                                                   node->trueFromAddress() ) );
        }

        if ( bOkDecrypt ) {
          // fixing the missing attachments bug #1090-b
          insertAndParseNewChildNode( *node,
                                      &*decryptedData,
                                      "encrypted data" );
        } else {
          mRawReplyString += decryptedData;
          if ( mReader ) {
            // print the error message that was returned in decryptedData
            // (utf8-encoded)
            htmlWriter()->queue( QString::fromUtf8( decryptedData.data() ) );
          }
        }

        if ( mReader )
          htmlWriter()->queue( writeSigstatFooter( messagePart ) );
      }
      return true;
    }
    setCryptPlugWrapper( oldUseThisCryptPlug );
    return false;
  }

  bool ObjectTreeParser::processApplicationPkcs7MimeSubtype( partNode * node, ProcessResult & result ) {
    if ( partNode * child = node->firstChild() ) {
      kdDebug(5006) << "\n----->  Calling parseObjectTree( curNode->mChild )\n" << endl;
      ObjectTreeParser otp( mReader, cryptPlugWrapper() );
      otp.parseObjectTree( child );
      mRawReplyString += otp.rawReplyString();
      mTextualContent += otp.textualContent();
      if ( !otp.textualContentCharset().isEmpty() )
	mTextualContentCharset = otp.textualContentCharset();
      kdDebug(5006) << "\n<-----  Returning from parseObjectTree( curNode->mChild )\n" << endl;
      return true;
    }

    kdDebug(5006) << "\n----->  Initially processing signed and/or encrypted data\n" << endl;
    if ( !node->dwPart() || !node->dwPart()->hasHeaders() )
      return false;

    CryptPlugWrapper * smimeCrypto = CryptPlugFactory::instance()->smime();

    const QString smimeType = node->contentTypeParameter("smime-type").lower();

    if ( smimeType == "certs-only" ) {
      result.setNeverDisplayInline( true );
      if ( !smimeCrypto )
        return false;

      const KConfigGroup reader( KMKernel::config(), "Reader" );
      if ( !reader.readBoolEntry( "AutoImportKeys", false ) )
        return false;

      const QByteArray certData = node->msgPart().bodyDecodedBinary();

      const GpgME::ImportResult res
        = smimeCrypto->importCertificate( certData.data(), certData.size() );
      if ( res.error() ) {
        htmlWriter()->queue( i18n( "Sorry, certificate could not be imported.<br>"
                                   "Reason: %1").arg( QString::fromLocal8Bit( res.error().asString() ) ) );
        return true;
      }

      const int nImp = res.numImported();
      const int nUnc = res.numUnchanged();
      const int nSKImp = res.numSecretKeysImported();
      const int nSKUnc = res.numSecretKeysUnchanged();
      if ( !nImp && !nSKImp && !nUnc && !nSKUnc ) {
        htmlWriter()->queue( i18n( "Sorry, no certificates were found in this message." ) );
        return true;
      }
      QString comment = "<b>" + i18n( "Certificate import status:" ) + "</b><br>&nbsp;<br>";
      if ( nImp )
        comment += i18n( "1 new certificate was imported.",
                         "%n new certificates were imported.", nImp ) + "<br>";
      if ( nUnc )
        comment += i18n( "1 certificate was unchanged.",
                         "%n certificates were unchanged.", nUnc ) + "<br>";
      if ( nSKImp )
        comment += i18n( "1 new secret key was imported.",
                         "%n new secret keys were imported.", nSKImp ) + "<br>";
      if ( nSKUnc )
        comment += i18n( "1 secret key was unchanged.",
                         "%n secret keys were unchanged.", nSKUnc ) + "<br>";
      comment += "&nbsp;<br>";
      htmlWriter()->queue( comment );
      if ( !nImp && !nSKImp ) {
        htmlWriter()->queue( "<hr>" );
        return true;
      }
      const std::vector<GpgME::Import> imports = res.imports();
      if ( imports.empty() ) {
        htmlWriter()->queue( i18n( "Sorry, no details on certificate import available." ) + "<hr>" );
        return true;
      }
      htmlWriter()->queue( "<b>" + i18n( "Certificate import details:" ) + "</b><br>" );
      for ( std::vector<GpgME::Import>::const_iterator it = imports.begin() ; it != imports.end() ; ++it ) {
        if ( (*it).error() )
          htmlWriter()->queue( i18n( "Failed: %1 (%2)" ).arg( (*it).fingerprint() )
                               .arg( QString::fromLocal8Bit( (*it).error().asString() ) ) );
        else if ( (*it).status() & ~GpgME::Import::ContainedSecretKey )
          if ( (*it).status() & GpgME::Import::ContainedSecretKey )
            htmlWriter()->queue( i18n( "New or changed: %1 (secret key available)" ).arg( (*it).fingerprint() ) );
          else
            htmlWriter()->queue( i18n( "New or changed: %1" ).arg( (*it).fingerprint() ) );
        htmlWriter()->queue( "<br>" );
      }

      htmlWriter()->queue( "<hr>" );
      return true;
    }

    if ( !smimeCrypto )
      return false;
    CryptPlugWrapperSaver cpws( this, smimeCrypto );

    bool isSigned      = smimeType == "signed-data";
    bool isEncrypted   = smimeType == "enveloped-data";

    // Analyze "signTestNode" node to find/verify a signature.
    // If zero this verification was successfully done after
    // decrypting via recursion by insertAndParseNewChildNode().
    partNode* signTestNode = isEncrypted ? 0 : node;


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
      CryptPlug::SignatureMetaData sigMeta;
      sigMeta.status              = 0;
      sigMeta.extended_info       = 0;
      sigMeta.extended_info_count = 0;
      bool passphraseError;

      if ( okDecryptMIME( *node,
                          decryptedData,
                          signatureFound,
                          sigMeta,
                          false,
                          passphraseError,
                          messagePart.errorText ) ) {
        kdDebug(5006) << "pkcs7 mime  -  encryption found  -  enveloped (encrypted) data !" << endl;
        isEncrypted = true;
        node->setEncryptionState( KMMsgFullyEncrypted );
        signTestNode = 0;
        // paint the frame
        messagePart.isDecryptable = true;
        if ( mReader )
          htmlWriter()->queue( writeSigstatHeader( messagePart,
                                                   cryptPlugWrapper(),
                                                   node->trueFromAddress() ) );
        insertAndParseNewChildNode( *node,
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
                                                     node->trueFromAddress() ) );
            writePartIcon( &node->msgPart(), node->nodeId() );
            htmlWriter()->queue( writeSigstatFooter( messagePart ) );
          }
        } else {
          kdDebug(5006) << "pkcs7 mime  -  NO encryption found" << endl;
        }
      }
      if ( isEncrypted )
        node->setEncryptionState( KMMsgFullyEncrypted );
    }

    // We now try signature verification if necessarry.
    if ( signTestNode ) {
      if ( isSigned )
        kdDebug(5006) << "pkcs7 mime     ==      S/MIME TYPE: opaque signed data" << endl;
      else
        kdDebug(5006) << "pkcs7 mime  -  type unknown  -  opaque signed data ?" << endl;

      bool sigFound = writeOpaqueOrMultipartSignedData( 0,
                                                        *signTestNode,
                                                        node->trueFromAddress(),
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
        if ( signTestNode != node )
          node->setSignatureState( KMMsgFullySigned );
      } else {
        kdDebug(5006) << "pkcs7 mime  -  NO signature found   :-(" << endl;
      }
    }

    return isSigned || isEncrypted;
  }

  bool ObjectTreeParser::processApplicationMsTnefSubtype( partNode * curNode,
                                                          ProcessResult & )
  {
    // For special treatment of invitations etc. we need a reader window
    if ( !mReader )
      return false;

    QByteArray theBody = curNode->msgPart().bodyDecodedBinary();
    QString html = kmkernel->groupware().msTNEFToHTML( theBody );
    if( !html.isEmpty() && theBody.size() > 0 )
      htmlWriter()->queue( html );
    else
      return false;

    return true;
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
        if( cryptPlug->protocol() == "openpgp" ) {
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
                result = i18n("<b>Bad</b> signature");
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
        else if( cryptPlug->protocol() == "smime" ) {
            // process status bits according to SigStatus_...
            // definitions in kdenetwork/libkdenetwork/cryptplug.h

            if( CryptPlugWrapper::SigStatus_UNKNOWN == statusFlags ) {
                result = i18n("No status information available.");
                frameColor = SIG_FRAME_COL_YELLOW;
                showKeyInfos = false;
                return result;
            }

            if( CryptPlugWrapper::SigStatus_VALID & statusFlags ) {
                result = i18n("Good signature.");
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
                result = i18n("Good signature.");
            } else if( SIG_FRAME_COL_RED == frameColor ) {
                result = i18n("<b>Bad</b> signature.");
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
                                              const QString & fromAddress,
                                              const QString & filename )
{
    bool isSMIME = cryptPlug && cryptPlug->protocol() == "smime";
    QString signer = block.signer;

    QString htmlStr;
    QString dir = ( QApplication::reverseLayout() ? "rtl" : "ltr" );
    QString cellPadding("cellpadding=\"1\"");

    if( block.isEncapsulatedRfc822Message )
    {
        htmlStr += "<table cellspacing=\"1\" "+cellPadding+" class=\"rfc822\">"
            "<tr class=\"rfc822H\"><td dir=\"" + dir + "\">";
        if( !filename.isEmpty() )
            htmlStr += "<a href=\"" + QString("file:")
                     + KURL::encode_string( filename ) + "\">"
                     + i18n("Encapsulated message") + "</a>";
        else
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
                    QString msgFrom( KPIM::getEmailAddr(fromAddress) );
                    QString certificate;
                    if( block.keyId.isEmpty() )
                        certificate = "certificate";
                    else
                        certificate = QString("%1%2</a>")
                                      .arg( startKeyHREF )
                                      .arg( "certificate" );
                    if( !blockAddrs.empty() ){
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
                                msgFrom +
                                "<br />" +
                                i18n("stored: ");
                            // We cannot use Qt's join() function here but
                            // have to join the addresses manually to
                            // extract the mail addresses (without '<''>')
                            // before including it into our string:
                            bool bStart = true;
                            for(QStringList::ConstIterator it = blockAddrs.begin();
                                it != blockAddrs.end(); ++it ){
                                if( !bStart )
                                    greenCaseWarning.append(", <br />&nbsp; &nbsp;");
                                bStart = false;
                                greenCaseWarning.append( KPIM::getEmailAddr(*it) );
                            }
                        }
                    } else {
                        greenCaseWarning =
                            "<u>" +
                            i18n("Warning:") +
                            "</u> " +
                            i18n("No mail address is stored in the %1 used for signing, "
                                 "so we cannot compare it to the sender's address %2.")
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
                        if( !blockAddrs.empty() ){
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
                        bool dateOK = ( 0 < block.creationTime.tm_year &&
                                        block.creationTime.tm_year < 3000 );
                        QDateTime created;
                        if ( dateOK )
                          created.setTime_t( mktime(&block.creationTime) );
                        if( dateOK && created.isValid() ) {
                            if( signer.isEmpty() ) {
                                if( onlyShowKeyURL )
                                    htmlStr += i18n( "Message was signed with key %1." )
                                                .arg( keyWithWithoutURL );
                                else
                                    htmlStr += i18n( "Message was signed on %1 with key %2." )
                                                .arg( KGlobal::locale()->formatDateTime( created ) )
                                                .arg( keyWithWithoutURL );
                            }
                            else {
                                if( onlyShowKeyURL )
                                    htmlStr += i18n( "Message was signed with key %1." )
                                            .arg( keyWithWithoutURL );
                                else
                                    htmlStr += i18n( "Message was signed by %3 on %1 with key %2" )
                                            .arg( KGlobal::locale()->formatDateTime( created ) )
                                            .arg( keyWithWithoutURL )
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
                    bool dateOK = ( 0 < block.creationTime.tm_year &&
                                    block.creationTime.tm_year < 3000 );
                    QDateTime created;
                    if ( dateOK )
                      created.setTime_t( mktime(&block.creationTime) );
                    if( dateOK && created.isValid() )
                        htmlStr += i18n( "Message was signed on %1 with unknown key %2." )
                                .arg( KGlobal::locale()->formatDateTime( created ) )
                                .arg( keyWithWithoutURL );
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
          if( ( block->type() == Kpgp::PgpMessageBlock &&
                // ### Workaround for bug 56693
                !kmkernel->contextMenuShown() ) ||
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

                      if( !keyId.isEmpty() ) {
                        keyTrust = pgp->keyTrust( keyId );
                        Kpgp::Key* key = pgp->publicKey( keyId );
                        if ( key ) {
                          // Use the user ID from the key because this one
                          // is charset safe.
                          signer = key->primaryUserID();
                        }
                      }
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

#ifdef MARCS_DEBUG
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
