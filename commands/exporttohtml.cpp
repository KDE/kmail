/******************************************************************************
 *
 *  Copyright 2008 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#include "commands/exporttohtml.h"

#include "messagetree.h"
#include "kmmessage.h"
#include "kmmsgbase.h"
#include "globalsettings.h"

#include <kmime/kmime_message.h>

#include <KDebug>
#include <KTemporaryFile>
#include <KFileDialog>
#include <KUrl>
#include <KMessageBox>
#include <KRun>

#include <kcursorsaver.h>

#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/netaccess.h>

#include <unistd.h> // link()
#include <errno.h>


namespace KMail
{

namespace Commands
{

ExportToHtml::ExportToHtml( QWidget * parent, MessageTreeCollection * collection, bool openInBrowser )
  : KMCommand( parent, collection->toFlatList() ),
    mMessageTreeCollection( collection ), mOpenInBrowser( openInBrowser )
{
}

ExportToHtml::~ExportToHtml()
{
  delete mMessageTreeCollection;
}

KMMessage * ExportToHtml::findMessageBySerial( const QList< KMMessage * > &messages, unsigned long serial )
{
  for ( QList< KMMessage * >::ConstIterator it = messages.begin(); it != messages.end(); ++it )
  {
    if ( ( *it )->getMsgSerNum() == serial )
      return *it;
  }

  return 0;
}

inline QString fix_string_for_html( const QString &str )
{
  QString ret = str;
  ret.replace( QChar( '<' ), "&lt;" );
  ret.replace( QChar( '>' ), "&gt;" );
  ret.replace( QChar( '	' ), "&nbsp;&nbsp;&nbsp;&nbsp;" ); // tab == 4 spaces! (tm)
  ret.replace( QChar( '\n' ), "<br>" );
  return ret;
}

QString ExportToHtml::getHtmlForTreeList( QList< MessageTree * > * treeList, const QList< KMMessage * > &messages )
{
  QString ret;

  ret += "<div class=\"subtree\">\n";


  for ( QList< MessageTree * >::Iterator it = treeList->begin(); it != treeList->end(); ++it )
  {
    KMMessage * msg = findMessageBySerial( messages, ( *it )->serial() );
    if ( !msg )
    {
      kWarning() << "Lost message in ExportToHtml command!";
      continue;
    }

    KMime::Message mimeMessage;
    mimeMessage.setContent( msg->asString() );
    mimeMessage.parse();

    KMime::Content * body = mimeMessage.mainBodyPart( "text/plain" );
    if ( !body )
    {
      kWarning() << "Could not extract message body";
      continue;
    }

    ret += "<div class=\"headers\">\n";

    ret += "<table>\n";

    if ( KMime::Headers::Subject * subject = mimeMessage.subject() )
    {
      QString str = subject->asUnicodeString();
      if ( !str.isEmpty() )
      {
        ret += "<td><td align=\"right\" valign=\"top\" width=\"50\"><b>Subject:</b></td><td align=\"left\" valign=\"top\">";
        ret += fix_string_for_html( str );
        ret += "</td></tr>\n";
      }
    }

    if ( KMime::Headers::From * from = mimeMessage.from() )
    {
      QString str = from->asUnicodeString();
      if ( !str.isEmpty() )
      {
        ret += "<td><td align=\"right\" valign=\"top\"><b>From:</b></td><td align=\"left\" valign=\"top\">";
        ret += fix_string_for_html( str );
        ret += "</td></tr>\n";
      }
    }

    if ( KMime::Headers::To * to = mimeMessage.to() )
    {
      QString str = to->asUnicodeString();
      if ( !str.isEmpty() )
      {
        ret += "<td><td align=\"right\" valign=\"top\"><b>To:</b></td><td align=\"left\" valign=\"top\">";
        ret += fix_string_for_html( str );
        ret += "</td></tr>\n";
      }
    }

    if ( KMime::Headers::Cc * cc = mimeMessage.cc() )
    {
      QString str = cc->asUnicodeString();
      if ( !str.isEmpty() )
      {
        ret += "<td><td align=\"right\" valign=\"top\"><b>CC:</b></td><td align=\"left\" valign=\"top\">";
        ret += fix_string_for_html( str );
        ret += "</td></tr>\n";
      }
    }

    if ( KMime::Headers::Bcc * bcc = mimeMessage.bcc() )
    {
      QString str = bcc->asUnicodeString();
      if ( !str.isEmpty() )
      {
        ret += "<td><td align=\"right\" valign=\"top\"><b>BCC:</b></td><td align=\"left\" valign=\"top\">";
        ret += fix_string_for_html( str );
        ret += "</td></tr>\n";
      }
    }

    if ( KMime::Headers::Date * date = mimeMessage.date() )
    {
      QString str = date->asUnicodeString();
      if ( !str.isEmpty() )
      {
        ret += "<td><td align=\"right\" valign=\"top\"><b>Date:</b></td><td align=\"left\" valign=\"top\">";
        ret += fix_string_for_html( str );
        ret += "</td></tr>\n";
      }
    }

    ret += "</table>";

    ret += "</div>\n<br>\n";

    ret += "<div class=\"body\">\n";
    ret += fix_string_for_html( body->decodedText() );
    ret += "</div>\n";

    ret += "<br>\n";

    if ( ( *it )->childList() )
    {
      Q_ASSERT( !( *it )->childList()->isEmpty() );

      ret += "<div class=\"childrentree\">\n";
      ret += getHtmlForTreeList( ( *it )->childList(), messages );
      ret += "</div>\n";
    }
  }

  ret += "</div>";

  return ret;
/*

    virtual KMime::Headers::Date *date( bool create = true );
    virtual KMime::Headers::Organization *organization( bool create = true );
    virtual KMime::Headers::ReplyTo *replyTo( bool create = true );
    virtual KMime::Headers::References *references( bool create = true );
    virtual KMime::Headers::UserAgent *userAgent( bool create = true );
    virtual KMime::Headers::InReplyTo *inReplyTo( bool create = true );
    virtual KMime::Headers::Sender *sender( bool create = true );
*/
  
}


KMCommand::Result ExportToHtml::execute()
{
  KCursorSaver busy(KBusyPtr::busy());

  const QList< KMMessage * > messages = retrievedMsgs();

  if ( messages.isEmpty() )
    return Failed;

  QList< KMMessage * >::ConstIterator it;

  // Make sure that all the messages are actually complete
  for ( it = messages.begin(); it != messages.end(); ++it )
  {
    if ( !( *it )->isComplete() )
      return Failed;
  }

  // make sure that we actually have ALL the messages in the tree

  QString html;

  // FIXME: Grab the title from the topmost message if it's a single thread

  html = "<html>\n";
  html += "<head>\n";
  html += "<title></title>\n";
  html += "<style type=\"text/css\">\n";
  html += "div.subtree {\n";
  html += "  border: 1px solid #e0e0e0;\n";
  html += "  padding: 2px;\n";
  html += "}\n";
  html += "div.childrentree {\n";
  html += "  margin-left: 40px;\n";
  html += "}\n";
  html += "div.headers {\n";
  html += "  border: 1px solid #808080;\n";
  html += "  padding: 4px;\n";
  html += "  background-color: #d0d0d0;\n";
  html += "}\n";
  html += "div.body {\n";
  html += "  font-family: Courier New, Fixed;\n";
  html += "}\n";
  html += "</style>\n";
  html += "</head>\n";
  html += "<body>\n";
  
  html += getHtmlForTreeList( mMessageTreeCollection->treeList(), messages );

  html += "</body>\n";
  html += "</html>\n";

  QDataStream ds;
  QFile file;
  KTemporaryFile tf;
  QIODevice * device;

  KUrl url;

  if ( mOpenInBrowser )
  {
    // tmp file for the browser
    tf.open();
    tf.setAutoRemove( false ); // konqueror will remove it
    device = &tf;
    url = KUrl::fromPath( tf.fileName() );

  } else {
    QString fileName = "kmail-message-export.html"; // FIXME: Compute from the first message ?

    url = KFileDialog::getSaveUrl( KUrl::fromPath( fileName ), QString(), parentWidget(), QString() );
    if ( url.isEmpty() )
      return Canceled;

    if ( KIO::NetAccess::exists( url, KIO::NetAccess::DestinationSide, parentWidget() ) )
    {
      if ( KMessageBox::warningContinueCancel(
             parentWidget(),
             i18n( "A file named %1 already exists. Do you want to overwrite it?", url.fileName() ),
             i18n( "File Already Exists" ), KGuiItem( i18n("&Overwrite") ) ) == KMessageBox::Cancel
          )
        return Canceled;
    }


    if ( url.isLocalFile() )
    {
      // save directly
      file.setFileName( url.path() );
      if ( !file.open( QIODevice::WriteOnly ) )
      {
        KMessageBox::error(
            parentWidget(),
            i18nc( "%2 is detailed error description",
              "Could not write the file %1:\n%2",
              file.fileName(),
              QString::fromLocal8Bit( strerror( errno ) )
            ),
            i18n( "KMail Error" )
          );
        return Failed;
      }

      // #79685 by default use the umask the user defined, but let it be configurable
      if ( GlobalSettings::self()->disregardUmask() )
        fchmod( file.handle(), S_IRUSR | S_IWUSR );

      device = &file;
    } else {
      // tmp file for upload
      tf.open();
      device = &tf;
    }
  }

  ds.setDevice( device );

  QByteArray data = html.toUtf8();

  ds.writeRawData( data.data(), data.size() );
  device->close();

  if ( mOpenInBrowser )
  {
    if ( !KRun::runUrl( url, QLatin1String( "text/html" ), parentWidget()->window(), true ) )
    {
      KMessageBox::error( parentWidget(),
          i18n( "Could not run application associated to file %1.", url.path() ),
          i18n( "KMail Error" )
        );
      return Failed;
    }

  } else {
    if ( !url.isLocalFile() )
    {
      if ( !KIO::NetAccess::upload( tf.fileName(), url, parentWidget() ) )
      {
        KMessageBox::error( parentWidget(),
            i18n( "Could not write the file %1.", url.path() ),
            i18n( "KMail Error" )
          );
        return Failed;
      }
    }
  }

  return OK;
}

} // namespace Commands

} // namespace KMail


