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

// Local helper methods
static void vPartMicroParser( const QString& str, QString& s );
static void reloadFolderTree();

// Local helper class
class KMailICalIfaceImpl::ExtraFolder {
public:
  ExtraFolder( KMFolder* f ) : folder( f ) {}
  KMFolder* folder;
};

// The index in this array is the KMail::FolderContentsType enum
static const struct {
  const char* contentsTypeStr; // the string used in the DCOP interface
} s_folderContentsType[] = {
  { "Mail" },
  { "Calendar" },
  { "Contact" },
  { "Note" },
  { "Task" },
  { "Journal" }
};

static QString folderContentsType( KMail::FolderContentsType type )
{
  return s_folderContentsType[type].contentsTypeStr;
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


// Add (or overwrite, resp.) an attachment in an existing mail,
// attachments must be local files, they are identified by their names.
// return value: wrong if attachment could not be added/updated
bool KMailICalIfaceImpl::updateAttachment( KMMessage& msg,
                                           const QString& attachmentURL )
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
      msgPart.setName( fileName );
      msgPart.setType(DwMime::kTypeText);
      msgPart.setSubtype(DwMime::kSubtypePlain);
      msgPart.setContentDisposition( QString("attachment;\n  filename=\"%1\"")
          .arg( fileName ).latin1() );
      QValueList<int> dummy;
      msgPart.setBodyAndGuessCte( rawData, dummy );
      if( !url.fileEncoding().isEmpty() )
        msgPart.setCharset( url.fileEncoding().latin1() );
      msgPart.setPartSpecifier( fileName );

      // quickly searching for our message part: since Kolab parts are
      // top-level parts we do *not* have to travel into embedded multiparts
      DwBodyPart* part = msg.getFirstDwBodyPart();
      while( part ){
        if( fileName == part->partId() ){
          DwBodyPart* newPart = msg.createDWBodyPart( &msgPart );
          // Make sure the replacing body part is pointing
          // to the same next part as the original body part.
          newPart->SetNext( part->Next() );
          // call DwBodyPart::operator =
          // which calls DwEntity::operator =
          *part = *newPart;
          delete newPart;
          msg.setNeedsAssembly();
          kdDebug(5006) << "Attachment updated." << endl;
          bOK = true;
          break;
        }
        part = part->Next();
      }

      if( !bOK ){
        kdDebug(5006) << "num: " << msg.numBodyParts() << endl;
        msg.addBodyPart( &msgPart );
        kdDebug(5006) << "num: " << msg.numBodyParts() << endl;
        kdDebug(5006) << "Attachment added." << endl;
        bOK = true;
      }
    }else{
      kdDebug(5006) << "Attachment " << attachmentURL << " can not be read." << endl;
    }
  }else{
    kdDebug(5006) << "Attachment " << attachmentURL << " not a local file." << endl;
  }

  return bOK;
}


// Delete an attachment in an existing mail.
// return value: wrong if attachment could not be deleted
//
// This code could be optimized: for now we just replace
// the attachment by an empty dummy attachment since Mimelib
// does not provide an option for deleting attachments yet.
bool KMailICalIfaceImpl::deleteAttachment( KMMessage& msg,
                                           const QString& attachmentURL )
{
  kdDebug(5006) << "KMailICalIfaceImpl::deleteAttachment( " << attachmentURL << " )" << endl;

  bool bOK = false;

  // quickly searching for our message part: since Kolab parts are
  // top-level parts we do *not* have to travel into embedded multiparts
  DwBodyPart* part = msg.getFirstDwBodyPart();
  while( part ){
    if( attachmentURL == part->partId() ){
      DwBodyPart emptyPart;
      // Make sure the empty replacement body part is pointing
      // to the same next part as the to be deleted body part.
      emptyPart.SetNext( part->Next() );
      // call DwBodyPart::operator =
      *part = emptyPart;
      msg.setNeedsAssembly();
      kdDebug(5006) << "Attachment deleted." << endl;
      bOK = true;
      break;
    }
    part = part->Next();
  }

  if( !bOK ){
    kdDebug(5006) << "Attachment " << attachmentURL << " not found." << endl;
  }

  return bOK;
}


// Store a new entry that was received from the resource
Q_UINT32 KMailICalIfaceImpl::addIncidence( KMFolder& folder,
                                           const QCString& subject,
                                           const QStringList& attachments )
{
  kdDebug(5006) << "KMailICalIfaceImpl::addIncidence( " << attachments << " )" << endl;

  Q_UINT32 sernum = 0;
  bool bAttachOK = true;
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;

  // Make a new message for the incidence
  KMMessage* msg = new KMMessage();
  msg->initHeader();
  msg->setType( DwMime::kTypeMultipart );
  msg->setSubtype( DwMime::kSubtypeMixed );
  msg->setSubject( subject ); // e.g.: "internal kolab data: Do not delete this mail."
  msg->setCharset( "US-ASCII" );
  msg->setAutomaticFields( true );

  // Add all attachments by reading them from their temp. files
  for( QStringList::ConstIterator it = attachments.begin();
       it != attachments.end(); ++it ){
    if( !updateAttachment( *msg, *it ) ){
      kdDebug(5006) << "Attachment error, can not add Incidence." << endl;
      bAttachOK = false;
      break;
    }
  }

  if( bAttachOK ){
    // Mark the message as read and store it in the folder
    //msg->setBody( "Your mailer can not display this format.\nSee http://www.kolab.org for details on the Kolab storage format." );
    msg->cleanupHeader();
    msg->touch();
    if ( folder.addMsg( msg ) == 0 )
      // Message stored
      sernum = msg->getMsgSerNum();
    kdDebug(5006) << "addIncidence(): Message done and saved. Sernum: "
                  << sernum << endl;
  } else
    kdError(5006) << "addIncidence: Message *NOT* saved!\n";

  mResourceQuiet = quiet;
  return sernum;
}


// The resource orders a deletion
bool KMailICalIfaceImpl::deleteIncidence( KMFolder& folder,
                                          const QString& uid,
                                          Q_UINT32 serNum )
{
  if( !mUseResourceIMAP )
    return false;

  bool rc = false;
  bool quiet = mResourceQuiet;
  mResourceQuiet = true;

  const bool bIsImap = isResourceImapFolder( &folder );
  KMMessage* msg = bIsImap
                 ? findMessageBySerNum( serNum, &folder )
                 : findMessageByUID(    uid,    &folder );
  if( msg ) {
    // Message found - delete it and return happy
    deleteMsg( msg );
    rc = true;
  }else{
    if( bIsImap )
      kdDebug(5006) << "Message not found, cannot remove serNum " << serNum << endl;
    else
      kdDebug(5006) << "Message not found, cannot remove id " << uid << endl;
  }

  mResourceQuiet = quiet;
  return rc;
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

  rc = deleteIncidence( folder, uid, 0 );

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

  rc = deleteIncidence( *f, "", sernum );

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
      if( sType.isEmpty() || sSubtype.isEmpty() ){
        kdError(5006) << mimetype << " not an type/subtype combination" << endl;
      }else{
        const int msgType    = DwTypeStrToEnum(   DwString(sType));
        const int msgSubtype = DwSubtypeStrToEnum(DwString(sSubtype));
        DwBodyPart* dwPart = msg->findDwBodyPart( msgType,
                                                  msgSubtype );
        if( dwPart ){
          KMMessagePart msgPart;
          KMMessage::bodyPart(dwPart, &msgPart);
          aMap.insert(msg->getMsgSerNum(), msgPart.body());
        }else{
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
    if ( f->storage()->contentsType() == t
         && storageFormat( f ) == StorageIcalVcard )
      lst << f->location();
  }

  return lst;
}

QMap<QString, bool> KMailICalIfaceImpl::subresourcesKolab( const QString& contentsType )
{
  QMap<QString, bool> map;

  kdDebug(5006) << k_funcinfo << endl;
  // Add the default one
  KMFolder* f = folderFromType( contentsType, QString::null );
  if ( f && storageFormat( f ) == StorageXML ) {
    map.insert( f->location(), !f->isReadOnly() );
    kdDebug(5006) << "Adding(1) folder " << f->location() << endl;
  }

  // get the extra ones
  const KMail::FolderContentsType t = folderContentsType( contentsType );
  QDictIterator<ExtraFolder> it( mExtraFolders );
  for ( ; it.current(); ++it ){
    f = it.current()->folder;
    if ( f->storage()->contentsType() == t
         && storageFormat( f ) == StorageXML ) {
      map.insert( f->location(), !f->isReadOnly() );
      kdDebug(5006) << "Adding(2) folder " << f->location() << endl;
    }
  }

  return map;
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
                                     const QCString& subject,
                                     const QStringList& attachments,
                                     const QStringList& deletedAttachments )
{
  Q_UINT32 rc = 0;

  // This finds the message with serial number "id", sets the
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

  kdDebug(5006) << "KMailICalIfaceImpl::update( " << resource << ", " << sernum << " )\n";

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

  kdDebug(5006) << "Updating in folder " << f << endl;
  KMMessage* msg = 0;
  if ( sernum != 0 )
    msg = findMessageBySerNum( sernum, f );
  if ( msg ) {
    // Message found - update it:

    // Delete some attachments according to list
    for( QStringList::ConstIterator it = deletedAttachments.begin();
         it != deletedAttachments.end();
         ++it ){
      if( !deleteAttachment( *msg, *it ) ){
        // Note: It is _not_ an error if an attachment was already deleted.
      }
    }

    // Add all attachments by reading them from their temp. files
    for( QStringList::ConstIterator it2 = attachments.begin();
         it2 != attachments.end();
         ++it2 ){
      if( !updateAttachment( *msg, *it2 ) ){
        kdDebug(5006) << "Attachment error, can not add Incidence." << endl;
        break;
      }
    }
  }else{
    // Message not found - store it newly
    rc = addIncidence( *f, subject, attachments );
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

    // quickly searching for our message part: since Kolab parts are
    // top-level parts we do *not* have to travel into embedded multiparts
    DwBodyPart* part = msg->getFirstDwBodyPart();
    while( part ){
      if( filename == part->partId() ){
        // Save the xml file. Will be deleted at the end of this method
        KMMessagePart aPart;
        msg->bodyPart( part, &aPart );
        QByteArray rawData( aPart.bodyDecodedBinary() );

        KTempFile file;
        file.setAutoDelete( true );
        QTextStream* stream = file.textStream();
        stream->setEncoding( QTextStream::UnicodeUTF8 );
        stream->writeRawBytes( rawData.data(), rawData.size() );
        file.close();

        // Compose the return value
        url.setPath( file.name() );
        url.setFileEncoding( "UTF-8" );

        bOK = true;
        break;
      }
      part = part->Next();
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

  QString type = icalFolderType( folder );
  if( !type.isEmpty() ) {
    // Get the index of the mail
    int i = 0;
    KMFolder* aFolder = 0;
    kmkernel->msgDict()->getLocation( sernum, &aFolder, &i );
    assert( folder == aFolder );

    // Read the iCal or vCard
    bool unget = !folder->isMessage( i );
    QString s;
    if( KMGroupware::vPartFoundAndDecoded( folder->getMsg( i ), s ) ) {
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

  QString type = icalFolderType( folder );
  if( !type.isEmpty() ) {
    // Get the index of the mail
    int i = 0;
    KMFolder* aFolder = 0;
    kmkernel->msgDict()->getLocation( sernum, &aFolder, &i );
    assert( folder == aFolder );

    // Read the iCal or vCard
    bool unget = !folder->isMessage( i );
    QString s;
    if( KMGroupware::vPartFoundAndDecoded( folder->getMsg( i ), s ) ) {
      QString uid( "UID" );
      vPartMicroParser( s, uid );
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


QString KMailICalIfaceImpl::icalFolderType( KMFolder* folder ) const
{
  if( mUseResourceIMAP && folder ) {
    if( folder == mCalendar )
      return "Calendar";
    else if( folder == mContacts )
      return "Contact";
    else if( folder == mNotes )
      return "Note";
    else if( folder == mTasks )
      return "Task";
    else if( folder == mJournals )
      return "Journal";
    else {
      ExtraFolder* ef = mExtraFolders.find( folder->location() );
      if ( ef != 0 )
        return folderContentsType( folder->storage()->contentsType() );
    }
  }

  return QString::null;
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
  if( aFolder != folder ){
    kdWarning(5006) << "findMessageBySerNum( " << serNum << " ) folder not matching\n" << endl;
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
  kdDebug(5006) << "folderContentsTypeChanged( " << folder->name()
                << ", " << contentsType << ")\n";

  // Check if already know that folder
  ExtraFolder* ef = mExtraFolders.find( folder->location() );
  if ( !ef && contentsType == 0 ) // mail -> don't care
    return;

  if ( ef ) {
    // Notify that the old folder resource is no longer available
    subresourceDeleted(folderContentsType( folder->storage()->contentsType() ), folder->location() );

    if ( contentsType == 0 ) {
      // Delete the old entry, stop listening and stop here
      mExtraFolders.remove( folder->location() );
      folder->disconnect( this );
      return;
    }

    // So the type changed to another groupware type, ok.
  } else {
    // Make a new entry for the list
    ef = new ExtraFolder( folder );
    mExtraFolders.insert( folder->location(), ef );

    // And listen to changes from it
    connect( folder, SIGNAL( msgAdded( KMFolder*, Q_UINT32 ) ),
             this, SLOT( slotIncidenceAdded( KMFolder*, Q_UINT32 ) ) );
    connect( folder, SIGNAL( msgRemoved( KMFolder*, Q_UINT32 ) ),
             this, SLOT( slotIncidenceDeleted( KMFolder*, Q_UINT32 ) ) );
  }

  // Tell about the new resource
  subresourceAdded( folderContentsType( contentsType ), folder->location() );
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
  if ( ef && ef->folder->storage()->contentsType() == t )
    return ef->folder;

  return 0;
}

KMailICalIfaceImpl::StorageFormat KMailICalIfaceImpl::storageFormat( KMFolder* folder ) const
{
  FolderInfoMap::ConstIterator it = mFolderInfoMap.find( folder );
  if ( it != mFolderInfoMap.end() )
    return (*it).mStorageFormat;
  return StorageIcalVcard;
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
    // Maybe nothing was configured?
    folderParentDir = &(kmkernel->folderMgr()->dir());
    folderType = KMFolderTypeMaildir;
  } else {
    folderParentDir = folderParent->createChildFolder();
    folderType = folderParent->folderType();
  }

  // Make sure the folder parent has the subdirs
  bool makeSubFolders = false;
  KMFolderNode* node;
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Calendar ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mCalendar = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Tasks ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mTasks = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Journals ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mJournals = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Contacts ) );
  if( !node || node->isDir() ) {
    makeSubFolders = true;
    mContacts = 0;
  }
  node = folderParentDir->hasNamedFolder( folderName( KFolderTreeItem::Notes ) );
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

  // Make KOrganizer re-read everything
  slotRefresh( "Calendar" );
  slotRefresh( "Task" );
  slotRefresh( "Journal" );
  slotRefresh( "Contact" );
  slotRefresh( "Notes" );

  reloadFolderTree();
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

  KConfigGroup configGroup( kmkernel->config(), "GroupwareFolderInfo" );
  FolderInfo info;

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
    info.mStorageFormat = GlobalSettings::theIMAPResourceStorageFormat() == GlobalSettings::EnumTheIMAPResourceStorageFormat::XML ? StorageXML : StorageIcalVcard;
    configGroup.writeEntry( folder->idString() + "-storageFormat",
                            info.mStorageFormat == StorageXML ? "xml" : "icalvcard" );
  } else {
    QString str = configGroup.readEntry( folder->idString() + "-storageFormat", "icalvcard" );
    info.mStorageFormat = ( str == "xml" ) ? StorageXML : StorageIcalVcard;

    //kdDebug(5006) << "Found existing folder type " << itemType << " : " << folder->location()  << endl;
  }

  if( folder->canAccess() != 0 ) {
    KMessageBox::sorry(0, i18n("You do not have read/write permission to your %1 folder.")
                       .arg( folderName( itemType ) ) );
    return 0;
  }
  folder->setType( typeString );
  mFolderInfoMap.insert( folder, info );
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
