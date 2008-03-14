/*  -*- c++ -*-
    urlhandlermanager.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002-2003 Klar�vdalens Datakonsult AB
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
#include "callback.h"

#include <kimproxy.h>
#include "stl_util.h"
#include <kurl.h>

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
    bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const;
    QString statusBarMessage( const KURL &, KMReaderWin * ) const;
  };

  class ShowAuditLogURLHandler : public KMail::URLHandler {
  public:
      ShowAuditLogURLHandler() : KMail::URLHandler() {}
      ~ShowAuditLogURLHandler() {}

      bool handleClick( const KURL &, KMReaderWin * ) const;
      bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const;
      QString statusBarMessage( const KURL &, KMReaderWin * ) const;
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
  *path = KURL::decode_string( urlParts[2], 106 );
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
  registerHandler( new HtmlAnchorHandler() );
  registerHandler( new AttachmentURLHandler() );
  registerHandler( mBodyPartURLHandlerManager = new BodyPartURLHandlerManager() );
  registerHandler( new ShowAuditLogURLHandler() );
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
        if ( query.length()>=2 )
          if ( query[ 1 ] =='-'  )
            return i18n("Expand all quoted text.");
          else
            return i18n("Collapse quoted text.");
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
  bool AttachmentURLHandler::handleClick( const KURL & url, KMReaderWin * w ) const {
    if ( !w || !w->message() )
      return false;
    const int id = KMReaderWin::msgPartFromUrl( url );
    if ( id <= 0 )
      return false;
    w->openAttachment( id, url.path() );
    return true;
  }

  bool AttachmentURLHandler::handleContextMenuRequest( const KURL & url, const QPoint & p, KMReaderWin * w ) const {
    if ( !w || !w->message() )
      return false;
    const int id = KMReaderWin::msgPartFromUrl( url );
    if ( id <= 0 )
      return false;
    w->showAttachmentPopup( id, url.path(), p );
    return true;
  }

  QString AttachmentURLHandler::statusBarMessage( const KURL & url, KMReaderWin * w ) const {
    if ( !w || !w->message() )
      return QString::null;
    const partNode * node = w->partNodeFromUrl( url );
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

  bool ShowAuditLogURLHandler::handleContextMenuRequest( const KURL & url, const QPoint &, KMReaderWin * w ) const {
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
