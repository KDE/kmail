/*******************************************************************************
**
** Filename   : util
** Created on : 03 April, 2005
** Copyright  : (c) 2005 Till Adam
** Email      : <adam@kde.org>
**
*******************************************************************************/

/*******************************************************************************
**
**   This program is free software; you can redistribute it and/or modify
**   it under the terms of the GNU General Public License as published by
**   the Free Software Foundation; either version 2 of the License, or
**   (at your option) any later version.
**
**   It is distributed in the hope that it will be useful, but
**   WITHOUT ANY WARRANTY; without even the implied warranty of
**   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
**   General Public License for more details.
**
**   You should have received a copy of the GNU General Public License
**   along with this program; if not, write to the Free Software
**   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
**   In addition, as a special exception, the copyright holders give
**   permission to link the code of this program with any edition of
**   the Qt library by Trolltech AS, Norway (or with modified versions
**   of Qt that use the same license as Qt), and distribute linked
**   combinations including the two.  You must obey the GNU General
**   Public License in all respects for all of the code used other than
**   Qt.  If you modify this file, you may extend this exception to
**   your version of the file, but you are not obligated to do so.  If
**   you do not wish to do so, delete this exception statement from
**   your version.
**
*******************************************************************************/


#include "util.h"
#include "imapsettings.h"
#include "kmkernel.h"

#include "messagecore/stringutil.h"
#include "messagecomposer/messagehelper.h"



#include <kmime/kmime_message.h>
#include <kpimutils/email.h>
#include <kimap/loginjob.h>
#include <mailtransport/transport.h>
#include <Akonadi/AgentManager>
#include <Akonadi/EntityTreeModel>
#include <akonadi/entitymimetypefiltermodel.h>

#include <KStandardDirs>
#include <kascii.h>
#include <KCharsets>
#include <KJob>
#include <kio/jobuidelegate.h>


#include <stdlib.h>

#include "foldercollection.h"

using namespace MailCommon;


KMime::Types::Mailbox::List KMail::Util::mailingListsFromMessage( const Akonadi::Item& item )
{
  KMime::Types::Mailbox::List addresses;
  // determine the mailing list posting address
  Akonadi::Collection parentCollection = item.parentCollection();
  QSharedPointer<FolderCollection> fd;
  if ( parentCollection.isValid() ) {
    fd = FolderCollection::forCollection( parentCollection );
    if ( fd->isMailingListEnabled() && !fd->mailingListPostAddress().isEmpty() ) {
      addresses << MessageCore::StringUtil::mailboxFromUnicodeString( fd->mailingListPostAddress() );
    }
  }

  return addresses;
}

Akonadi::Item::Id KMail::Util::putRepliesInSameFolder( const Akonadi::Item& item )
{
  Akonadi::Collection parentCollection = item.parentCollection();
  QSharedPointer<FolderCollection> fd;
  if ( parentCollection.isValid() ) {
    fd = FolderCollection::forCollection( parentCollection );
    if( fd->putRepliesInSameFolder() ) {
      return parentCollection.id();
    }
  }
  return -1;
}

void KMail::Util::launchAccountWizard( QWidget *w )
{
  QStringList lst;
  lst.append( "--type" );
  lst.append( "message/rfc822" );

  const QString path = KStandardDirs::findExe( QLatin1String("accountwizard" ) );
  if( !QProcess::startDetached( path, lst ) )
    KMessageBox::error( w, i18n( "Could not start the account wizard. "
                                 "Please check your installation." ),
                        i18n( "Unable to start account wizard" ) );

}

void KMail::Util::handleClickedURL( const KUrl &url, uint identity )
{
  if ( url.protocol() == "mailto" )
  {
    KMime::Message::Ptr msg ( new KMime::Message );
    MessageHelper::initHeader( msg, KMKernel::self()->identityManager(), identity );
    msg->contentType()->setCharset("utf-8");

    QMap<QString, QString> fields =  MessageCore::StringUtil::parseMailtoUrl( url );

    msg->to()->fromUnicodeString( fields.value( "to" ),"utf-8" );
    if ( !fields.value( "subject" ).isEmpty() )
      msg->subject()->fromUnicodeString( fields.value( "subject" ),"utf-8" );
    if ( !fields.value( "body" ).isEmpty() )
      msg->setBody( fields.value( "body" ).toUtf8() );
    if ( !fields.value( "cc" ).isEmpty() )
      msg->cc()->fromUnicodeString( fields.value( "cc" ),"utf-8" );

    KMail::Composer * win = KMail::makeComposer( msg, KMail::Composer::New, identity );
    win->setFocusToSubject();
    win->show();
  } else {
    kWarning() << "Can't handle URL:" << url;
  }
}

