/*  -*- c++ -*-
    urlhandlermanager.cpp

    This file is part of KMail, the KDE mail client.
    Copyright (c) 2002-2003 Klarälvdalens Datakonsult AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "urlhandlermanager.h"

#include "interfaces/urlhandler.h"

#include <kurl.h>

#include <algorithm>
using std::for_each;
using std::remove;
using std::find;

KMail::URLHandlerManager * KMail::URLHandlerManager::self = 0;

namespace {
  class ShowHtmlSwitchURLHandler : public KMail::URLHandler {
  public:
    ShowHtmlSwitchURLHandler() : KMail::URLHandler() {}
    ~ShowHtmlSwitchURLHandler() {}

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

  class GroupwareURLHandler : public KMail::URLHandler {
  public:
    GroupwareURLHandler() : KMail::URLHandler() {}
    ~GroupwareURLHandler() {}

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

  class FallBackURLHandler : public KMail::URLHandler {
  public:
    FallBackURLHandler() : KMail::URLHandler() {}
    ~FallBackURLHandler() {}

    bool handleClick( const KURL &, KMReaderWin * ) const;
    bool handleContextMenuRequest( const KURL &, const QPoint &, KMReaderWin * ) const;
    QString statusBarMessage( const KURL & url, KMReaderWin * ) const {
      return url.url();
    }
  };


}

KMail::URLHandlerManager::URLHandlerManager() {
  registerHandler( new ShowHtmlSwitchURLHandler() );
  registerHandler( new SMimeURLHandler() );
  registerHandler( new GroupwareURLHandler() );
  registerHandler( new MailToURLHandler() );
  registerHandler( new HtmlAnchorHandler() );
  registerHandler( new AttachmentURLHandler() );
  registerHandler( new FallBackURLHandler() );
}

namespace {
  template <typename T> struct Delete {
    void operator()( const T * x ) { delete x; x = 0; }
  };
}

KMail::URLHandlerManager::~URLHandlerManager() {
  for_each( mHandlers.begin(), mHandlers.end(), Delete<URLHandler>() );
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

bool KMail::URLHandlerManager::handleClick( const KURL & url, KMReaderWin * w ) const {
  for ( const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it )
    if ( (*it)->handleClick( url, w ) )
      return true;
  return false;
}

bool KMail::URLHandlerManager::handleContextMenuRequest( const KURL & url, const QPoint & p, KMReaderWin * w ) const {
  for ( const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it )
    if ( (*it)->handleContextMenuRequest( url, p, w ) )
      return true;
  return false;
}

QString KMail::URLHandlerManager::statusBarMessage( const KURL & url, KMReaderWin * w ) const {
  for ( const_iterator it = mHandlers.begin() ; it != mHandlers.end() ; ++it ) {
    const QString msg = (*it)->statusBarMessage( url, w );
    if ( !msg.isEmpty() )
      return msg;
  }
  return QString::null;
}

// these includes are temporary and should not be needed for the code
// above this line, so they appear only here:
#include "kmgroupware.h"
#include "kmmessage.h"
#include "kmkernel.h"
#include "kmreaderwin.h"
#include "partNode.h"
#include "kmmsgpart.h"

#include <klocale.h>
#include <kprocess.h>
#include <kmessagebox.h>
#include <khtml_part.h>

#include <qstring.h>

namespace {
  bool ShowHtmlSwitchURLHandler::handleClick( const KURL & url, KMReaderWin * w ) const {
    if ( url.protocol() != "kmail" || url.path() != "showHTML" )
      return false;
    if ( w ) {
      w->setHtmlOverride( !w->htmlOverride() );
      w->update( true );
    }
    return true;
  }

  QString ShowHtmlSwitchURLHandler::statusBarMessage( const KURL & url, KMReaderWin * ) const {
    return url.url() == "kmail:showHTML"
      ? i18n("Turn on HTML rendering for this message.")
      : QString::null ;
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
    cmp << "kgpgcertmanager" << displayName << libName << "-query" << keyId;
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
  bool GroupwareURLHandler::handleClick( const KURL & url, KMReaderWin * w ) const {
    if ( !kmkernel->groupware().isEnabled() )
      return false;
    return !w || kmkernel->groupware().handleLink( url, w->message() );
  }

  QString GroupwareURLHandler::statusBarMessage( const KURL & url, KMReaderWin * ) const {
    QString type, action, action2, dummy;
    if ( !KMGroupware::foundGroupwareLink( url.url(), type, action, action2, dummy ) )
      return QString::null;
    QString result = type + ' ' + action;
    if ( !action2.isEmpty() )
      result += ' ' + action2;
    return i18n("Groupware: \"%1\"").arg( result );
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
