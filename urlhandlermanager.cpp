/*  -*- c++ -*-
    urlhandlermanager.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002-2003 Klar�lvdalens Datakonsult AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "urlhandlermanager.h"

#include "interfaces/urlhandler.h"
#include "interfaces/bodyparturlhandler.h"
#include "partNode.h"
#include "partnodebodypart.h"
#include "kmreaderwin.h"
#include "kmkernel.h"
#include "broadcaststatus.h"
#include "callback.h"
#include "stl_util.h"

#include <kabc/stdaddressbook.h>
#include <kabc/addressee.h>
#include <dcopclient.h>
#include <kstandarddirs.h>
#include <kurldrag.h>
#include <kimproxy.h>
#include <kpopupmenu.h>
#include <krun.h>
#include <kurl.h>

#include <qclipboard.h>

#include <algorithm>
using std::for_each;
using std::remove;
using std::find;

KMail::URLHandlerManager * KMail::URLHandlerManager::self = 0;

namespace {
  class KMailProtocolURLHandler : public KMail::URLHandler {
  public:
    KMailProtocolURLHandler() : KMail::URLHandler() {}
    ~KMailProtocolURLHandler() {}

    bool handleClick( const KURL &, KMReaderWin * ) const;
    bool handleContextMenuRequest( const KURL & url, const QPoint &, KMReaderWin * ) const {
      return url.protocol() == "kmail";
    }
    QString statusBarMessage( const KURL &, KMReaderWin * ) const;
  };

  class ExpandCollapseQuoteURLManager : public KMail::URLHandler {
  public:
    ExpandCollapseQuoteURLManager() : KMail::URLHandler() {}
    ~ExpandCollapseQuoteURLManager() {}

    bool handleClick( const KURL &, KMReaderWin * ) const;
    bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const {
      return false;
    }
    QString statusBarMessage( const KURL &, KMReaderWin * ) const;

  };

  class SMimeURLHandler : public KMail::URLHandler {
  public:
    SMimeURLHandler() : KMail::URLHandler() {}
    ~SMimeURLHandler() {}

    bool handleClick( const KURL &, KMReaderWin * ) const;
    bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const {
      return false;
    }
    QString statusBarMessage( const KURL &, KMReaderWin * ) const;
  };

  class MailToURLHandler : public KMail::URLHandler {
  public:
    MailToURLHandler() : KMail::URLHandler() {}
    ~MailToURLHandler() {}

    bool handleClick( const KURL &, KMReaderWin * ) const { return false; }
    bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const {
      return false;
    }
    QString statusBarMessage( const KURL &, KMReaderWin * ) const;
  };

  class ContactUidURLHandler : public KMail::URLHandler {
  public:
    ContactUidURLHandler() : KMail::URLHandler() {}
    ~ContactUidURLHandler() {}

    bool handleClick( const KURL &, KMReaderWin * ) const;
    bool handleContextMenuRequest( const KURL &url, const QPoint &p, KMReaderWin * ) const;
    QString statusBarMessage( const KURL &, KMReaderWin * ) const;
  };

  class HtmlAnchorHandler : public KMail::URLHandler {
  public:
    HtmlAnchorHandler() : KMail::URLHandler() {}
    ~HtmlAnchorHandler() {}

    bool handleClick( const KURL &, KMReaderWin * ) const;
    bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const {
      return false;
    }
    QString statusBarMessage( const KURL &, KMReaderWin * ) const { return QString::null; }
  };

  class AttachmentURLHandler : public KMail::URLHandler {
  public:
    AttachmentURLHandler() : KMail::URLHandler() {}
    ~AttachmentURLHandler() {}

    bool handleClick( const KURL &, KMReaderWin * ) const;
    bool handleShiftClick( const KURL &url, KMReaderWin *window ) const;
    bool handleDrag( const KURL &url, const QString& imagePath, KMReaderWin *window ) const;
    bool willHandleDrag( const KURL &url, const QString& imagePath, KMReaderWin *window ) const;
    bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const;
    QString statusBarMessage( const KURL &, KMReaderWin * ) const;
  private:
    partNode* partNodeForUrl( const KURL &url, KMReaderWin *w ) const;
    bool attachmentIsInHeader( const KURL &url ) const;
  };

  class ShowAuditLogURLHandler : public KMail::URLHandler {
  public:
      ShowAuditLogURLHandler() : KMail::URLHandler() {}
      ~ShowAuditLogURLHandler() {}

      bool handleClick( const KURL &, KMReaderWin * ) const;
      bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const;
      QString statusBarMessage( const KURL &, KMReaderWin * ) const;
  };

  // Handler that prevents dragging of internal images added by KMail, such as the envelope image
  // in the enterprise header
  class InternalImageURLHandler : public KMail::URLHandler {
  public:
      InternalImageURLHandler() : KMail::URLHandler()
        {}
      ~InternalImageURLHandler()
        {}
      bool handleDrag( const KURL &url, const QString& imagePath, KMReaderWin *window ) const;
      bool willHandleDrag( const KURL &url, const QString& imagePath, KMReaderWin *window ) const;
      bool handleClick( const KURL &, KMReaderWin * ) const
        { return false; }
      bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const
        { return false; }
      QString statusBarMessage( const KURL &, KMReaderWin * ) const
        { return QString(); }
  };

  class FallBackURLHandler : public KMail::URLHandler {
  public:
    FallBackURLHandler() : KMail::URLHandler() {}
    ~FallBackURLHandler() {}

    bool handleClick( const KURL &, KMReaderWin * ) const;
    bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const;
    QString statusBarMessage( const KURL & url, KMReaderWin * ) const {
      return url.prettyURL();
    }
  };

} // anon namespace


//
//
// BodyPartURLHandlerManager
//
//

class KMail::URLHandlerManager::BodyPartURLHandlerManager : public KMail::URLHandler {
public:
  BodyPartURLHandlerManager() : KMail::URLHandler() {}
  ~BodyPartURLHandlerManager();

  bool handleClick( const KURL &, KMReaderWin * ) const;
  bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const;
  QString statusBarMessage( const KURL &, KMReaderWin * ) const;

  void registerHandler( const Interface::BodyPartURLHandler * handler );
  void unregisterHandler( const Interface::BodyPartURLHandler * handler );

private:
  typedef QValueVector<const Interface::BodyPartURLHandler*> BodyPartHandlerList;
  BodyPartHandlerList mHandlers;
};

KMail::URLHandlerManager::BodyPartURLHandlerManager::~BodyPartURLHandlerManager() {
  for_each( mHandlers.begin(), mHandlers.end(),
	    DeleteAndSetToZero<Interface::BodyPartURLHandler>() );
}

void KMail::URLHandlerManager::BodyPartURLHandlerManager::registerHandler( const Interface::BodyPartURLHandler * handler ) {
  if ( !handler )
    return;
  unregisterHandler( handler ); // don't produce duplicates
  mHandlers.push_back( handler );
}

void KMail::URLHandlerManager::BodyPartURLHandlerManager::unregisterHandler( const Interface::BodyPartURLHandler * handler ) {
  // don't delete them, only remove them from the list!
  mHandlers.erase( remove( mHandlers.begin(), mHandlers.end(), handler ), mHandlers.end() );
}

static partNode * partNodeFromXKMailUrl( const KURL & url, KMReaderWin * w, QString * path ) {
  assert( path );

  if ( !w || url.protocol() != "x-kmail" )
    return 0;
  const QString urlPath = url.path();

  // urlPath format is: /bodypart/<random number>/<part id>/<path>

  kdDebug( 5006 ) << "BodyPartURLHandler: urlPath == \"" << urlPath << "\"" << endl;
  if ( !urlPath.startsWith( "/bodypart/" ) )
    return 0;

  const QStringList urlParts = QStringList::split( '/', urlPath.mid( 10 ), true );
  if ( urlParts.size() != 3 )
    return 0;
  bool ok = false;
  const int part_id = urlParts[1].toInt( &ok );
  if ( !ok )
    return 0;
  *path = KURL::decode_string( urlParts[2] );
  return w->partNodeForId( part_id );
}

bool KMail::URLHandlerManager::BodyPartURLHandlerManager::handleClick( const KURL & url, KMReaderWin * w ) const {
  QString path;
  partNode * node = partNodeFromXKMailUrl( url, w, &path );
  if ( !node )
    return false;
  KMMessage *msg = w->message();
  if ( !msg ) return false;
  Callback callback( msg, w );
  KMail::PartNodeBodyPart part( *node, w->overrideCodec() );
  for ( BodyPartHandlerList::const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it )
    if ( (*it)->handleClick( &part, path, callback ) )
      return true;
  return false;
}

bool KMail::URLHandlerManager::BodyPartURLHandlerManager::handleContextMenuRequest( const KURL & url, const QPoint & p, KMReaderWin * w ) const {
  QString path;
  partNode * node = partNodeFromXKMailUrl( url, w, &path );
  if ( !node )
    return false;

  KMail::PartNodeBodyPart part( *node, w->overrideCodec() );
  for ( BodyPartHandlerList::const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it )
    if ( (*it)->handleContextMenuRequest( &part, path, p ) )
      return true;
  return false;
}

QString KMail::URLHandlerManager::BodyPartURLHandlerManager::statusBarMessage( const KURL & url, KMReaderWin * w ) const {
  QString path;
  partNode * node = partNodeFromXKMailUrl( url, w, &path );
  if ( !node )
    return QString::null;

  KMail::PartNodeBodyPart part( *node, w->overrideCodec() );
  for ( BodyPartHandlerList::const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it ) {
    const QString msg = (*it)->statusBarMessage( &part, path );
    if ( !msg.isEmpty() )
      return msg;
  }
  return QString::null;
}

//
//
// URLHandlerManager
//
//

KMail::URLHandlerManager::URLHandlerManager() {
  registerHandler( new KMailProtocolURLHandler() );
  registerHandler( new ExpandCollapseQuoteURLManager() );
  registerHandler( new SMimeURLHandler() );
  registerHandler( new MailToURLHandler() );
  registerHandler( new ContactUidURLHandler() );
  registerHandler( new HtmlAnchorHandler() );
  registerHandler( new AttachmentURLHandler() );
  registerHandler( mBodyPartURLHandlerManager = new BodyPartURLHandlerManager() );
  registerHandler( new ShowAuditLogURLHandler() );
  registerHandler( new InternalImageURLHandler );
  registerHandler( new FallBackURLHandler() );
}

KMail::URLHandlerManager::~URLHandlerManager() {
  for_each( mHandlers.begin(), mHandlers.end(),
	    DeleteAndSetToZero<URLHandler>() );
}

void KMail::URLHandlerManager::registerHandler( const URLHandler * handler ) {
  if ( !handler )
    return;
  unregisterHandler( handler ); // don't produce duplicates
  mHandlers.push_back( handler );
}

void KMail::URLHandlerManager::unregisterHandler( const URLHandler * handler ) {
  // don't delete them, only remove them from the list!
  mHandlers.erase( remove( mHandlers.begin(), mHandlers.end(), handler ), mHandlers.end() );
}

void KMail::URLHandlerManager::registerHandler( const Interface::BodyPartURLHandler * handler ) {
  if ( mBodyPartURLHandlerManager )
    mBodyPartURLHandlerManager->registerHandler( handler );
}

void KMail::URLHandlerManager::unregisterHandler( const Interface::BodyPartURLHandler * handler ) {
  if ( mBodyPartURLHandlerManager )
    mBodyPartURLHandlerManager->unregisterHandler( handler );
}

bool KMail::URLHandlerManager::handleClick( const KURL & url, KMReaderWin * w ) const {
  for ( HandlerList::const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it )
    if ( (*it)->handleClick( url, w ) )
      return true;
  return false;
}

bool KMail::URLHandlerManager::handleShiftClick( const KURL &url, KMReaderWin *window ) const
{
  for ( HandlerList::const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it )
    if ( (*it)->handleShiftClick( url, window ) )
      return true;
  return false;
}

bool KMail::URLHandlerManager::willHandleDrag( const KURL &url, const QString& imagePath,
                                               KMReaderWin *window ) const
{
  for ( HandlerList::const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it )
    if ( (*it)->willHandleDrag( url, imagePath, window ) )
      return true;
  return false;
}

bool KMail::URLHandlerManager::handleDrag( const KURL &url, const QString& imagePath,
                                           KMReaderWin *window ) const
{
  for ( HandlerList::const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it )
    if ( (*it)->handleDrag( url, imagePath, window ) )
      return true;
  return false;
}

bool KMail::URLHandlerManager::handleContextMenuRequest( const KURL & url, const QPoint & p, KMReaderWin * w ) const {
  for ( HandlerList::const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it )
    if ( (*it)->handleContextMenuRequest( url, p, w ) )
      return true;
  return false;
}

QString KMail::URLHandlerManager::statusBarMessage( const KURL & url, KMReaderWin * w ) const {
  for ( HandlerList::const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it ) {
    const QString msg = (*it)->statusBarMessage( url, w );
    if ( !msg.isEmpty() )
      return msg;
  }
  return QString::null;
}


//
//
// URLHandler
//
//

// these includes are temporary and should not be needed for the code
// above this line, so they appear only here:
#include "kmmessage.h"
#include "kmreaderwin.h"
#include "partNode.h"
#include "kmmsgpart.h"

#include <ui/messagebox.h>

#include <klocale.h>
#include <kprocess.h>
#include <kmessagebox.h>
#include <khtml_part.h>

#include <qstring.h>

namespace {
  bool KMailProtocolURLHandler::handleClick( const KURL & url, KMReaderWin * w ) const {
    if ( url.protocol() == "kmail" ) {
      if ( !w )
        return false;

      if ( url.path() == "showHTML" ) {
        w->setHtmlOverride( !w->htmlOverride() );
        w->update( true );
        return true;
      }

      if ( url.path() == "loadExternal" ) {
        w->setHtmlLoadExtOverride( !w->htmlLoadExtOverride() );
        w->update( true );
        return true;
      }

      if ( url.path() == "goOnline" ) {
        kmkernel->resumeNetworkJobs();
        return true;
      }

      if ( url.path() == "decryptMessage" ) {
        w->setDecryptMessageOverwrite( true );
        w->update( true );
        return true;
      }

      if ( url.path() == "showSignatureDetails" ) {
        w->setShowSignatureDetails( true );
        w->update( true );
        return true;
      }

      if ( url.path() == "hideSignatureDetails" ) {
        w->setShowSignatureDetails( false );
        w->update( true );
        return true;
      }

      if ( url.path() == "showAttachmentQuicklist" ) {
	  w->saveRelativePosition();
	  w->setShowAttachmentQuicklist( true );
	  w->update( true );
	  return true;
      }

      if ( url.path() == "hideAttachmentQuicklist" ) {
	  w->saveRelativePosition();
	  w->setShowAttachmentQuicklist( false );
	  w->update( true );
	  return true;
      }

      if ( url.path() == "showRawToltecMail" ) {
        w->saveRelativePosition();
        w->setShowRawToltecMail( true );
        w->update( true );
        return true;
      }

//       if ( url.path() == "startIMApp" )
//       {
//         kmkernel->imProxy()->startPreferredApp();
//         return true;
//       }
//       //FIXME: handle startIMApp urls in their own handler, or rename this one
    }
    return false;
  }

  QString KMailProtocolURLHandler::statusBarMessage( const KURL & url, KMReaderWin * ) const {
    if ( url.protocol() == "kmail" )
    {
      if ( url.path() == "showHTML" )
        return i18n("Turn on HTML rendering for this message.");
      if ( url.path() == "loadExternal" )
        return i18n("Load external references from the Internet for this message.");
      if ( url.path() == "goOnline" )
        return i18n("Work online.");
      if ( url.path() == "decryptMessage" )
        return i18n("Decrypt message.");
      if ( url.path() == "showSignatureDetails" )
        return i18n("Show signature details.");
      if ( url.path() == "hideSignatureDetails" )
        return i18n("Hide signature details.");
      if ( url.path() == "hideAttachmentQuicklist" )
        return i18n( "Hide attachment list" );
      if ( url.path() == "showAttachmentQuicklist" )
        return i18n( "Show attachment list" );
    }
    return QString::null ;
  }
}

namespace {

  bool ExpandCollapseQuoteURLManager::handleClick(
      const KURL & url, KMReaderWin * w ) const
  {
    //  kmail:levelquote/?num      -> the level quote to collapse.
    //  kmail:levelquote/?-num      -> expand all levels quote.
    if ( url.protocol() == "kmail" && url.path()=="levelquote" )
    {
      QString levelStr= url.query().mid( 1,url.query().length() );
      bool isNumber;
      int levelQuote= levelStr.toInt(&isNumber);
      if ( isNumber )
        w->slotLevelQuote( levelQuote );
      return true;
    }
    return false;
  }
  QString ExpandCollapseQuoteURLManager::statusBarMessage(
      const KURL & url, KMReaderWin * ) const
  {
      if ( url.protocol() == "kmail" && url.path() == "levelquote" )
      {
        QString query= url.query();
        if ( query.length()>=2 ) {
          if ( query[ 1 ] =='-'  ) {
            return i18n("Expand all quoted text.");
          }
          else {
            return i18n("Collapse quoted text.");
          }
        }
      }
      return QString::null ;
  }

}

// defined in kmreaderwin.cpp...
extern bool foundSMIMEData( const QString aUrl, QString & displayName,
			    QString & libName, QString & keyId );

namespace {
  bool SMimeURLHandler::handleClick( const KURL & url, KMReaderWin * w ) const {
    if ( !url.hasRef() )
      return false;
    QString displayName, libName, keyId;
    if ( !foundSMIMEData( url.path() + '#' + url.ref(), displayName, libName, keyId ) )
      return false;
    KProcess cmp;
    cmp << "kleopatra" << "-query" << keyId;
    if ( !cmp.start( KProcess::DontCare ) )
      KMessageBox::error( w, i18n("Could not start certificate manager. "
				  "Please check your installation."),
			  i18n("KMail Error") );
    return true;
  }

  QString SMimeURLHandler::statusBarMessage( const KURL & url, KMReaderWin * ) const {
    QString displayName, libName, keyId;
    if ( !foundSMIMEData( url.path() + '#' + url.ref(), displayName, libName, keyId ) )
      return QString::null;
    return i18n("Show certificate 0x%1").arg( keyId );
  }
}

namespace {
  bool HtmlAnchorHandler::handleClick( const KURL & url, KMReaderWin * w ) const {
    if ( url.hasHost() || url.path() != "/" || !url.hasRef() )
      return false;
    if ( w && !w->htmlPart()->gotoAnchor( url.ref() ) )
      static_cast<QScrollView*>( w->htmlPart()->widget() )->ensureVisible( 0, 0 );
    return true;
  }
}

namespace {
  QString MailToURLHandler::statusBarMessage( const KURL & url, KMReaderWin * ) const {
    if ( url.protocol() != "mailto" )
      return QString::null;
    return KMMessage::decodeMailtoUrl( url.url() );
  }
}

namespace {
  static QString searchFullEmailByUid( const QString &uid )
  {
    QString fullEmail;
    KABC::AddressBook *add_book = KABC::StdAddressBook::self( true );
    KABC::Addressee o = add_book->findByUid( uid );
    if ( !o.isEmpty() ) {
      fullEmail = o.fullEmail();
    }
    return fullEmail;
  }

  static void runKAddressBook( const KURL &url )
  {
    QString uid = url.path();
    DCOPClient *client = KApplication::kApplication()->dcopClient();
    const QByteArray noParamData;
    const QByteArray paramData;
    QByteArray replyData;
    QCString replyTypeStr;
    bool foundAbbrowser = client->call( "kaddressbook", "KAddressBookIface",
                                        "interfaces()",  noParamData,
                                        replyTypeStr, replyData );
    if ( foundAbbrowser ) {
      // KAddressbook is already running, so just DCOP to it to bring up the contact editor
      DCOPRef kaddressbook( "kaddressbook", "KAddressBookIface" );
      kaddressbook.send( "showContactEditor", uid );
    } else {
      // KaddressBook is not already running.
      // Pass it the UID of the contact via the command line while starting it - its neater.
      // We start it without its main interface
      QString iconPath = KGlobal::iconLoader()->iconPath( "go", KIcon::Small );
      QString tmpStr = "kaddressbook --editor-only --uid ";
      tmpStr += KProcess::quote( uid );
      KRun::runCommand( tmpStr, "KAddressBook", iconPath );
    }
  }

  bool ContactUidURLHandler::handleClick( const KURL &url, KMReaderWin * ) const
  {
    if ( url.protocol() != "uid" ) {
      return false;
    } else {
      runKAddressBook( url );
      return true;
    }
  }

  bool ContactUidURLHandler::handleContextMenuRequest( const KURL &url, const QPoint &p,
                                                       KMReaderWin * ) const
  {
    if ( url.protocol() != "uid" || url.path().isEmpty() ) {
      return false;
    }

    KPopupMenu *menu = new KPopupMenu();
    menu->insertItem( i18n( "&Open in Address Book" ), 0 );
    menu->insertItem( i18n( "&Copy Email Address" ), 1 );

    switch( menu->exec( p, 0 ) ) {
    case 0: // open
      runKAddressBook( url );
      break;
    case 1: // copy
    {
      const QString fullEmail = searchFullEmailByUid( url.path() );
      if ( !fullEmail.isEmpty() ) {
        QClipboard *clip = QApplication::clipboard();
        clip->setSelectionMode( true );
        clip->setText( fullEmail );
        clip->setSelectionMode( false );
        clip->setText( fullEmail );
        KPIM::BroadcastStatus::instance()->setStatusMsg( i18n( "Address copied to clipboard." ) );
      }
      break;
    }
    default:
      break;
    }
    return true;
  }

  QString ContactUidURLHandler::statusBarMessage( const KURL &url, KMReaderWin * ) const
  {
    if ( url.protocol() != "uid" ) {
      return QString::null;
    } else {
      return i18n( "Lookup the contact in KAddressbook" );
    }
  }
}

namespace {

  partNode* AttachmentURLHandler::partNodeForUrl( const KURL &url, KMReaderWin *w ) const
  {
    if ( !w || !w->message() )
      return 0;
    if ( url.protocol() != "attachment" )
      return 0;

    bool ok;
    int nodeId = url.path().toInt( &ok );
    if ( !ok )
      return 0;

    partNode * node = w->partNodeForId( nodeId );
    return node;
  }

  bool AttachmentURLHandler::attachmentIsInHeader( const KURL &url ) const
  {
    bool inHeader = false;
    const QString place = url.queryItem( "place" ).lower();
    if ( place != QString::null ) {
      inHeader = ( place == "header" );
    }
    return inHeader;
  }

  bool AttachmentURLHandler::handleClick( const KURL & url, KMReaderWin * w ) const
  {
    partNode * node = partNodeForUrl( url, w );
    if ( !node )
      return false;

    const bool inHeader = attachmentIsInHeader( url );
    const bool shouldShowDialog = !node->isDisplayedEmbedded() || !inHeader;
    if ( inHeader )
      w->scrollToAttachment( node );
    if ( shouldShowDialog )
      w->openAttachment( node->nodeId(), w->tempFileUrlFromPartNode( node ).path() );
    return true;
  }

  bool AttachmentURLHandler::handleShiftClick( const KURL &url, KMReaderWin *window ) const
  {
    partNode * node = partNodeForUrl( url, window );
    if ( !node )
      return false;
    if ( !window )
      return false;
    window->saveAttachment( window->tempFileUrlFromPartNode( node ) );
    return true;
  }

  bool AttachmentURLHandler::willHandleDrag( const KURL &url, const QString& imagePath,
                                             KMReaderWin *window ) const
  {
    Q_UNUSED( imagePath );
    return partNodeForUrl( url, window ) != 0;
  }

  bool AttachmentURLHandler::handleDrag( const KURL &url, const QString& imagePath,
                                         KMReaderWin *window ) const
  {
    Q_UNUSED( imagePath );
    const partNode * node = partNodeForUrl( url, window );
    if ( !node )
      return false;

    KURL file = window->tempFileUrlFromPartNode( node ).path();
    if ( !file.isEmpty() ) {
      QString icon = node->msgPart().iconName( KIcon::Small );
      KURLDrag* urlDrag = new KURLDrag( file, window );
      if ( !icon.isEmpty() ) {
        QPixmap iconMap( icon );
        urlDrag->setPixmap( iconMap );
      }
      urlDrag->drag();
      return true;
    }
    else {
      return false;
    }
  }

  bool AttachmentURLHandler::handleContextMenuRequest( const KURL & url, const QPoint & p, KMReaderWin * w ) const
  {
    partNode * node = partNodeForUrl( url, w );
    if ( !node )
      return false;

    w->showAttachmentPopup( node->nodeId(), w->tempFileUrlFromPartNode( node ).path(), p );
    return true;
  }

  QString AttachmentURLHandler::statusBarMessage( const KURL & url, KMReaderWin * w ) const
  {
    partNode * node = partNodeForUrl( url, w );
    if ( !node )
      return QString::null;

    const KMMessagePart & msgPart = node->msgPart();
    QString name = msgPart.fileName();
    if ( name.isEmpty() )
      name = msgPart.name();
    if ( !name.isEmpty() )
      return i18n( "Attachment: %1" ).arg( name );
    return i18n( "Attachment #%1 (unnamed)" ).arg( KMReaderWin::msgPartFromUrl( url ) );
  }
}

namespace {
  static QString extractAuditLog( const KURL & url ) {
    if ( url.protocol() != "kmail" || url.path() != "showAuditLog" )
      return QString();
    assert( !url.queryItem( "log" ).isEmpty() );
    return url.queryItem( "log" );
  }

  bool ShowAuditLogURLHandler::handleClick( const KURL & url, KMReaderWin * w ) const {
    const QString auditLog = extractAuditLog( url );
    if ( auditLog.isEmpty() )
        return false;
    Kleo::MessageBox::auditLog( w, auditLog );
    return true;
  }

  bool ShowAuditLogURLHandler::handleContextMenuRequest( const KURL & url, const QPoint &, KMReaderWin * w ) const
  {
    Q_UNUSED( w );
    // disable RMB for my own links:
    return !extractAuditLog( url ).isEmpty();
  }

  QString ShowAuditLogURLHandler::statusBarMessage( const KURL & url, KMReaderWin * ) const {
    if ( extractAuditLog( url ).isEmpty() )
      return QString();
    else
      return i18n("Show GnuPG Audit Log for this operation");
  }
}

namespace {
  bool InternalImageURLHandler::handleDrag( const KURL &url, const QString& imagePath,
                                            KMReaderWin *window ) const
  {
    Q_UNUSED( window );
    Q_UNUSED( url );
    const QString kmailImagePath = locate( "data", "kmail/pics/" );
    if ( imagePath.contains( kmailImagePath ) ) {
      // Do nothing, don't start a drag
      return true;
    }
    return false;
  }

  bool InternalImageURLHandler::willHandleDrag( const KURL &url, const QString& imagePath,
                                                KMReaderWin *window ) const
  {
    Q_UNUSED( window );
    Q_UNUSED( url );
    const QString kmailImagePath = locate( "data", "kmail/pics/" );
    return imagePath.contains( kmailImagePath );
  }
}

namespace {
  bool FallBackURLHandler::handleClick( const KURL & url, KMReaderWin * w ) const {
    if ( w )
      w->emitUrlClicked( url, Qt::LeftButton );
    return true;
  }

  bool FallBackURLHandler::handleContextMenuRequest( const KURL & url, const QPoint & p, KMReaderWin * w ) const {
    if ( w )
      w->emitPopupMenu( url, p );
    return true;
  }
}
