/*
    This file is part of KMail.

    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 - 2004 Bo Thorsen <bo@sonofthor.dk>
    Copyright (c) 2004 Till Adam <adam@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmailicalifaceimpl.h"
#include "kmfoldertree.h"
#include "kmfolderdir.h"
#include "kmgroupware.h"
#include "kmfoldermgr.h"
#include "kmcommands.h"
#include "kmfolderindex.h"
#include "kmmsgdict.h"
#include "kmmsgpart.h"
#include "kmfolderimap.h"
#include "globalsettings.h"
#include "kmacctmgr.h"

#include <mimelib/enum.h>
#include <mimelib/utility.h>
#include <mimelib/body.h>
#include <mimelib/mimepp.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <dcopclient.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kurl.h>
#include <qmap.h>
#include <ktempfile.h>
#include <qfile.h>
#include <qdom.h>
#include "kmfoldercachedimap.h"

// Local helper methods
static void vPartMicroParser( const QString& str, QString& s );
static void reloadFolderTree();

// The index in this array is the KMail::FolderContentsType enum
static const struct {
  const char* contentsTypeStr; // the string used in the DCOP interface
  const char* mimetype;
  KFolderTreeItem::Type treeItemType;
  const char* annotation;
} s_folderContentsType[] = {
  { "Mail", "application/x-vnd.kolab.mail", KFolderTreeItem::Other, "mail"},
  { "Calendar", "application/x-vnd.kolab.event", KFolderTreeItem::Calendar, "event" },
  { "Contact", "application/x-vnd.kolab.contact", KFolderTreeItem::Contacts, "contact" },
  { "Note", "application/x-vnd.kolab.note", KFolderTreeItem::Notes, "note" },
  { "Task", "application/x-vnd.kolab.task", KFolderTreeItem::Tasks, "task" },
  { "Journal", "application/x-vnd.kolab.journal", KFolderTreeItem::Journals, "journal" }
};

static QString folderContentsType( KMail::FolderContentsType type )
{
  return s_folderContentsType[type].contentsTypeStr;
}

static QString folderKolabMimeType( KMail::FolderContentsType type )
{
  return s_folderContentsType[type].mimetype;
}

static KMail::FolderContentsType folderContentsType( const QString& type )
{
  for ( uint i = 0 ; i < sizeof s_folderContentsType / sizeof *s_folderContentsType; ++i )
    if ( type == s_folderContentsType[i].contentsTypeStr )
      return static_cast<KMail::FolderContentsType>( i );
  return KMail::ContentsTypeMail;
}

const char* KMailICalIfaceImpl::annotationForContentsType( KMail::FolderContentsType type )
{
  return s_folderContentsType[type].annotation;
}

/*
  This interface has three parts to it - libkcal interface;
  kmail interface; and helper functions.

  The libkcal interface and the kmail interface have the same three
  methods: add, delete and refresh. The only difference is that the
  libkcal interface is used from the IMAP resource in libkcal and
  the kmail interface is used from the groupware object in kmail.
*/

KMailICalIfaceImpl::KMailICalIfaceImpl()
  : DCOPObject( "KMailICalIface" ), QObject( 0, "KMailICalIfaceImpl" ),
    mContacts( 0 ), mCalendar( 0 ), mNotes( 0 ), mTasks( 0 ), mJournals( 0 ),
    mFolderLanguage( 0 ), mUseResourceIMAP( false ), mResourceQuiet( false ),
    mHideFolders( true )
{
  // Listen to config changes
  connect( kmkernel, SIGNAL( configChanged() ), this, SLOT( readConfig() ) );
  connect( kmkernel, SIGNAL( folderRemoved( KMFolder* ) ),
           this, SLOT( slotFolderRemoved( KMFolder* ) ) );

  mExtraFolders.setAutoDelete( true );
  mAccumulators.setAutoDelete( true );
}


/* libkcal part of the interface, called from the resources using this
 * when incidences are added or deleted */

// Receive an iCal or vCard from the resource
bool KMailICalIfaceImpl::addIncidence( const QString& type,
                                       const QString& folder,
                                       const QString& uid,
                                       const QString& ical )
{
  kdDebug(5006) << "KMailICalIfaceImpl::addIncidence( " << type << ", "
                << uid << ", " << ical << " )" << endl;

  if( !mUseResourceIMAP )
    return false;

  bool rc = false;

  if ( !mInTransit.contains( uid ) ) {
    mInTransit.insert( uid, true );

  }

  // Find the folder
  KMFolder* f = folderFromType( type, folder );
  if( f ) {
    // Make a new message for the incidence
    KMMessage* msg = new KMMessage();
    msg->initHeader();
    msg->setType( DwMime::kTypeText );
    if( f == mContacts ) {
      msg->setSubtype( DwMime::kSubtypeXVCard );
      msg->setHeaderField( "Content-Type", "Text/X-VCard; charset=\"utf-8\"" );
      msg->setSubject( "vCard " + uid );
    } else {
      msg->setSubtype( DwMime::kSubtypeVCal );
      msg->setHeaderField("Content-Type",
                          "text/calendar; method=REQUEST; charset=\"utf-8\"");
      msg->setSubject( "iCal " + uid );
    }
    msg->setBodyEncoded( ical.utf8() );

    // Mark the message as read and store it in the folder
    msg->touch();
    f->addMsg( msg );
    rc = true;
  } else
    kdError(5006) << "Not an IMAP resource folder" << endl;

  addFolderChange( f, Contents );

  return rc;
}

// Helper function to find an attachment of a given mimetype
// Can't use KMMessage::findDwBodyPart since it only works with known mimetypes.
static DwBodyPart* findBodyPartByMimeType( const KMMessage& msg, const char* sType, const char* sSubtype )
{
  // quickly searching for our message part: since Kolab parts are
  // top-level parts we do *not* have to travel into embedded multiparts
  DwBodyPart* part = msg.getFirstDwBodyPart();
  while( part ){
    //  kdDebug() << part->Headers().ContentType().TypeStr().c_str() << " "
    //          << part->Headers().ContentType().SubtypeStr().c_str() << endl;
    if ( part->hasHeaders() ) {
      DwMediaType& contentType = part->Headers().ContentType();
      if ( contentType.TypeStr() == sType
           && contentType.SubtypeStr() == sSubtype )
        return part;
    }
    part = part->Next();
  }
  return 0;
}

// Helper function to find an attachment with a given filename
static DwBodyPart* findBodyPart( const KMMessage& msg, const QString& attachmentName )
{
  // quickly searching for our message part: since Kolab parts are
  // top-level parts we do *not* have to travel into embedded multiparts
  for ( DwBodyPart* part = msg.getFirstDwBodyPart(); part; part = part->Next() ) {
    //kdDebug(5006) << "findBodyPart:  - " << part->Headers().ContentDisposition().Filename().c_str() << endl;
    if ( part->hasHeaders()
         && attachmentName == part->Headers().ContentDisposition().Filename().c_str() )
      return part;
  }
  return 0;
}

#if 0
static void debugBodyParts( const char* foo, const KMMessage& msg )
{
  kdDebug(5006) << "--debugBodyParts " << foo << "--" << endl;
  for ( DwBodyPart* part = msg.getFirstDwBodyPart(); part; part = part->Next() ) {
    if ( part->hasHeaders() ) {
      kdDebug(5006) << " bodypart: " << part << endl;
      kdDebug(5006) << "        " << part->Headers().AsString().c_str() << endl;
    }
    else
      kdDebug(5006) << " part " << part << " has no headers" << endl;
  }
}
#else
inline static void debugBodyParts( const char*, const KMMessage& ) {}
#endif


// Add (or overwrite, resp.) an attachment in an existing mail,
// attachments must be local files, they are identified by their names.
// If lookupByName if false the attachment to replace is looked up by mimetype.
// return value: wrong if attachment could not be added/updated
bool KMailICalIfaceImpl::updateAttachment( KMMessage& msg,
                                           const QString& attachmentURL,
                                           const QString& attachmentName,
                                           const QString& attachmentMimetype,
                                           bool lookupByName )
{
  kdDebug(5006) << "KMailICalIfaceImpl::updateAttachment( " << attachmentURL << " )" << endl;

  bool bOK = false;

  KURL url( attachmentURL );
  if ( url.isValid() && url.isLocalFile() ) {
    const QString fileName( url.path() );
    QFile file( fileName );
    if( file.open( IO_ReadOnly ) ) {
      QByteArray rawData = file.readAll();
      file.close();

      // create the new message part with data read from temp file
      KMMessagePart msgPart;
      msgPart.setName( attachmentName );

      const int iSlash = attachmentMimetype.find('/');
      const QCString sType    = attachmentMimetype.left( iSlash   ).latin1();
      const QCString sSubtype = attachmentMimetype.mid(  iSlash+1 ).latin1();
      msgPart.setTypeStr( sType );
      msgPart.setSubtypeStr( sSubtype );
      QCString ctd("attachment;\n  filename=\"");
      ctd.append( attachmentName.latin1() );
      ctd.append("\"");
      msgPart.setContentDisposition( ctd );
      QValueList<int> dummy;
      msgPart.setBodyAndGuessCte( rawData, dummy );
      msgPart.setPartSpecifier( fileName );

      DwBodyPart* newPart = msg.createDWBodyPart( &msgPart );
      // This whole method is a bit special. We mix code for writing and code for reading.
      // E.g. we need to parse the content-disposition again for ContentDisposition().Filename()
      // to work later on.
      newPart->Headers().ContentDisposition().Parse();

      DwBodyPart* part = lookupByName ? findBodyPart( msg, attachmentName )
                         : findBodyPartByMimeType( msg, sType, sSubtype );
      if ( part ) {
        // Make sure the replacing body part is pointing
        // to the same next part as the original body part.
        newPart->SetNext( part->Next() );
        // call DwBodyPart::operator =
        // which calls DwEntity::operator =
        *part = *newPart;
        delete newPart;
        msg.setNeedsAssembly();
        kdDebug(5006) << "Attachment " << attachmentName << " updated." << endl;
      } else {
        msg.addDwBodyPart( newPart );
        kdDebug(5006) << "Attachment " << attachmentName << " added." << endl;
      }
      bOK = true;
    }else{
      kdDebug(5006) << "Attachment " << attachmentURL << " can not be read." << endl;
    }
  }else{
    kdDebug(5006) << "Attachment " << attachmentURL << " not a local file." << endl;
  }

  return bOK;
}

// Look for the attachment with the right mimetype
bool KMailICalIfaceImpl::kolabXMLFoundAndDecoded( const KMMessage& msg, const QString& mimetype, QString& s )
{
  const int iSlash = mimetype.find('/');
  const QCString sType    = mimetype.left( iSlash   ).latin1();
  const QCString sSubtype = mimetype.mid(  iSlash+1 ).latin1();
  DwBodyPart* part = findBodyPartByMimeType( msg, sType, sSubtype );
  if ( part ) {
    KMMessagePart msgPart;
    KMMessage::bodyPart(part, &msgPart);
    s = msgPart.bodyToUnicode();
    return true;
  }
  return false;
}

// Delete an attachment in an existing mail.
// return value: wrong if attachment could not be deleted
//
// This code could be optimized: for now we just replace
// the attachment by an empty dummy attachment since Mimelib
// does not provide an option for deleting attachments yet.
bool KMailICalIfaceImpl::deleteAttachment( KMMessage& msg,
                                           const QString& attachmentName )
{
  kdDebug(5006) << "KMailICalIfaceImpl::deleteAttachment( " << attachmentName << " )" << endl;

  bool bOK = false;

  // quickly searching for our message part: since Kolab parts are
  // top-level parts we do *not* have to travel into embedded multiparts
  DwBodyPart* part = findBodyPart( msg, attachmentName );
  if ( part ) {
    msg.getTopLevelPart()->Body().RemoveBodyPart( part );
    delete part;
    msg.setNeedsAssembly();
    kdDebug(5006) << "Attachment deleted." << endl;
    bOK = true;
  }

  if( !bOK ){
    kdDebug(5006) << "Attachment " << attachmentName << " not found." << endl;
  }

  return bOK;
}


// Store a new entry that was received from the resource
Q_UINT32 KMailICalIfaceImpl::addIncidenceKolab( KMFolder& folder,
                                                const QString& subject,
                                                const QString& plainTextBody,
                                                const QMap<QCString, QString>& customHeaders,
                                                const QStringList& attachmentURLs,
                                                const QStringList& attachmentNames,
                                                const QStringList& attachmentMimetypes )
{
  kdDebug(5006) << "KMailICalIfaceImpl::addIncidenceKolab( " << attachmentNames << " )" << endl;

  Q_UINT32 sernum = 0;
  bool bAttachOK = true;

  // Make a new message for the incidence
  KMMessage* msg = new KMMessage();
  msg->initHeader();
  msg->setType( DwMime::kTypeMultipart );
  msg->setSubtype( DwMime::kSubtypeMixed );
  msg->headers().ContentType().CreateBoundary( 0 );
  msg->headers().ContentType().Assemble();
  msg->setSubject( subject );
  msg->setAutomaticFields( true );

  QMap<QCString, QString>::ConstIterator ith = customHeaders.begin();
  const QMap<QCString, QString>::ConstIterator ithEnd = customHeaders.end();
  for ( ; ith != ithEnd ; ++ith ) {
    msg->setHeaderField( ith.key(), ith.data() );
  }

  // add a first body part to be displayed by all mailer
  // than can NOT display Kolab data: no matter if these
  // mailers are MIME compliant or not
  KMMessagePart firstPart;
  firstPart.setType(    DwMime::kTypeText     );
  firstPart.setSubtype( DwMime::kSubtypePlain );
  firstPart.setBodyFromUnicode( plainTextBody );
  msg->addBodyPart( &firstPart );

  Q_ASSERT( attachmentMimetypes.count() == attachmentURLs.count() );
  Q_ASSERT( attachmentNames.count() == attachmentURLs.count() );
  // Add all attachments by reading them from their temp. files
  QStringList::ConstIterator itmime = attachmentMimetypes.begin();
  QStringList::ConstIterator iturl = attachmentURLs.begin();
  for( QStringList::ConstIterator itname = attachmentNames.begin();
       itname != attachmentNames.end()
       && itmime != attachmentMimetypes.end()
       && iturl != attachmentURLs.end();
       ++itname, ++iturl, ++itmime ){
    bool bymimetype = (*itmime).startsWith( "application/x-vnd.kolab." );
    if( !updateAttachment( *msg, *iturl, *itname, *itmime, !bymimetype ) ){
      kdWarning(5006) << "Attachment error, can not add Incidence." << endl;
      bAttachOK = false;
      break;
    }
  }

  if( bAttachOK ){
    // Mark the message as read and store it in the folder
    msg->cleanupHeader();
    //debugBodyParts( "after cleanup", *msg );
    msg->touch();
    if ( folder.addMsg( msg ) == 0 )
      // Message stored
      sernum = msg->getMsgSerNum();
    kdDebug(5006) << "addIncidenceKolab(): Message done and saved. Sernum: "
                  << sernum << endl;

    //debugBodyParts( "after addMsg", *msg );

  } else
    kdError(5006) << "addIncidenceKolab(): Message *NOT* saved!\n";

  return sernum;
}

// The resource orders a deletion
bool KMailICalIfaceImpl::deleteIncidence( const QString& type,
                                          const QString& folder,
                                          const QString& uid )
{
  if( !mUseResourceIMAP )
    return false;

  kdDebug(5006) << "KMailICalIfaceImpl::deleteIncidence( " << type << ", "
            << uid << " )" << endl;

  // Find the folder
  KMFolder* f = folderFromType( type, folder );

  if( !f ) {
    kdError(5006) << "deleteIncidence(" << type << "," << folder << ") : Not an IMAP resource folder" << endl;
    return false;
  }
  if ( storageFormat( f ) != StorageIcalVcard ) {
    kdError(5006) << "deleteIncidence(" << type << "," << folder << ") : Folder has wrong storage format " << storageFormat( f ) << endl;
    return false;
  }

  bool rc = false;

  KMMessage* msg = findMessageByUID( uid, f );
  if( msg ) {
    // Message found - delete it and return happy
    deleteMsg( msg );
    rc = true;
    mUIDToSerNum.remove( uid );
  } else
    kdDebug(5006) << type << " not found, cannot remove uid " << uid << endl;
  return rc;
}


bool KMailICalIfaceImpl::deleteIncidenceKolab( const QString& resource,
                                               Q_UINT32 sernum )
{
  // Find the message from the serial number and delete it.
  if( !mUseResourceIMAP )
    return false;

  kdDebug(5006) << "KMailICalIfaceImpl::deleteIncidenceKolab( "
                << resource << ", " << sernum << ")\n";

  // Find the folder
  KMFolder* f = findResourceFolder( resource );
  if( !f ) {
    kdError(5006) << "deleteIncidenceKolab(" << resource << ") : Not an IMAP resource folder" << endl;
    return false;
  }
  if ( storageFormat( f ) != StorageXML ) {
    kdError(5006) << "deleteIncidenceKolab(" << resource << ") : Folder has wrong storage format " << storageFormat( f ) << endl;
    return false;
  }

  bool rc = false;

  KMMessage* msg = findMessageBySerNum( sernum, f );
  if( msg ) {
    // Message found - delete it and return happy
    deleteMsg( msg );
    rc = true;
  } else {
    kdDebug(5006) << "Message not found, cannot remove serNum " << sernum << endl;
  }
  return rc;
}

// The resource asks for a full list of incidences
QStringList KMailICalIfaceImpl::incidences( const QString& type,
                                            const QString& folder )
{
  if( !mUseResourceIMAP )
    return QStringList();

  kdDebug(5006) << "KMailICalIfaceImpl::incidences( " << type << ", "
                << folder << " )" << endl;
  QStringList ilist;

  KMFolder* f = folderFromType( type, folder );
  if ( f ) {
    f->open();
    QString s;
    for( int i=0; i<f->count(); ++i ) {
      bool unget = !f->isMessage(i);
      KMMessage *msg = f->getMsg( i );
      Q_ASSERT( msg );
      if( msg->isComplete() ) {
        if( vPartFoundAndDecoded( msg, s ) ) {
          QString uid( "UID" );
          vPartMicroParser( s, uid );
          const Q_UINT32 sernum = msg->getMsgSerNum();
          kdDebug(5006) << "Insert uid: " << uid << endl;
          mUIDToSerNum.insert( uid, sernum );
          ilist << s;
        }
        if( unget ) f->unGetMsg(i);
      } else {
        // message needs to be gotten first, once it arrives, we'll
        // accumulate it and add it to the resource
        if ( !mAccumulators[ folder ] )
          mAccumulators.insert( folder, new Accumulator( type, folder, f->count() ));
        if ( unget ) mTheUnGetMes.insert( msg->getMsgSerNum(), true );
        FolderJob *job = msg->parent()->createJob( msg );
        connect( job, SIGNAL( messageRetrieved( KMMessage* ) ),
                 this, SLOT( slotMessageRetrieved( KMMessage* ) ) );
        job->start();
      }
    }
  }
  return ilist;
}

int KMailICalIfaceImpl::incidencesKolabCount( const QString& mimetype,
                                              const QString& resource )
{
  if( !mUseResourceIMAP )
    return 0;

  KMFolder* f = findResourceFolder( resource );
  if( !f ) {
    kdError(5006) << "incidencesKolab(" << resource << ") : Not an IMAP resource folder" << endl;
    return 0;
  }
  if ( storageFormat( f ) != StorageXML ) {
    kdError(5006) << "incidencesKolab(" << resource << ") : Folder has wrong storage format " << storageFormat( f ) << endl;
    return 0;
  }

  f->open();
  int n = f->count();
  f->close();
  kdDebug(5006) << "KMailICalIfaceImpl::incidencesKolabCount( " << mimetype << ", "
                << resource << " ) returned " << n << endl;
  return n;
}

QMap<Q_UINT32, QString> KMailICalIfaceImpl::incidencesKolab( const QString& mimetype,
                                                             const QString& resource,
                                                             int startIndex,
                                                             int nbMessages )
{
  /// Get the mimetype attachments from this folder. Returns a
  /// QMap with serialNumber/attachment pairs.
  /// (serial numbers of the mail are provided for easier later update)

  QMap<Q_UINT32, QString> aMap;
  if( !mUseResourceIMAP )
    return aMap;

  KMFolder* f = findResourceFolder( resource );
  if( !f ) {
    kdError(5006) << "incidencesKolab(" << resource << ") : Not an IMAP resource folder" << endl;
    return aMap;
  }
  if ( storageFormat( f ) != StorageXML ) {
    kdError(5006) << "incidencesKolab(" << resource << ") : Folder has wrong storage format " << storageFormat( f ) << endl;
    return aMap;
  }

  f->open();

  int stopIndex = nbMessages == -1 ? f->count() :
                  QMIN( f->count(), startIndex + nbMessages );
  kdDebug(5006) << "KMailICalIfaceImpl::incidencesKolab( " << mimetype << ", "
                << resource << " ) from " << startIndex << " to " << stopIndex << endl;

  for(int i = startIndex; i < stopIndex; ++i) {
#if 0
    bool unget = !f->isMessage(i);
    KMMessage* msg = f->getMsg( i );
#else // faster
    KMMessage* msg = f->storage()->readTemporaryMsg(i);
#endif
    if ( msg ) {
      const int iSlash = mimetype.find('/');
      const QCString sType    = mimetype.left( iSlash   ).latin1();
      const QCString sSubtype = mimetype.mid(  iSlash+1 ).latin1();
      if ( sType.isEmpty() || sSubtype.isEmpty() ) {
        kdError(5006) << mimetype << " not an type/subtype combination" << endl;
      } else {
        DwBodyPart* dwPart = findBodyPartByMimeType( *msg, sType, sSubtype );
        if ( dwPart ) {
          KMMessagePart msgPart;
          KMMessage::bodyPart(dwPart, &msgPart);
          aMap.insert(msg->getMsgSerNum(), msgPart.bodyToUnicode());
        } else {
          // This is *not* an error: it may be that not all of the messages
          // have a message part that is matching the wanted MIME type
        }
      }
#if 0
      if( unget ) f->unGetMsg(i);
#else
      delete msg;
#endif
    }
  }
  return aMap;
}


/* Called when a message that was downloaded from an online imap folder
 * arrives. Needed when listing incidences on online account folders. */
void KMailICalIfaceImpl::slotMessageRetrieved( KMMessage* msg )
{
  if( !msg ) return;

  KMFolder *parent = msg->parent();
  Q_ASSERT( parent );
  Q_UINT32 sernum = msg->getMsgSerNum();

  // do we have an accumulator for this folder?
  Accumulator *ac = mAccumulators.find( parent->location() );
  if( ac ) {
    QString s;
    if ( !vPartFoundAndDecoded( msg, s ) ) return;
    QString uid( "UID" );
    vPartMicroParser( s, uid );
    const Q_UINT32 sernum = msg->getMsgSerNum();
    mUIDToSerNum.insert( uid, sernum );
    ac->add( s );
    if( ac->isFull() ) {
      /* if this was the last one we were waiting for, tell the resource
       * about the new incidences and clean up. */
      asyncLoadResult( ac->incidences, ac->type, ac->folder );
      mAccumulators.remove( ac->folder ); // autodelete
    }
  } else {
    /* We are not accumulating for this folder, so this one was added
     * by KMail. Do your thang. */
     slotIncidenceAdded( msg->parent(), msg->getMsgSerNum() );
  }

  if ( mTheUnGetMes.contains( sernum ) ) {
    mTheUnGetMes.remove( sernum );
    int i = 0;
    KMFolder* folder = 0;
    kmkernel->msgDict()->getLocation( sernum, &folder, &i );
    folder->unGetMsg( i );
  }
}

/* ical/vcard version of listing all available subresources */
QStringList KMailICalIfaceImpl::subresources( const QString& type )
{
  QStringList lst;

  // Add the default one
  KMFolder* f = folderFromType( type, QString::null );
  if ( f && storageFormat( f ) == StorageIcalVcard )
    lst << f->location();

  // Add the extra folders
  KMail::FolderContentsType t = folderContentsType( type );
  QDictIterator<ExtraFolder> it( mExtraFolders );
  for ( ; it.current(); ++it ) {
    f = it.current()->folder;
    if ( f && f->storage()->contentsType() == t
         && storageFormat( f ) == StorageIcalVcard )
      lst << f->location();
  }

  return lst;
}

/* kolab/xml version of listing all available subresources */
QValueList<KMailICalIfaceImpl::SubResource> KMailICalIfaceImpl::subresourcesKolab( const QString& contentsType )
{
  QValueList<SubResource> subResources;

  // Add the default one
  KMFolder* f = folderFromType( contentsType, QString::null );
  if ( f && storageFormat( f ) == StorageXML ) {
    subResources.append( SubResource( f->location(),  f->prettyURL(), !f->isReadOnly() ) );
    kdDebug(5006) << "Adding(1) folder " << f->location() << "    " <<
      ( f->isReadOnly() ? "readonly" : "" ) << endl;
  }

  // get the extra ones
  const KMail::FolderContentsType t = folderContentsType( contentsType );
  QDictIterator<ExtraFolder> it( mExtraFolders );
  for ( ; it.current(); ++it ){
    f = it.current()->folder;
    if ( f && f->storage()->contentsType() == t
         && storageFormat( f ) == StorageXML ) {
      subResources.append( SubResource( f->location(), f->prettyURL(), !f->isReadOnly() ) );
      kdDebug(5006) << "Adding(2) folder " << f->location() << "     " <<
              ( f->isReadOnly() ? "readonly" : "" ) << endl;
    }
  }

  if ( subResources.isEmpty() )
    kdDebug(5006) << "subresourcesKolab: No folder found for " << contentsType << endl;
  return subResources;
}

/* Used by the resource to query whether folders are writable. */
bool KMailICalIfaceImpl::isWritableFolder( const QString& type,
                                           const QString& resource )
{
  KMFolder* f = folderFromType( type, resource );
  if ( !f )
    // Definitely not writable
    return false;

  return !f->isReadOnly();
}

/* update a list of uid/content pairs of a given type in a given folder. */
bool KMailICalIfaceImpl::update( const QString& type, const QString& folder,
                                 const QStringList& entries )
{
  if( !mUseResourceIMAP )
    return false;

  if( entries.count() % 2 == 1 )
    // Something is wrong - an odd amount of strings should not happen
    return false;

  QStringList::ConstIterator it = entries.begin();
  while( true ) {
    // Read them in pairs and call the single update method
    QString uid, entry;
    if( it == entries.end() )
      break;
    uid = *it;
    ++it;
    if( it == entries.end() )
      break;
    entry = *it;
    ++it;

    if( !update( type, folder, uid, entry ) )
      // Some error happened
      return false;
  }

  return true;
}

/* ical/vcard version */
bool KMailICalIfaceImpl::update( const QString& type, const QString& folder,
                                 const QString& uid, const QString& entry )
{
  if( !mUseResourceIMAP )
    return false;

  kdDebug(5006) << "Update( " << type << ", " << folder << ", " << uid << ")\n";
  bool rc = true;

  if ( !mInTransit.contains( uid ) ) {
    mInTransit.insert( uid, true );
  } else {
    // this is reentrant, if a new update comes in, we'll just
    // replace older ones
    mPendingUpdates.insert( uid, entry );
    return rc;
  }

  // Find the folder and the incidence in it
  KMFolder* f = folderFromType( type, folder );
  if( f ) {
    KMMessage* msg = findMessageByUID( uid, f );
    if( msg ) {
      // Message found - update it
      deleteMsg( msg );
      mUIDToSerNum.remove( uid );
    } else {
      kdDebug(5006) << type << " not found, cannot update uid " << uid << endl;
    }
    addIncidence( type, folder, uid, entry );
  } else {
    kdError(5006) << "Not an IMAP resource folder" << endl;
    rc = false;
  }
  return rc;
}

/* kolab version */
Q_UINT32 KMailICalIfaceImpl::update( const QString& resource,
                                     Q_UINT32 sernum,
                                     const QString& subject,
                                     const QString& plainTextBody,
                                     const QMap<QCString, QString>& customHeaders,
                                     const QStringList& attachmentURLs,
                                     const QStringList& attachmentMimetypes,
                                     const QStringList& attachmentNames,
                                     const QStringList& deletedAttachments )
{
  Q_UINT32 rc = 0;

  // This finds the message with serial number "sernum", sets the
  // xml attachments to hold the contents of "xml", and updates all
  // attachments.
  // The mail can have additional attachments, and these are not
  // touched!  They belong to other clients - like Outlook
  // So we delete all the attachments listed in the
  // "deletedAttachments" arg, and then update/add all the attachments
  // given by the urllist attachments.

  // If the mail does not already exist, id will not be a valid serial
  // number, and the mail is just added instead. In this case
  // the deletedAttachments can be forgotten.
  if( !mUseResourceIMAP )
    return rc;

  Q_ASSERT( !resource.isEmpty() );

  kdDebug(5006) << "KMailICalIfaceImpl::update( " << resource << ", " << sernum << " )\n";
  kdDebug(5006) << attachmentURLs << "\n";
  kdDebug(5006) << attachmentMimetypes << "\n";
  kdDebug(5006) << attachmentNames << "\n";
  kdDebug(5006) << "deleted attachments:" << deletedAttachments << "\n";

  // Find the folder
  KMFolder* f = findResourceFolder( resource );
  if( !f ) {
    kdError(5006) << "update(" << resource << ") : Not an IMAP resource folder" << endl;
    return rc;
  }
  if ( storageFormat( f ) != StorageXML ) {
    kdError(5006) << "update(" << resource << ") : Folder has wrong storage format " << storageFormat( f ) << endl;
    return rc;
  }

  f->open();

  kdDebug(5006) << "Updating in folder " << f << " " << f->location() << endl;
  KMMessage* msg = 0;
  if ( sernum != 0 )
    msg = findMessageBySerNum( sernum, f );
  if ( msg ) {
    // Message found - make a copy and update it:
    KMMessage* newMsg = new KMMessage( *msg );
    newMsg->setSubject( subject );
    QMap<QCString, QString>::ConstIterator ith = customHeaders.begin();
    const QMap<QCString, QString>::ConstIterator ithEnd = customHeaders.begin();
    for ( ; ith != ithEnd ; ++ith )
      newMsg->setHeaderField( ith.key(), ith.data() );
    newMsg->setParent( 0 ); // workaround strange line in KMMsgBase::assign. newMsg is not in any folder yet.
    // Note that plainTextBody isn't used in this branch. We assume it's still valid from when the mail was created.

    // Delete some attachments according to list
    for( QStringList::ConstIterator it = deletedAttachments.begin();
         it != deletedAttachments.end();
         ++it ){
      if( !deleteAttachment( *newMsg, *it ) ){
        // Note: It is _not_ an error if an attachment was already deleted.
      }
    }

    // Add all attachments by reading them from their temp. files
    QStringList::ConstIterator iturl = attachmentURLs.begin();
    QStringList::ConstIterator itmime = attachmentMimetypes.begin();
    QStringList::ConstIterator itname = attachmentNames.begin();
    for( ;
         iturl != attachmentURLs.end()
         && itmime != attachmentMimetypes.end()
         && itname != attachmentNames.end();
         ++iturl, ++itname, ++itmime ){
      bool bymimetype = (*itname).startsWith( "application/x-vnd.kolab." );
      if( !updateAttachment( *newMsg, *iturl, *itname, *itmime, bymimetype ) ){
        kdDebug(5006) << "Attachment error, can not update attachment " << *iturl << endl;
        break;
      }
    }

    //debugBodyParts( "in update, before cleanup", *newMsg );

    // This is necessary for the headers to be readable later on
    newMsg->cleanupHeader();

    //debugBodyParts( "in update, after cleanup", *newMsg );

    deleteMsg( msg );
    if ( f->addMsg( newMsg ) == 0 ) {
      // Message stored
      rc = newMsg->getMsgSerNum();
      kdDebug(5006) << "forget about " << sernum << ", it's " << rc << " now" << endl;
    }

  } else {
    // Message not found - store it newly
    rc = addIncidenceKolab( *f, subject, plainTextBody, customHeaders,
                            attachmentURLs,
                            attachmentNames,
                            attachmentMimetypes );
  }

  f->close();
  addFolderChange( f, Contents );
  return rc;
}

KURL KMailICalIfaceImpl::getAttachment( const QString& resource,
                                        Q_UINT32 sernum,
                                        const QString& filename )
{
  // This finds the attachment with the filename, saves it to a
  // temp file and returns a URL to it. It's up to the resource
  // to delete the tmp file later.
  if( !mUseResourceIMAP )
    return KURL();

  kdDebug(5006) << "KMailICalIfaceImpl::getAttachment( "
                << resource << ", " << sernum << ", " << filename << " )\n";

  // Find the folder
  KMFolder* f = findResourceFolder( resource );
  if( !f ) {
    kdError(5006) << "getAttachment(" << resource << ") : Not an IMAP resource folder" << endl;
    return KURL();
  }
  if ( storageFormat( f ) != StorageXML ) {
    kdError(5006) << "getAttachment(" << resource << ") : Folder has wrong storage format " << storageFormat( f ) << endl;
    return KURL();
  }

  KURL url;

  bool bOK = false;
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;

  KMMessage* msg = findMessageBySerNum( sernum, f );
  if( msg ) {
    // Message found - look for the attachment:

    DwBodyPart* part = findBodyPart( *msg, filename );
    if ( part ) {
      // Save the contents of the attachment.
      KMMessagePart aPart;
      msg->bodyPart( part, &aPart );
      QByteArray rawData( aPart.bodyDecodedBinary() );

      KTempFile file;
      file.file()->writeBlock( rawData.data(), rawData.size() );

      url.setPath( file.name() );

      bOK = true;
    }

    if( !bOK ){
      kdDebug(5006) << "Attachment " << filename << " not found." << endl;
    }
  }else{
    kdDebug(5006) << "Message not found." << endl;
  }

  mResourceQuiet = quiet;
  return url;
}

// ============================================================================

/* KMail part of the interface. These slots are connected to the resource
 * folders and inform us of folders or incidences in them changing, being 
 * added or going away. */

void KMailICalIfaceImpl::slotFolderRemoved( KMFolder* folder )
{
  // pretend the folder just changed back to the mail type, which
  // does the right thing, namely remove resource
  folderContentsTypeChanged( folder, KMail::ContentsTypeMail );
}

// KMail added a file to one of the groupware folders
void KMailICalIfaceImpl::slotIncidenceAdded( KMFolder* folder,
                                             Q_UINT32 sernum )
{
  if( !mUseResourceIMAP )
    return;

  QString type = folderContentsType( folder->storage()->contentsType() );
  if( !type.isEmpty() ) {
    // Get the index of the mail
    int i = 0;
    KMFolder* aFolder = 0;
    kmkernel->msgDict()->getLocation( sernum, &aFolder, &i );
    assert( folder == aFolder );

    bool unget = !folder->isMessage( i );
    QString s;
    QString uid( "UID" );
    KMMessage *msg = folder->getMsg( i );
    if( !msg ) return;
    if( msg->isComplete() ) {

      bool ok = false;
      StorageFormat format = storageFormat( folder );
      switch( format ) {
        case StorageIcalVcard:
          // Read the iCal or vCard
          ok = vPartFoundAndDecoded( msg, s );
          if ( ok )
            vPartMicroParser( s, uid );
          break;
        case StorageXML:
          // Read the XML from the attachment with the given mimetype
          ok = kolabXMLFoundAndDecoded( *msg, folderKolabMimeType( folder->storage()->contentsType() ), s );
          if ( ok ) {
            QDomDocument doc;
            if ( doc.setContent( s ) ) {
              QDomElement top = doc.documentElement();
              for ( QDomNode n = top.firstChild(); !n.isNull(); n = n.nextSibling() ) {
                QDomElement e = n.toElement();
                if ( e.tagName() == "uid" ) {
                  uid = e.text();
                }
              }
            }
          }
          break;
      }
      if ( !ok ) {
        if ( unget )
          folder->unGetMsg( i );
        return;
      }
      const Q_UINT32 sernum = msg->getMsgSerNum();
      kdDebug(5006) << "Insert uid: " << uid << endl;
      mUIDToSerNum.insert( uid, sernum );
      // tell the resource if we didn't trigger this ourselves
      if( !mInTransit.contains( uid ) ) {
        incidenceAdded( type, folder->location(), sernum, format, s );
      }
      else
        mInTransit.remove( uid );
      
      // this one is for the imap resource FIXME
      incidenceAdded( type, folder->location(), s );

      // Check if new updates have since arrived, if so, trigger them
      if ( mPendingUpdates.contains( uid ) ) {
        kdDebug(5006) << "KMailICalIfaceImpl::slotIncidenceAdded - Pending Update" << endl;
        QString entry = mPendingUpdates[ uid ];
        mPendingUpdates.remove( uid );
        update( type, folder->location(), uid, entry );
      }
    } else {
      // go get the rest of it, then try again
      if ( unget ) mTheUnGetMes.insert( msg->getMsgSerNum(), true );
      FolderJob *job = msg->parent()->createJob( msg );
      connect( job, SIGNAL( messageRetrieved( KMMessage* ) ),
               this, SLOT( slotMessageRetrieved( KMMessage* ) ) );
      job->start();
      return;
    }
    if( unget ) folder->unGetMsg(i);
  } else
    kdError(5006) << "Not an IMAP resource folder" << endl;
}

// KMail deleted a file
void KMailICalIfaceImpl::slotIncidenceDeleted( KMFolder* folder,
                                               Q_UINT32 sernum )
{
  if( mResourceQuiet || !mUseResourceIMAP )
    return;

  QString type = folderContentsType( folder->storage()->contentsType() );
  kdDebug(5006) << folder << " " << type << " " << sernum << endl;
  if( !type.isEmpty() ) {
    // Get the index of the mail
    int i = 0;
    KMFolder* aFolder = 0;
    kmkernel->msgDict()->getLocation( sernum, &aFolder, &i );
    assert( folder == aFolder );

    // Read the iCal or vCard
    bool unget = !folder->isMessage( i );
    QString s;
    bool ok = false;
    KMMessage* msg = folder->getMsg( i );
    QString uid( "UID" );
    switch( storageFormat( folder ) ) {
    case StorageIcalVcard:
        if( vPartFoundAndDecoded( msg, s ) ) {
            vPartMicroParser( s, uid );
            ok = true;
        }
        break;
    case StorageXML:
        if ( kolabXMLFoundAndDecoded( *msg, folderKolabMimeType( folder->storage()->contentsType() ), s ) ) {
            QDomDocument doc;
            if ( doc.setContent( s ) ) {
                QDomElement top = doc.documentElement();
                for ( QDomNode n = top.firstChild(); !n.isNull(); n = n.nextSibling() ) {
                    QDomElement e = n.toElement();
                    if ( e.tagName() == "uid" ) {
                        uid = e.text();
                        ok = true;
                        break;
                    }
                }
            }
        }
        break;
    }
    if ( ok ) {
        kdDebug(5006) << "Emitting DCOP signal incidenceDeleted( "
                      << type << ", " << folder->location() << ", " << uid
                      << " )" << endl;
        incidenceDeleted( type, folder->location(), uid );
    }
    if( unget ) folder->unGetMsg(i);
  } else
    kdError(5006) << "Not a groupware folder" << endl;
}

// KMail orders a refresh
void KMailICalIfaceImpl::slotRefresh( const QString& type )
{
  if( mUseResourceIMAP ) {
    signalRefresh( type, QString::null /* PENDING(bo) folder->location() */ );
    kdDebug(5006) << "Emitting DCOP signal signalRefresh( " << type << " )" << endl;
  }
}

// This is among other things called when an expunge of a folder happens
void KMailICalIfaceImpl::slotRefreshFolder( KMFolder* folder)
{
  // TODO: The resources would of course be better off, if only this
  // folder would need refreshing. Currently it just orders a reload of
  // the type of the folder
  if( mUseResourceIMAP && folder ) {
    if( folder == mCalendar || folder == mContacts
        || folder == mNotes || folder == mTasks
        || folder == mJournals || mExtraFolders.find( folder->location() ) ) {
      // Refresh the folder of this type
      KMail::FolderContentsType ct = folder->storage()->contentsType();
      slotRefresh( s_folderContentsType[ct].contentsTypeStr );
    }
  }
}

/****************************
 * The folder and message stuff code
 */

KMFolder* KMailICalIfaceImpl::folderFromType( const QString& type,
                                              const QString& folder )
{
  if( mUseResourceIMAP ) {
    KMFolder* f = 0;
    if ( !folder.isEmpty() ) {
      f = extraFolder( type, folder );
      if ( f )
        return f;
    }

    if( type == "Calendar" ) f = mCalendar;
    else if( type == "Contact" ) f = mContacts;
    else if( type == "Note" ) f = mNotes;
    else if( type == "Task" || type == "Todo" ) f = mTasks;
    else if( type == "Journal" ) f = mJournals;

    if ( f && ( folder.isEmpty() || folder == f->location() ) )
      return f;

    kdError(5006) << "No folder ( " << type << ", " << folder << " )\n";
  }

  return 0;
}


// Returns true if folder is a resource folder. If the resource isn't enabled
// this always returns false
bool KMailICalIfaceImpl::isResourceFolder( KMFolder* folder ) const
{
  return mUseResourceIMAP && folder &&
    ( isStandardResourceFolder( folder ) || mExtraFolders.find( folder->location() )!=0 );
}

bool KMailICalIfaceImpl::isStandardResourceFolder( KMFolder* folder ) const
{
  return ( folder == mCalendar || folder == mTasks || folder == mJournals ||
           folder == mNotes || folder == mContacts );
}

bool KMailICalIfaceImpl::hideResourceFolder( KMFolder* folder ) const
{
  return mHideFolders && isResourceFolder( folder );
}

KFolderTreeItem::Type KMailICalIfaceImpl::folderType( KMFolder* folder ) const
{
  if( mUseResourceIMAP && folder ) {
    if( folder == mCalendar || folder == mContacts
        || folder == mNotes || folder == mTasks
        || folder == mJournals || mExtraFolders.find( folder->location() ) ) {
      KMail::FolderContentsType ct = folder->storage()->contentsType();
      return s_folderContentsType[ct].treeItemType;
    }
  }

  return KFolderTreeItem::Other;
}

// Global tables of foldernames is different languages
// For now: 0->English, 1->German, 2->French, 3->Dutch
static QMap<KFolderTreeItem::Type,QString> folderNames[4];
QString KMailICalIfaceImpl::folderName( KFolderTreeItem::Type type, int language ) const
{
  // With the XML storage, folders are always (internally) named in English
  if ( GlobalSettings::theIMAPResourceStorageFormat() == GlobalSettings::EnumTheIMAPResourceStorageFormat::XML )
    language = 0;

  static bool folderNamesSet = false;
  if( !folderNamesSet ) {
    folderNamesSet = true;
    /* NOTE: If you add something here, you also need to update
       GroupwarePage in configuredialog.cpp */

    // English
    folderNames[0][KFolderTreeItem::Calendar] = QString::fromLatin1("Calendar");
    folderNames[0][KFolderTreeItem::Tasks] = QString::fromLatin1("Tasks");
    folderNames[0][KFolderTreeItem::Journals] = QString::fromLatin1("Journal");
    folderNames[0][KFolderTreeItem::Contacts] = QString::fromLatin1("Contacts");
    folderNames[0][KFolderTreeItem::Notes] = QString::fromLatin1("Notes");

    // German
    folderNames[1][KFolderTreeItem::Calendar] = QString::fromLatin1("Kalender");
    folderNames[1][KFolderTreeItem::Tasks] = QString::fromLatin1("Aufgaben");
    folderNames[1][KFolderTreeItem::Journals] = QString::fromLatin1("Journal");
    folderNames[1][KFolderTreeItem::Contacts] = QString::fromLatin1("Kontakte");
    folderNames[1][KFolderTreeItem::Notes] = QString::fromLatin1("Notizen");

    // French
    folderNames[2][KFolderTreeItem::Calendar] = QString::fromLatin1("Calendrier");
    folderNames[2][KFolderTreeItem::Tasks] = QString::fromLatin1("Tâches");
    folderNames[2][KFolderTreeItem::Journals] = QString::fromLatin1("Journal");
    folderNames[2][KFolderTreeItem::Contacts] = QString::fromLatin1("Contacts");
    folderNames[2][KFolderTreeItem::Notes] = QString::fromLatin1("Notes");

    // Dutch
    folderNames[3][KFolderTreeItem::Calendar] = QString::fromLatin1("Agenda");
    folderNames[3][KFolderTreeItem::Tasks] = QString::fromLatin1("Taken");
    folderNames[3][KFolderTreeItem::Journals] = QString::fromLatin1("Logboek");
    folderNames[3][KFolderTreeItem::Contacts] = QString::fromLatin1("Contactpersonen");
    folderNames[3][KFolderTreeItem::Notes] = QString::fromLatin1("Notities");
  }

  if( language < 0 || language > 3 ) {
    return folderNames[mFolderLanguage][type];
  }
  else {
    return folderNames[language][type];
  }
}


// Find message matching a given UID
KMMessage *KMailICalIfaceImpl::findMessageByUID( const QString& uid, KMFolder* folder )
{
  if( !folder || !mUIDToSerNum.contains( uid ) ) return 0;
  int i;
  KMFolder *aFolder;
  kmkernel->msgDict()->getLocation( mUIDToSerNum[uid], &aFolder, &i );
  Q_ASSERT( aFolder == folder );
  return folder->getMsg( i );
}

// Find message matching a given serial number
KMMessage *KMailICalIfaceImpl::findMessageBySerNum( Q_UINT32 serNum, KMFolder* folder )
{
  if( !folder ) return 0;

  KMMessage *message = 0;
  KMFolder* aFolder = 0;
  int index;
  kmkernel->msgDict()->getLocation( serNum, &aFolder, &index );
  if( aFolder && aFolder != folder ){
    kdWarning(5006) << "findMessageBySerNum( " << serNum << " ) found it in folder " << aFolder->location() << ", expected " << folder->location() << endl;
  }else{
    if( aFolder )
      message = aFolder->getMsg( index );
    if (!message)
      kdWarning(5006) << "findMessageBySerNum( " << serNum << " ) invalid serial number\n" << endl;
  }
  return message;
}

void KMailICalIfaceImpl::deleteMsg( KMMessage *msg )
{
  if( !msg ) return;
  // Commands are now delayed; can't use that anymore, we need immediate deletion
  //( new KMDeleteMsgCommand( msg->parent(), msg ) )->start();
  KMFolder *srcFolder = msg->parent();
  int idx = srcFolder->find(msg);
  assert(idx != -1);
  srcFolder->removeMsg(idx);
  delete msg;
  addFolderChange( srcFolder, Contents );
}

void KMailICalIfaceImpl::folderContentsTypeChanged( KMFolder* folder,
                                                    KMail::FolderContentsType contentsType )
{
  if ( !mUseResourceIMAP )
    return;
  kdDebug(5006) << "folderContentsTypeChanged( " << folder->name()
                << ", " << contentsType << ")\n";

  // The builtins can't change type
  if ( isStandardResourceFolder( folder ) )
    return;

  // Check if already know that 'extra folder'
  const QString location = folder->location();
  ExtraFolder* ef = mExtraFolders.find( location );
  if ( ef && ef->folder ) {
    // Notify that the old folder resource is no longer available
    subresourceDeleted(folderContentsType( folder->storage()->contentsType() ), location );

    if ( contentsType == 0 ) {
      // Delete the old entry, stop listening and stop here
      mExtraFolders.remove( location );
      folder->disconnect( this );
      return;
    }
    // So the type changed to another groupware type, ok.
  } else {
    if ( ef && !ef->folder ) // deleted folder, clean up
      mExtraFolders.remove( location );
    if ( contentsType == 0 )
        return;

    kdDebug(5006) << "registering " << location << " as extra folder" << endl;
    // Make a new entry for the list
    ef = new ExtraFolder( folder );
    mExtraFolders.insert( location, ef );

    StorageFormat format= GlobalSettings::theIMAPResourceStorageFormat() 
      == GlobalSettings::EnumTheIMAPResourceStorageFormat::XML ? StorageXML : StorageIcalVcard;
    FolderInfo info( format, NoChange );
    mFolderInfoMap.insert( folder, info );

    // avoid multiple connections
    disconnect( folder, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
                this, SLOT( slotIncidenceAdded( KMFolder*, Q_UINT32 ) ) );
    disconnect( folder, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
                this, SLOT( slotIncidenceDeleted( KMFolder*, Q_UINT32 ) ) );
    disconnect( folder, SIGNAL( expunged( KMFolder* ) ),
                this, SLOT( slotRefreshFolder( KMFolder* ) ) );
    disconnect( folder->storage(), SIGNAL( readOnlyChanged( KMFolder* ) ),
                this, SLOT( slotFolderPropertiesChanged( KMFolder* ) ) );

    // Listen to changes from the folder
    connect( folder, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
             this, SLOT( slotIncidenceAdded( KMFolder*, Q_UINT32 ) ) );
    connect( folder, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
             this, SLOT( slotIncidenceDeleted( KMFolder*, Q_UINT32 ) ) );
    connect( folder, SIGNAL( expunged( KMFolder* ) ),
             this, SLOT( slotRefreshFolder( KMFolder* ) ) );
    connect( folder->storage(), SIGNAL( readOnlyChanged( KMFolder* ) ),
             this, SLOT( slotFolderPropertiesChanged( KMFolder* ) ) );
  }

  // Tell about the new resource
  /* FIXME merge once we are back in HEAD. IMAP Resource still uses the other one. */
  subresourceAdded( folderContentsType( contentsType ), location, folder->prettyURL() );
  subresourceAdded( folderContentsType( contentsType ), location );
}

KMFolder* KMailICalIfaceImpl::extraFolder( const QString& type,
                                           const QString& folder )
{
  // If an extra folder exists that matches the type and folder location,
  // use that
  int t = folderContentsType( type );
  if ( t < 1 || t > 5 )
    return 0;

  ExtraFolder* ef = mExtraFolders.find( folder );
  if ( ef && ef->folder && ef->folder->storage()->contentsType() == t )
    return ef->folder;

  return 0;
}

KMailICalIfaceImpl::StorageFormat KMailICalIfaceImpl::storageFormat( KMFolder* folder ) const
{
  FolderInfoMap::ConstIterator it = mFolderInfoMap.find( folder );
  if ( it != mFolderInfoMap.end() )
    return (*it).mStorageFormat;
  return GlobalSettings::theIMAPResourceStorageFormat() == GlobalSettings::EnumTheIMAPResourceStorageFormat::XML ? StorageXML : StorageIcalVcard;
}

void KMailICalIfaceImpl::setStorageFormat( KMFolder* folder, StorageFormat format )
{
  FolderInfoMap::Iterator it = mFolderInfoMap.find( folder );
  if ( it != mFolderInfoMap.end() ) {
    (*it).mStorageFormat = format;
  } else {
    FolderInfo info( format, NoChange );
    mFolderInfoMap.insert( folder, info );
  }
  KConfigGroup configGroup( kmkernel->config(), "GroupwareFolderInfo" );
  configGroup.writeEntry( folder->idString() + "-storageFormat",
                          format == StorageXML ? "xml" : "icalvcard" );
}

void KMailICalIfaceImpl::addFolderChange( KMFolder* folder, FolderChanges changes )
{
  FolderInfoMap::Iterator it = mFolderInfoMap.find( folder );
  if ( it != mFolderInfoMap.end() ) {
    (*it).mChanges = static_cast<FolderChanges>( (*it).mChanges | changes );
  } else { // Otherwise, well, it's a folder we don't care about.
    kdDebug(5006) << "addFolderChange: nothing known about folder " << folder->location() << endl;
  }
  KConfigGroup configGroup( kmkernel->config(), "GroupwareFolderInfo" );
  configGroup.writeEntry( folder->idString() + "-changes", (*it).mChanges );
}

void KMailICalIfaceImpl::folderSynced( KMFolder* folder, const KURL& folderURL )
{
  FolderInfoMap::Iterator it = mFolderInfoMap.find( folder );
  if ( it != mFolderInfoMap.end() && (*it).mChanges ) {
    handleFolderSynced( folder, folderURL, (*it).mChanges );
    (*it).mChanges = NoChange;
  }
}

void KMailICalIfaceImpl::handleFolderSynced( KMFolder* folder,
                                             const KURL& folderURL,
                                             int _changes )
{
  // This is done here instead of in the resource, because
  // there could be 0, 1, or N kolab resources at this point.
  // We can hack the N case, but not the 0 case.
  // So the idea of a DCOP signal for this wouldn't work.
  if ( ( _changes & KMailICalIface::Contents ) ||
       ( _changes & KMailICalIface::ACL ) ) {
    if ( storageFormat( folder ) == StorageXML && folder->storage()->contentsType() == KMail::ContentsTypeCalendar )
      triggerKolabFreeBusy( folderURL );
  }
}

void KMailICalIfaceImpl::triggerKolabFreeBusy( const KURL& folderURL )
{
  /* Steffen said: you must issue an authenticated HTTP GET request to
     https://kolabserver/freebusy/trigger/user@domain/Folder/NestedFolder.pfb
     (replace .pfb with .xpfb for extended fb lists). */
  KURL httpURL( folderURL );
  // Keep username ("user@domain"), pass, and host from the imap url
  httpURL.setProtocol( "https" );
  httpURL.setPort( 0 ); // remove imap port

  // IMAP path is either /INBOX/<path> or /user/someone/<path>
  QString path = folderURL.path( -1 );
  Q_ASSERT( path.startsWith( "/" ) );
  int secondSlash = path.find( '/', 1 );
  if ( secondSlash == -1 ) {
    kdWarning() << "KCal::ResourceKolab::fromKMailFolderSynced path is too short: " << path << endl;
    return;
  }
  if ( path.startsWith( "/INBOX/", false ) ) {
    // If INBOX, replace it with the username (which is user@domain)
    path = path.mid( secondSlash );
    path.prepend( folderURL.user() );
  } else {
    // If user, just remove it. So we keep the IMAP-returned username.
    // This assumes it's a known user on the same domain.
    path = path.mid( secondSlash );
  }

  httpURL.setPath( "/freebusy/trigger/" + path + ".pfb" );
  httpURL.setQuery( QString::null );
  kdDebug() << "Triggering PFB update for " << folderURL << " : getting " << httpURL << endl;
  // "Fire and forget". No need for error handling, nor for explicit deletion.
  // Maybe we should try to prevent launching it if it's already running (for this URL) though.
  /*KIO::Job* job =*/ KIO::get( httpURL, false, false /*no progress info*/ );
}

void KMailICalIfaceImpl::slotFolderPropertiesChanged( KMFolder* folder )
{
  if ( isResourceFolder( folder ) ) {
    const QString location = folder->location();
    const QString contentsTypeStr = folderContentsType( folder->storage()->contentsType() );
    subresourceDeleted( contentsTypeStr, location );

    subresourceAdded( contentsTypeStr, location, folder->prettyURL()  /*,
                      !folder->isReadOnly() , folderIsAlarmRelevant( folder ) TODO */ );

    /* FIXME merge once we are back in HEAD. IMAP Resource still uses the other one. */
    subresourceAdded( contentsTypeStr, location );
  }
}

KMFolder* KMailICalIfaceImpl::findResourceFolder( const QString& resource )
{
  // Try the standard folders
  if( mCalendar && mCalendar->location() == resource )
    return mCalendar;
  if ( mContacts && mContacts->location() == resource )
    return mContacts;
  if ( mNotes && mNotes->location() == resource )
    return mNotes;
  if ( mTasks && mTasks->location() == resource )
    return mTasks;
  if ( mJournals && mJournals->location() == resource )
    return mJournals;

  // No luck. Try the extrafolders
  ExtraFolder* ef = mExtraFolders.find( resource );
  if ( ef )
    return ef->folder;

  // No luck at all
  return 0;
}

/****************************
 * The config stuff
 */

void KMailICalIfaceImpl::readConfig()
{
  bool enabled = GlobalSettings::theIMAPResourceEnabled();

  if( !enabled ) {
    if( mUseResourceIMAP == true ) {
      // Shutting down
      mUseResourceIMAP = false;
      cleanup();
      reloadFolderTree();
    }
    return;
  }

  // Read remaining options
  const bool hideFolders = GlobalSettings::hideGroupwareFolders();
  QString parentName = GlobalSettings::theIMAPResourceFolderParent();

  // Find the folder parent
  KMFolderDir* folderParentDir;
  KMFolderType folderType;
  KMFolder* folderParent = kmkernel->findFolderById( parentName );
  if( folderParent == 0 ) {
    // Parent folder not found. It was probably deleted. The user will have to
    // configure things again.
    kdDebug(5006) << "Groupware folder " << parentName << " not found. Groupware functionality disabled" << endl;
    // Or maybe the inbox simply wasn't created on the first startup
    KMAccount* account = kmkernel->acctMgr()->find( GlobalSettings::theIMAPResourceAccount() );
    Q_ASSERT( account );
    if ( account ) {
      // just in case we were connected already
      disconnect( account, SIGNAL( finishedCheck( bool, CheckStatus ) ),
               this, SLOT( slotCheckDone() ) );
      connect( account, SIGNAL( finishedCheck( bool, CheckStatus ) ),
               this, SLOT( slotCheckDone() ) );
    }
    mUseResourceIMAP = false;
    // We can't really call cleanup(), if those folders were completely deleted.
    mCalendar = 0;
    mTasks    = 0;
    mJournals = 0;
    mContacts = 0;
    mNotes    = 0;
    return;
  } else {
    folderParentDir = folderParent->createChildFolder();
    folderType = folderParent->folderType();
  }

  // Make sure the folder parent has the subdirs
  bool makeSubFolders = false;
  KMFolder* folder;
  folder = findStandardResourceFolder( folderParentDir, KMail::ContentsTypeCalendar );
  if( !folder ) {
    kdDebug(5006) << "Calendar folder not found in " << parentName << endl;
    makeSubFolders = true;
    mCalendar = 0;
  }
  folder = findStandardResourceFolder( folderParentDir, KMail::ContentsTypeTask );
  if( !folder ) {
    kdDebug(5006) << "Tasks folder not found in " << parentName << endl;
    makeSubFolders = true;
    mTasks = 0;
  }
  folder = findStandardResourceFolder( folderParentDir, KMail::ContentsTypeJournal );
  if( !folder ) {
    kdDebug(5006) << "Journals folder not found in " << parentName << endl;
    makeSubFolders = true;
    mJournals = 0;
  }
  folder = findStandardResourceFolder( folderParentDir, KMail::ContentsTypeContact );
  if( !folder ) {
    kdDebug(5006) << "Contacts folder not found in " << parentName << endl;
    makeSubFolders = true;
    mContacts = 0;
  }
  folder = findStandardResourceFolder( folderParentDir, KMail::ContentsTypeNote );
  if( !folder ) {
    kdDebug(5006) << "Notes folder not found in " << parentName << endl;
    makeSubFolders = true;
    mNotes = 0;
  }
  if( makeSubFolders ) {
    // Not all subfolders were there, so ask if we can make them
    if( KMessageBox::questionYesNo( 0, i18n("KMail will now create the required folders for the IMAP resource"
                                            " as subfolders of %1; if you do not want this, press \"No\","
                                            " and the IMAP resource will be disabled").arg(folderParent!=0?folderParent->name():folderParentDir->name()),
                                    i18n("IMAP Resource Folders") ) == KMessageBox::No ) {

      GlobalSettings::setTheIMAPResourceEnabled( false );
      mUseResourceIMAP = false;
      mFolderParentDir = 0;
      mFolderParent = 0;
      reloadFolderTree();
      return;
    }
  }

  // Check if something changed
  if( mUseResourceIMAP && !makeSubFolders && mFolderParentDir == folderParentDir
      && mFolderType == folderType ) {
    // Nothing changed
    if ( hideFolders != mHideFolders ) {
      // Well, the folder hiding has changed
      mHideFolders = hideFolders;
      reloadFolderTree();
    }
    return;
  }

  // Make the new settings work
  mUseResourceIMAP = true;
  mFolderLanguage = GlobalSettings::theIMAPResourceFolderLanguage();
  if( mFolderLanguage > 3 ) mFolderLanguage = 0;
  mFolderParentDir = folderParentDir;
  mFolderParent = folderParent;
  mFolderType = folderType;
  mHideFolders = hideFolders;

  // Close the previous folders
  cleanup();

  // Set the new folders
  mCalendar = initFolder( "GCa", KMail::ContentsTypeCalendar );
  mTasks    = initFolder( "GTa", KMail::ContentsTypeTask );
  mJournals = initFolder( "GTa", KMail::ContentsTypeJournal );
  mContacts = initFolder( "GCo", KMail::ContentsTypeContact );
  mNotes    = initFolder( "GNo", KMail::ContentsTypeNote );

  mCalendar->setLabel( i18n( "Calendar" ) );
  mTasks->setLabel( i18n( "Tasks" ) );
  mJournals->setLabel( i18n( "Journal" ) );
  mContacts->setLabel( i18n( "Contacts" ) );
  mNotes->setLabel( i18n( "Notes" ) );

  // Store final annotation (with .default) so that we won't ask again on next startup
  if ( mCalendar->folderType() == KMFolderTypeCachedImap )
    static_cast<KMFolderCachedImap *>( mCalendar->storage() )->updateAnnotationFolderType();
  if ( mTasks->folderType() == KMFolderTypeCachedImap )
    static_cast<KMFolderCachedImap *>( mTasks->storage() )->updateAnnotationFolderType();
  if ( mJournals->folderType() == KMFolderTypeCachedImap )
    static_cast<KMFolderCachedImap *>( mJournals->storage() )->updateAnnotationFolderType();
  if ( mContacts->folderType() == KMFolderTypeCachedImap )
    static_cast<KMFolderCachedImap *>( mContacts->storage() )->updateAnnotationFolderType();
  if ( mNotes->folderType() == KMFolderTypeCachedImap )
    static_cast<KMFolderCachedImap *>( mNotes->storage() )->updateAnnotationFolderType();

  // BEGIN TILL TODO The below only uses the dimap folder manager, which 
  // will fail for all other folder types. Adjust.
 
  kdDebug(5006) << k_funcinfo << "mCalendar=" << mCalendar << " " << mCalendar->location() << endl;
  kdDebug(5006) << k_funcinfo << "mNotes=" << mNotes << " " << mNotes->location() << endl;

  // Find all extra folders
  QStringList folderNames;
  QValueList<QGuardedPtr<KMFolder> > folderList;
  kmkernel->dimapFolderMgr()->createFolderList(&folderNames, &folderList);
  for(QValueList<QGuardedPtr<KMFolder> >::iterator it = folderList.begin();
      it != folderList.end(); ++it)
  {
    FolderStorage* storage = (*it)->storage();
    if ( storage->contentsType() != 0 ) {
      folderContentsTypeChanged( *it, storage->contentsType() );
    }
  }

  // If we just created them, they might have been registered as extra folders temporarily.
  // -> undo that.
  mExtraFolders.remove( mCalendar->location() );
  mExtraFolders.remove( mTasks->location() );
  mExtraFolders.remove( mJournals->location() );
  mExtraFolders.remove( mContacts->location() );
  mExtraFolders.remove( mNotes->location() );

  // END TILL TODO

  subresourceAdded( folderContentsType( KMail::ContentsTypeCalendar ), mCalendar->location(), mCalendar->label() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeCalendar ), mCalendar->location() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeTask ), mTasks->location(), mTasks->label() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeTask ), mTasks->location() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeJournal ), mJournals->location(), mJournals->label() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeJournal ), mJournals->location() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeContact ), mContacts->location(), mContacts->label() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeContact ), mContacts->location() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeNote ), mNotes->location(), mNotes->label() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeNote ), mNotes->location() );

  reloadFolderTree();
}

void KMailICalIfaceImpl::slotCheckDone()
{
  QString parentName = GlobalSettings::theIMAPResourceFolderParent();
  KMFolder* folderParent = kmkernel->findFolderById( parentName );
  kdDebug(5006) << k_funcinfo << " folderParent=" << folderParent << endl;
  if ( folderParent )  // cool it exists now
  {
    KMAccount* account = kmkernel->acctMgr()->find( GlobalSettings::theIMAPResourceAccount() );
    if ( account )
      disconnect( account, SIGNAL( finishedCheck( bool, CheckStatus ) ),
                  this, SLOT( slotCheckDone() ) );
    readConfig();
  }
}

KMFolder* KMailICalIfaceImpl::initFolder( const char* typeString,
                                          KMail::FolderContentsType contentsType )
{
  // Figure out what type of folder this is supposed to be
  KMFolderType type = mFolderType;
  if( type == KMFolderTypeUnknown ) type = KMFolderTypeMaildir;

  KFolderTreeItem::Type itemType = s_folderContentsType[contentsType].treeItemType;
  //kdDebug(5006) << "KMailICalIfaceImpl::initFolder " << folderName( itemType ) << endl;

  // Find the folder
  KMFolder* folder = findStandardResourceFolder( mFolderParentDir, contentsType );
  if( !folder && GlobalSettings::theIMAPResourceStorageFormat() == GlobalSettings::EnumTheIMAPResourceStorageFormat::XML ) {
    // Maybe there's a folder with the right name - well, change its type then
    KMFolderNode* node = mFolderParentDir->hasNamedFolder( folderName( itemType ) );
    if ( node && !node->isDir() ) {
      folder = static_cast<KMFolder *>( node );
      folder->storage()->setContentsType( contentsType );
      kdDebug(5006) << "Adjusted type of " << folder->location() << " to contentsType " << contentsType << endl;
      folder->storage()->writeConfig();
    }
  }

  if ( !folder ) {
    // The folder isn't there yet - create it
    folder =
      mFolderParentDir->createFolder( folderName( itemType ), false, type );
    if( mFolderType == KMFolderTypeImap ) {
      KMFolderImap* parentFolder = static_cast<KMFolderImap*>( mFolderParent->storage() );
      parentFolder->createFolder( folderName( itemType ) );
      static_cast<KMFolderImap*>( folder->storage() )->setAccount( parentFolder->account() );
    }
    // Groupware folder created, use the global setting for storage format
    setStorageFormat( folder, GlobalSettings::theIMAPResourceStorageFormat() == GlobalSettings::EnumTheIMAPResourceStorageFormat::XML ? StorageXML : StorageIcalVcard );
  } else {
    KConfigGroup configGroup( kmkernel->config(), "GroupwareFolderInfo" );
    QString str = configGroup.readEntry( folder->idString() + "-storageFormat", "icalvcard" );
    FolderInfo info;
    info.mStorageFormat = ( str == "xml" ) ? StorageXML : StorageIcalVcard;
    info.mChanges = (FolderChanges) configGroup.readNumEntry(  folder->idString() + "-changes" );
    mFolderInfoMap.insert( folder, info );

    //kdDebug(5006) << "Found existing folder type " << itemType << " : " << folder->location()  << endl;
  }

  if( folder->canAccess() != 0 ) {
    KMessageBox::sorry(0, i18n("You do not have read/write permission to your %1 folder.")
                       .arg( folderName( itemType ) ) );
    return 0;
  }
  folder->setType( typeString );
  folder->storage()->setContentsType( contentsType );
  folder->setSystemFolder( true );
  folder->storage()->writeConfig();
  folder->open();
  // avoid multiple connections
  disconnect( folder, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
              this, SLOT( slotIncidenceAdded( KMFolder*, Q_UINT32 ) ) );
  disconnect( folder, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
              this, SLOT( slotIncidenceDeleted( KMFolder*, Q_UINT32 ) ) );
  disconnect( folder, SIGNAL( expunged( KMFolder* ) ),
              this, SLOT( slotRefreshFolder( KMFolder* ) ) );
  // Setup the signals to listen for changes
  connect( folder, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
           this, SLOT( slotIncidenceAdded( KMFolder*, Q_UINT32 ) ) );
  connect( folder, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
           this, SLOT( slotIncidenceDeleted( KMFolder*, Q_UINT32 ) ) );
  connect( folder, SIGNAL( expunged( KMFolder* ) ),
           this, SLOT( slotRefreshFolder( KMFolder* ) ) );

  return folder;
}

static void cleanupFolder( KMFolder* folder, KMailICalIfaceImpl* _this )
{
  if( folder ) {
    folder->setType( "plain" );
    folder->setSystemFolder( false );
    folder->disconnect( _this );
    folder->close();
  }
}

void KMailICalIfaceImpl::cleanup()
{
  cleanupFolder( mContacts, this );
  cleanupFolder( mCalendar, this );
  cleanupFolder( mNotes, this );
  cleanupFolder( mTasks, this );
  cleanupFolder( mJournals, this );

  mContacts = mCalendar = mNotes = mTasks = mJournals = 0;
}

void KMailICalIfaceImpl::loadPixmaps() const
{
  static bool pixmapsLoaded = false;

  if( mUseResourceIMAP && !pixmapsLoaded ) {
    pixmapsLoaded = true;
    pixContacts = new QPixmap( UserIcon("kmgroupware_folder_contacts"));
    pixCalendar = new QPixmap( UserIcon("kmgroupware_folder_calendar"));
    pixNotes    = new QPixmap( UserIcon("kmgroupware_folder_notes"));
    pixTasks    = new QPixmap( UserIcon("kmgroupware_folder_tasks"));
    pixJournals = new QPixmap( UserIcon("kmgroupware_folder_journals"));
  }
}

QString KMailICalIfaceImpl::folderPixmap( KFolderTreeItem::Type type ) const
{
  if( !mUseResourceIMAP )
    return QString::null;

  if( type == KFolderTreeItem::Contacts )
    return QString::fromLatin1( "kmgroupware_folder_contacts" );
  else if( type == KFolderTreeItem::Calendar )
    return QString::fromLatin1( "kmgroupware_folder_calendar" );
  else if( type == KFolderTreeItem::Notes )
    return QString::fromLatin1( "kmgroupware_folder_notes" );
  else if( type == KFolderTreeItem::Tasks )
    return QString::fromLatin1( "kmgroupware_folder_tasks" );
  else if( type == KFolderTreeItem::Journals )
    return QString::fromLatin1( "kmgroupware_folder_journals" );

  return QString::null;
}

QPixmap* KMailICalIfaceImpl::pixContacts;
QPixmap* KMailICalIfaceImpl::pixCalendar;
QPixmap* KMailICalIfaceImpl::pixNotes;
QPixmap* KMailICalIfaceImpl::pixTasks;
QPixmap* KMailICalIfaceImpl::pixJournals;

static void reloadFolderTree()
{
  // Make the folder tree show the icons or not
  kmkernel->folderMgr()->contentsChanged();
}

// This is a very light-weight and fast 'parser' to retrieve
// a data entry from a vCal taking continuation lines
// into account
static void vPartMicroParser( const QString& str, QString& s )
{
  QString line;
  uint len = str.length();

  for( uint i=0; i<len; ++i){
    if( str[i] == '\r' || str[i] == '\n' ){
      if( str[i] == '\r' )
        ++i;
      if( i+1 < len && str[i+1] == ' ' ){
        // found a continuation line, skip it's leading blanc
        ++i;
      }else{
        // found a logical line end, process the line
        if( line.startsWith( s ) ) {
          s = line.mid( s.length() + 1 );
          return;
        }
        line = "";
      }
    } else {
      line += str[i];
    }
  }

  // Not found. Clear it
  s.truncate(0);
}

KMFolder* KMailICalIfaceImpl::findStandardResourceFolder( KMFolderDir* folderParentDir, KMail::FolderContentsType contentsType )
{
  if ( GlobalSettings::theIMAPResourceStorageFormat() == GlobalSettings::EnumTheIMAPResourceStorageFormat::XML )
  {
    // Look for a folder with an annotation like "event.default"
    QPtrListIterator<KMFolderNode> it( *folderParentDir );
    for ( ; it.current(); ++it ) {
      if ( !it.current()->isDir() ) {
        KMFolder* folder = static_cast<KMFolder *>( it.current() );
        if ( folder->folderType() == KMFolderTypeCachedImap ) {
          QString annotation = static_cast<KMFolderCachedImap*>( folder->storage() )->annotationFolderType();
          //kdDebug(5006) << "findStandardResourceFolder: " << folder->name() << " has annotation " << annotation << endl;
          if ( annotation == QString( s_folderContentsType[contentsType].annotation ) + ".default" )
            return folder;
        }
      }
    }
    kdDebug(5006) << "findStandardResourceFolder: no standard resource folder for " << s_folderContentsType[contentsType].annotation << endl;
    return 0;
  }
  else // icalvcard: look up standard resource folders by name
  {
    KFolderTreeItem::Type itemType = s_folderContentsType[contentsType].treeItemType;
    unsigned int folderLanguage = GlobalSettings::theIMAPResourceFolderLanguage();
    if( folderLanguage > 3 ) folderLanguage = 0;
    KMFolderNode* node = folderParentDir->hasNamedFolder( folderName( itemType, folderLanguage ) );
    if ( !node || node->isDir() )
      return 0;
    return static_cast<KMFolder*>( node );
  }
}

void KMailICalIfaceImpl::setResourceQuiet(bool q)
{
  mResourceQuiet = q;
}

bool KMailICalIfaceImpl::isResourceQuiet() const
{
  return mResourceQuiet;
}

#include "kmailicalifaceimpl.moc"
