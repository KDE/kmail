/*
    This file is part of KMail.

    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 - 2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>

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
#include "kmacctcachedimap.h"
#include "kmacctimap.h"

// Local helper methods
static void vPartMicroParser( const QString& str, QString& s );
static void reloadFolderTree();
// Local helper class
class KMailICalIfaceImpl::ExtraFolder {
public:
  ExtraFolder( KMFolder* f ) : folder( f ) {}
  QGuardedPtr<KMFolder> folder;
};

// The index in this array is the KMail::FolderContentsType enum
static const struct {
  const char* contentsTypeStr; // the string used in the DCOP interface
  const char* mimetype;
} s_folderContentsType[] = {
  { "Mail", "application/x-vnd.kolab.mail" },
  { "Calendar", "application/x-vnd.kolab.event" },
  { "Contact", "application/x-vnd.kolab.contact" },
  { "Note", "application/x-vnd.kolab.note" },
  { "Task", "application/x-vnd.kolab.task" },
  { "Journal", "application/x-vnd.kolab.journal" }
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

/*
  This interface have three parts to it - libkcal interface;
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

  mExtraFolders.setAutoDelete( true );
}

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

  // Find the folder
  KMFolder* f = folderFromType( type, folder );
  if( !f ) {
    kdError(5006) << "addIncidence(" << type << "," << folder << ") : Not an IMAP resource folder" << endl;
    return false;
  }
  if ( storageFormat( f ) != StorageIcalVcard ) {
    kdError(5006) << "addIncidence(" << type << "," << folder << ") : Folder has wrong storage format " << storageFormat( f ) << endl;
    return false;
  }

  bool rc = false;
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;
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

  mResourceQuiet = quiet;
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
    if ( part->hasHeaders()
         && part->Headers().HasContentType()
         && part->Headers().ContentType().TypeStr() == sType
         && part->Headers().ContentType().SubtypeStr() == sSubtype)
      return part;
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
  kdDebug() << "--debugBodyParts " << foo << "--" << endl;
  for ( DwBodyPart* part = msg.getFirstDwBodyPart(); part; part = part->Next() ) {
    if ( part->hasHeaders() ) {
      kdDebug() << " bodypart: " << part << endl;
      kdDebug() << "        " << part->Headers().AsString().c_str() << endl;
    }
    else
      kdDebug() << " part " << part << " has no headers" << endl;
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
                                                const QStringList& attachmentURLs,
                                                const QStringList& attachmentNames,
                                                const QStringList& attachmentMimetypes )
{
  kdDebug(5006) << "KMailICalIfaceImpl::addIncidenceKolab( " << attachmentNames << " )" << endl;

  Q_UINT32 sernum = 0;
  bool bAttachOK = true;
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;

  // Make a new message for the incidence
  KMMessage* msg = new KMMessage();
  msg->initHeader();
  msg->setType( DwMime::kTypeMultipart );
  msg->setSubtype( DwMime::kSubtypeMixed );
  msg->headers().ContentType().CreateBoundary( 0 );
  msg->headers().ContentType().Assemble();
  msg->setSubject( subject );
  msg->setAutomaticFields( true );
  // add a first body part to be displayed by all mailer
  // than cn NOT display Kolab data: no matter if these
  // mailers are MIME compliant or not
  KMMessagePart firstPart;
  firstPart.setType(    DwMime::kTypeText     );
  firstPart.setSubtype( DwMime::kSubtypePlain );
  const char * firstPartTextUntranslated = I18N_NOOP(
    "This is a Kolab Groupware object.\nTo view this object you"
    " will need an email client that can understand the Kolab"
    " Groupware format.\nFor a list of such email clients please"
    " visit\nhttp://www.kolab.org/kolab-clients.html");
  QString firstPartText = i18n( firstPartTextUntranslated );
  if ( firstPartText != firstPartTextUntranslated ) {
    firstPartText.append("\n\n-----------------------------------------------------\n\n");
    firstPartText.append( firstPartTextUntranslated );
  }
  firstPart.setBodyFromUnicode( firstPartText );
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
    if ( bymimetype )
      msg->setHeaderField( "X-Kolab-Type", *itmime );
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

  mResourceQuiet = quiet;
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
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;

  KMMessage* msg = findMessageByUID( uid, f );
  if( msg ) {
    // Message found - delete it and return happy
    deleteMsg( msg );
    rc = true;
  } else {
    kdDebug(5006) << "Message not found, cannot remove id " << uid << endl;
  }

  mResourceQuiet = quiet;
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
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;

  KMMessage* msg = findMessageBySerNum( sernum, f );
  if( msg ) {
    // Message found - delete it and return happy
    deleteMsg( msg );
    rc = true;
  } else {
    kdDebug(5006) << "Message not found, cannot remove serNum " << sernum << endl;
  }

  mResourceQuiet = quiet;
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
  if( !f ) {
    kdError(5006) << "incidences(" << type << "," << folder << ") : Not an IMAP resource folder" << endl;
    return ilist;
  }
  if ( storageFormat( f ) != StorageIcalVcard ) {
    kdError(5006) << "incidences(" << type << "," << folder << ") : Folder has wrong storage format " << storageFormat( f ) << endl;
    return ilist;
  }

  f->open();
  QString s;
  for( int i=0; i<f->count(); ++i ) {
    bool unget = !f->isMessage(i);
    if( KMGroupware::vPartFoundAndDecoded( f->getMsg( i ), s ) )
      ilist << s;
    if( unget ) f->unGetMsg(i);
  }

  return ilist;
}

QMap<Q_UINT32, QString> KMailICalIfaceImpl::incidencesKolab( const QString& mimetype,
                                                             const QString& resource )
{
  /// Get the mimetype attachments from this folder. Returns a
  /// QMap with serialNumber/attachment pairs.
  /// (serial numbers of the mail are provided for easier later update)

  QMap<Q_UINT32, QString> aMap;
  if( !mUseResourceIMAP )
    return aMap;

  kdDebug(5006) << "KMailICalIfaceImpl::incidencesKolab( " << mimetype << ", "
                << resource << " )" << endl;

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
  QString s;
  for( int i=0; i<f->count(); ++i ) {
    bool unget = !f->isMessage(i);
    KMMessage* msg = f->getMsg( i );
    if( msg ){
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
      if( unget ) f->unGetMsg(i);
    }
  }
  return aMap;
}

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

static QString subResourceLabel( KMFolder* folder )
{
  // Let's show account name + imap path of folder
  // Showing the folder label isn't enough, one could have several folders
  // called "Notes", e.g. some coming from other people sharing theirs.
  QString accountName;
  QString imapPath;
  FolderStorage* storage = folder->storage();
  if ( storage->folderType() == KMFolderTypeCachedImap ) {
    KMFolderCachedImap* dimap = static_cast<KMFolderCachedImap*>( storage );
    accountName = dimap->account()->name();
    imapPath = dimap->imapPath();
  } else if ( storage->folderType() == KMFolderTypeImap ) {
    KMFolderImap* imap = static_cast<KMFolderImap*>( storage );
    accountName = imap->account()->name();
    imapPath = imap->imapPath();
  } else {
    accountName = QString::null;
    imapPath = folder->label();
  }
  if ( !accountName.isEmpty() )
    accountName += ' ';
  return accountName + imapPath;
}

QValueList<KMailICalIfaceImpl::SubResource> KMailICalIfaceImpl::subresourcesKolab( const QString& contentsType )
{
  QValueList<SubResource> subResources;

  // Add the default one
  KMFolder* f = folderFromType( contentsType, QString::null );
  if ( f && storageFormat( f ) == StorageXML ) {
    subResources.append( SubResource( f->location(), subResourceLabel( f ), !f->isReadOnly() ) );
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
      subResources.append( SubResource( f->location(), subResourceLabel( f ), !f->isReadOnly() ) );
      kdDebug(5006) << "Adding(2) folder " << f->location() << "     " <<
              ( f->isReadOnly() ? "readonly" : "" ) << endl;
    }
  }

  if ( subResources.isEmpty() )
    kdDebug(5006) << "subresourcesKolab: No folder found for " << contentsType << endl;
  return subResources;
}

bool KMailICalIfaceImpl::isWritableFolder( const QString& type,
                                           const QString& resource )
{
  KMFolder* f = folderFromType( type, resource );
  if ( !f )
    // Definitely not writable
    return false;

  return !f->isReadOnly();
}

bool KMailICalIfaceImpl::update( const QString& type, const QString& folder,
                                 const QStringList& entries )
{
  if( !mUseResourceIMAP )
    return false;

  if( entries.count() & 2 == 1 )
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

bool KMailICalIfaceImpl::update( const QString& type, const QString& folder,
                                 const QString& uid, const QString& entry )
{
  if( !mUseResourceIMAP )
    return false;

  kdDebug(5006) << "Update( " << type << ", " << folder << ", " << uid << ")\n";

  // Find the folder and the incidence in it
  KMFolder* f = folderFromType( type, folder );
  if( !f ) {
    kdError(5006) << "update(" << type << "," << folder << ") : Not an IMAP resource folder" << endl;
    return false;
  }
  if ( storageFormat( f ) != StorageIcalVcard ) {
    kdError(5006) << "update(" << type << "," << folder << ") : Folder has wrong storage format " << storageFormat( f ) << endl;
    return false;
  }

  bool rc = true;
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;
  KMMessage* msg = findMessageByUID( uid, f );
  if( msg ) {
    // Message found - update it
    deleteMsg( msg );
    addIncidence( type, folder, uid, entry );
    rc = true;
  } else {
    kdDebug(5006) << type << " not found, cannot update uid " << uid << endl;
    // Since it doesn't seem to be there, save it instead
    addIncidence( type, folder, uid, entry );
  }

  mResourceQuiet = quiet;
  return rc;
}

Q_UINT32 KMailICalIfaceImpl::update( const QString& resource,
                                     Q_UINT32 sernum,
                                     const QString& subject,
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
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;

  kdDebug(5006) << "Updating in folder " << f << " " << f->location() << endl;
  KMMessage* msg = 0;
  if ( sernum != 0 )
    msg = findMessageBySerNum( sernum, f );
  if ( msg ) {
    // Message found - make a copy and update it:
    KMMessage* newMsg = new KMMessage( *msg );
    newMsg->setSubject( subject );
    newMsg->setParent( 0 ); // workaround strange line in KMMsgBase::assign. newMsg is not in any folder yet.

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

  }else{
    // Message not found - store it newly
    rc = addIncidenceKolab( *f, subject,
                            attachmentURLs,
                            attachmentNames,
                            attachmentMimetypes );
  }

  mResourceQuiet = quiet;
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

// KMail added a file to one of the groupware folders
void KMailICalIfaceImpl::slotIncidenceAdded( KMFolder* folder,
                                             Q_UINT32 sernum )
{
  if( mResourceQuiet || !mUseResourceIMAP )
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
    bool ok = false;
    KMMessage* msg = folder->getMsg( i );
    switch( storageFormat( folder ) ) {
    case StorageIcalVcard:
      // Read the iCal or vCard
      ok = KMGroupware::vPartFoundAndDecoded( msg, s );
      break;
    case StorageXML:
      // Read the XML from the attachment with the given mimetype
      ok = kolabXMLFoundAndDecoded( *msg, folderKolabMimeType( folder->storage()->contentsType() ), s );
      break;
    }
    if ( ok ) {
      kdDebug(5006) << "Emitting DCOP signal incidenceAdded( " << type
                      << ", " << folder->location() << ", " << s << " )" << endl;
      incidenceAdded( type, folder->location(), s );
      incidenceAdded( type, folder->location(), sernum, s );
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
        if( KMGroupware::vPartFoundAndDecoded( msg, s ) ) {
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
bool KMailICalIfaceImpl::isResourceImapFolder( KMFolder* folder ) const
{
  return mUseResourceIMAP && folder &&
    ( folder == mCalendar || folder == mTasks || folder == mJournals ||
      folder == mNotes || folder == mContacts );
  // Extra folders are not checked here, since those can't be hidden (right?)
}

bool KMailICalIfaceImpl::hideResourceImapFolder( KMFolder* folder ) const
{
  return mHideFolders && isResourceImapFolder( folder );
}

KFolderTreeItem::Type KMailICalIfaceImpl::folderType( KMFolder* folder ) const
{
  if( mUseResourceIMAP && folder ) {
    if( folder == mCalendar )
      return KFolderTreeItem::Calendar;
    else if( folder == mContacts )
      return KFolderTreeItem::Contacts;
    else if( folder == mNotes )
      return KFolderTreeItem::Notes;
    else if( folder == mTasks )
      return KFolderTreeItem::Tasks;
    else if( folder == mJournals )
      return KFolderTreeItem::Journals;
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
    folderNames[2][KFolderTreeItem::Tasks] = QString::fromLatin1("T�ches");
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
  if( !folder ) return 0;

  for( int i=0; i<folder->count(); ++i ) {
    bool unget = !folder->isMessage(i);
    KMMessage* msg = folder->getMsg( i );
    if( msg ) {
      QString vCal;
      if( KMGroupware::vPartFoundAndDecoded( msg, vCal ) ) {
        QString msgUid( "UID" );
        vPartMicroParser( vCal, msgUid );
        if( msgUid == uid )
          return msg;
      }
    }
    if( unget ) folder->unGetMsg(i);
  }

  return 0;
}

// Find message matching a given UID
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
}

void KMailICalIfaceImpl::folderContentsTypeChanged( KMFolder* folder,
                                                    KMail::FolderContentsType contentsType )
{
  if ( !mUseResourceIMAP )
    return;
  kdDebug(5006) << "folderContentsTypeChanged( " << folder->name()
                << ", " << contentsType << ")\n";

  if ( isResourceImapFolder( folder ) )
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

    // And listen to changes from it
    connect( folder, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
             this, SLOT( slotIncidenceAdded( KMFolder*, Q_UINT32 ) ) );
    connect( folder, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
             this, SLOT( slotIncidenceDeleted( KMFolder*, Q_UINT32 ) ) );
  }

  // Tell about the new resource
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
  if ( it != mFolderInfoMap.end() )
    (*it).mStorageFormat = format;
  else {
    FolderInfo info;
    info.mStorageFormat = format;
    mFolderInfoMap.insert( folder, info );
  }
  KConfigGroup configGroup( kmkernel->config(), "GroupwareFolderInfo" );
  configGroup.writeEntry( folder->idString() + "-storageFormat",
                          format == StorageXML ? "xml" : "icalvcard" );
}

KMFolder* KMailICalIfaceImpl::findResourceFolder( const QString& resource )
{
  // Try the standard folders
  if( mCalendar->location() == resource )
    return mCalendar;
  if ( mContacts->location() == resource )
    return mContacts;
  if ( mNotes->location() == resource )
    return mNotes;
  if ( mTasks->location() == resource )
    return mTasks;
  if ( mJournals->location() == resource )
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
  unsigned int folderLanguage = GlobalSettings::theIMAPResourceFolderLanguage();
  if( folderLanguage > 3 ) folderLanguage = 0;
  if ( GlobalSettings::theIMAPResourceStorageFormat() == GlobalSettings::EnumTheIMAPResourceStorageFormat::XML )
    folderLanguage = 0; // xml storage -> English
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
  KMFolderNode* node;
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Calendar, folderLanguage ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mCalendar = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Tasks, folderLanguage ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mTasks = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Journals, folderLanguage ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mJournals = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Contacts, folderLanguage ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mContacts = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Notes, folderLanguage ) );
  if( !node || node->isDir() ) {
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
      mFolderParent = 0;
      reloadFolderTree();
      return;
    }
  }

  // Check if something changed
  if( mUseResourceIMAP && !makeSubFolders && mFolderParent == folderParentDir
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
  mFolderLanguage = folderLanguage;
  mFolderParent = folderParentDir;
  mFolderType = folderType;
  mHideFolders = hideFolders;

  // Close the previous folders
  cleanup();

  // Set the new folders
  mCalendar = initFolder( KFolderTreeItem::Calendar, "GCa", KMail::ContentsTypeCalendar );
  mTasks    = initFolder( KFolderTreeItem::Tasks, "GTa", KMail::ContentsTypeTask );
  mJournals = initFolder( KFolderTreeItem::Journals, "GTa", KMail::ContentsTypeJournal );
  mContacts = initFolder( KFolderTreeItem::Contacts, "GCo", KMail::ContentsTypeContact );
  mNotes    = initFolder( KFolderTreeItem::Notes, "GNo", KMail::ContentsTypeNote );

  mCalendar->setLabel( i18n( "Calendar" ) );
  mTasks->setLabel( i18n( "Tasks" ) );
  mJournals->setLabel( i18n( "Journal" ) );
  mContacts->setLabel( i18n( "Contacts" ) );
  mNotes->setLabel( i18n( "Notes" ) );

  // Connect the expunged signal
  connect( mCalendar, SIGNAL( expunged() ), this, SLOT( slotRefreshCalendar() ) );
  connect( mTasks,    SIGNAL( expunged() ), this, SLOT( slotRefreshTasks() ) );
  connect( mJournals, SIGNAL( expunged() ), this, SLOT( slotRefreshJournals() ) );
  connect( mContacts, SIGNAL( expunged() ), this, SLOT( slotRefreshContacts() ) );
  connect( mNotes,    SIGNAL( expunged() ), this, SLOT( slotRefreshNotes() ) );

  // Bad hack
  connect( mNotes,    SIGNAL( changed() ),  this, SLOT( slotRefreshNotes() ) );

  kdDebug() << k_funcinfo << "mCalendar=" << mCalendar << " " << mCalendar->location() << endl;
  kdDebug() << k_funcinfo << "mNotes=" << mNotes << " " << mNotes->location() << endl;

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


  // Make KOrganizer re-read everything
#if 0 // old way, not enough finegrained (and most resources don't call doOpen, so they miss the subresources anyway)
  slotRefresh( "Calendar" );
  slotRefresh( "Task" );
  slotRefresh( "Journal" );
  slotRefresh( "Contact" );
  slotRefresh( "Notes" );
#else
  subresourceAdded( folderContentsType( KMail::ContentsTypeCalendar ), mCalendar->location() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeTask ), mTasks->location() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeJournal ), mJournals->location() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeContact ), mContacts->location() );
  subresourceAdded( folderContentsType( KMail::ContentsTypeNote ), mNotes->location() );
  // This also shows that we might even get rid of the mCalendar etc. special
  // case and just use ExtraFolder for all
#endif
  reloadFolderTree();
}

void KMailICalIfaceImpl::slotCheckDone()
{
  QString parentName = GlobalSettings::theIMAPResourceFolderParent();
  KMFolder* folderParent = kmkernel->findFolderById( parentName );
  kdDebug() << k_funcinfo << " folderParent=" << folderParent << endl;
  if ( folderParent )  // cool it exists now
  {
    KMAccount* account = kmkernel->acctMgr()->find( GlobalSettings::theIMAPResourceAccount() );
    if ( account )
      disconnect( account, SIGNAL( finishedCheck( bool, CheckStatus ) ),
                  this, SLOT( slotCheckDone() ) );
    readConfig();
  }
}

void KMailICalIfaceImpl::slotRefreshCalendar() { slotRefresh( "Calendar" ); }
void KMailICalIfaceImpl::slotRefreshTasks() { slotRefresh( "Task" ); }
void KMailICalIfaceImpl::slotRefreshJournals() { slotRefresh( "Journal" ); }
void KMailICalIfaceImpl::slotRefreshContacts() { slotRefresh( "Contact" ); }
void KMailICalIfaceImpl::slotRefreshNotes() { slotRefresh( "Notes" ); }

KMFolder* KMailICalIfaceImpl::initFolder( KFolderTreeItem::Type itemType,
                                          const char* typeString,
                                          KMail::FolderContentsType contentsType )
{
  // Figure out what type of folder this is supposed to be
  KMFolderType type = mFolderType;
  if( type == KMFolderTypeUnknown ) type = KMFolderTypeMaildir;

  // Find the folder
  KMFolder* folder = 0;
  KMFolderNode* node = mFolderParent->hasNamedFolder( folderName( itemType ) );
  if( node && !node->isDir() ) folder = static_cast<KMFolder*>(node);
  if( !folder ) {
    // The folder isn't there yet - create it
    folder =
      mFolderParent->createFolder( folderName( itemType ), false, type );
    if( mFolderType == KMFolderTypeImap )
      static_cast<KMFolderImap*>( folder->storage() )->
        createFolder( folderName( itemType ) );

    // Groupware folder created, use the global setting for storage format
    setStorageFormat( folder, GlobalSettings::theIMAPResourceStorageFormat() == GlobalSettings::EnumTheIMAPResourceStorageFormat::XML ? StorageXML : StorageIcalVcard );
  } else {
    KConfigGroup configGroup( kmkernel->config(), "GroupwareFolderInfo" );
    QString str = configGroup.readEntry( folder->idString() + "-storageFormat", "icalvcard" );
    FolderInfo info;
    info.mStorageFormat = ( str == "xml" ) ? StorageXML : StorageIcalVcard;
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
  folder->open();

  // Setup the signals to listen for changes
  connect( folder, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
           this, SLOT( slotIncidenceAdded( KMFolder*, Q_UINT32 ) ) );
  connect( folder, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
           this, SLOT( slotIncidenceDeleted( KMFolder*, Q_UINT32 ) ) );

  return folder;
}

static void cleanupFolder( KMFolder* folder, KMailICalIfaceImpl* _this )
{
  if( folder ) {
    folder->setType( "plain" );
    folder->setSystemFolder( false );
    folder->disconnect( _this );
    folder->close( true );
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
    // TODO: Find a pixmap for journals
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

#include "kmailicalifaceimpl.moc"
