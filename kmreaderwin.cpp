/* -*- mode: C++; c-file-style: "gnu" -*-
  This file is part of KMail, the KDE mail client.
  Copyright (c) 1997 Markus Wuebben <markus.wuebben@kde.org>
  Copyright (c) 2009 Laurent Montel <montel@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

// define this to copy all html that is written to the readerwindow to
// filehtmlwriter.out in the current working directory
#include "kmreaderwin.h"

#include "globalsettings.h"
#include "kmmainwidget.h"
#include "kmreadermainwin.h"
#include "mailkernel.h"

#include "kdepim-version.h"
#include <kpimutils/email.h>
#include <libkdepim/addemailaddressjob.h>
#include <libkdepim/openemailaddressjob.h>
#include "kmcommands.h"
#include "mailcommon/sendmdnhandler.h"
#include <QVBoxLayout>
#include "messageviewer/headerstrategy.h"
#include "messageviewer/headerstyle.h"
#include "messageviewer/mailwebview.h"
#include "messageviewer/markmessagereadhandler.h"
#include "messageviewer/globalsettings.h"

#include "messageviewer/csshelper.h"
using MessageViewer::CSSHelper;
#include "util.h"
#include "stringutil.h"

#include <kmime/kmime_mdn.h>
using namespace KMime;

#include "messageviewer/viewer.h"
using namespace MessageViewer;
#include "messageviewer/attachmentstrategy.h"
#include "messagecomposer/messagesender.h"
#include "messagecomposer/messagefactory.h"
using MessageComposer::MessageFactory;

#include "messagecore/messagehelpers.h"


#include <kde_file.h>
#include <kdebug.h>
#include <klocale.h>
#include <kaction.h>
#include <kicon.h>
#include <kcodecs.h>
#include <ktoggleaction.h>
#include <kservice.h>

#include <QClipboard>

// X headers...
#undef Never
#undef Always

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <mailutil.h>

using namespace KMail;
using namespace MailCommon;

//-----------------------------------------------------------------------------
KMReaderWin::KMReaderWin(QWidget *aParent,
                         QWidget *mainWindow,
                         KActionCollection* actionCollection,
                         Qt::WindowFlags aFlags )
  : QWidget(aParent, aFlags ),
    mMainWindow( mainWindow ),
    mActionCollection( actionCollection ),
    mMailToComposeAction( 0 ),
    mMailToReplyAction( 0 ),
    mMailToForwardAction( 0 ),
    mAddAddrBookAction( 0 ),
    mOpenAddrBookAction( 0 ),
    mUrlSaveAsAction( 0 ),
    mAddBookmarksAction( 0 )
{
  createActions();
  QVBoxLayout * vlay = new QVBoxLayout( this );
  vlay->setMargin( 0 );
  mViewer = new Viewer( this, mainWindow, mActionCollection );
  mViewer->setAppName( "KMail" );
  connect( mViewer, SIGNAL(urlClicked(Akonadi::Item,KUrl)),
           this, SLOT(slotUrlClicked(Akonadi::Item,KUrl)) );
  connect( mViewer, SIGNAL(requestConfigSync()), kmkernel, SLOT(slotRequestConfigSync()), Qt::QueuedConnection ); // happens anyway on shutdown, so we can skip it there with using a queued connection
  connect( mViewer, SIGNAL(resumeNetworkJobs()), kmkernel, SLOT(resumeNetworkJobs()));
  connect( mViewer, SIGNAL(showReader(KMime::Content*,bool,QString)),
           this, SLOT(slotShowReader(KMime::Content*,bool,QString)) );
  connect( mViewer, SIGNAL(showMessage(KMime::Message::Ptr,QString)),
           this, SLOT(slotShowMessage(KMime::Message::Ptr,QString)) );
  connect( mViewer, SIGNAL(showStatusBarMessage(QString)),
           this, SIGNAL(showStatusBarMessage(QString)) );
  connect( mViewer, SIGNAL(deleteMessage(Akonadi::Item)),
           this, SLOT(slotDeleteMessage(Akonadi::Item)) );

  mViewer->addMessageLoadedHandler( new MessageViewer::MarkMessageReadHandler( this ) );
  mViewer->addMessageLoadedHandler( new MailCommon::SendMdnHandler( kmkernel, this ) );

  vlay->addWidget( mViewer );
  readConfig();

}

void KMReaderWin::createActions()
{
  KActionCollection *ac = mActionCollection;
  if ( !ac ) {
    return;
  }
  //
  // Message Menu
  //
  // new message to
  mMailToComposeAction = new KAction( KIcon( "mail-message-new" ),
                                      i18n( "New Message To..." ), this );
  ac->addAction("mail_new", mMailToComposeAction );
  mMailToComposeAction->setShortcutConfigurable( false );
  connect( mMailToComposeAction, SIGNAL(triggered(bool)),
           SLOT(slotMailtoCompose()) );

  // reply to
  mMailToReplyAction = new KAction( KIcon( "mail-reply-sender" ),
                                    i18n( "Reply To..." ), this );
  ac->addAction( "mailto_reply", mMailToReplyAction );
  mMailToReplyAction->setShortcutConfigurable( false );
  connect( mMailToReplyAction, SIGNAL(triggered(bool)),
           SLOT(slotMailtoReply()) );

  // forward to
  mMailToForwardAction = new KAction( KIcon( "mail-forward" ),
                                      i18n( "Forward To..." ), this );
  mMailToForwardAction->setShortcutConfigurable( false );		                                      
  ac->addAction( "mailto_forward", mMailToForwardAction );
  connect( mMailToForwardAction, SIGNAL(triggered(bool)),
           SLOT(slotMailtoForward()) );


  // add to addressbook
  mAddAddrBookAction = new KAction( KIcon( "contact-new" ),
                                    i18n( "Add to Address Book" ), this );
  mAddAddrBookAction->setShortcutConfigurable( false );
  ac->addAction( "add_addr_book", mAddAddrBookAction );
  connect( mAddAddrBookAction, SIGNAL(triggered(bool)),
           SLOT(slotMailtoAddAddrBook()) );

  // open in addressbook
  mOpenAddrBookAction = new KAction( KIcon( "view-pim-contacts" ),
                                     i18n( "Open in Address Book" ), this );
  mOpenAddrBookAction->setShortcutConfigurable( false );
  ac->addAction( "openin_addr_book", mOpenAddrBookAction );
  connect( mOpenAddrBookAction, SIGNAL(triggered(bool)),
           SLOT(slotMailtoOpenAddrBook()) );
  // bookmark message
  mAddBookmarksAction = new KAction( KIcon( "bookmark-new" ), i18n( "Bookmark This Link" ), this );
  mAddBookmarksAction->setShortcutConfigurable( false );
  ac->addAction( "add_bookmarks", mAddBookmarksAction );
  connect( mAddBookmarksAction, SIGNAL(triggered(bool)),
           SLOT(slotAddBookmarks()) );

  // save URL as
  mUrlSaveAsAction = new KAction( i18n( "Save Link As..." ), this );
  ac->addAction( "saveas_url", mUrlSaveAsAction );
  mUrlSaveAsAction->setShortcutConfigurable( false );
  connect( mUrlSaveAsAction, SIGNAL(triggered(bool)), SLOT(slotUrlSave()) );

  // find text
  KAction *action = new KAction(KIcon("edit-find"), i18n("&Find in Message..."), this);
  ac->addAction("find_in_messages", action );
  connect(action, SIGNAL(triggered(bool)), SLOT(slotFind()));
  action->setShortcut(KStandardShortcut::find());

}

void KMReaderWin::setUseFixedFont( bool useFixedFont )
{
  mViewer->setUseFixedFont( useFixedFont );
}

bool KMReaderWin::isFixedFont() const
{
  return mViewer->isFixedFont();
}

//-----------------------------------------------------------------------------
KMReaderWin::~KMReaderWin()
{
}


//-----------------------------------------------------------------------------
void KMReaderWin::readConfig(void)
{
  mViewer->readConfig();
}

void KMReaderWin::setAttachmentStrategy( const AttachmentStrategy * strategy ) {
  mViewer->setAttachmentStrategy( strategy );
}

void KMReaderWin::setHeaderStyleAndStrategy( HeaderStyle * style,
                                             const HeaderStrategy * strategy ) {
  mViewer->setHeaderStyleAndStrategy( style, strategy );
}
//-----------------------------------------------------------------------------
void KMReaderWin::setOverrideEncoding( const QString & encoding )
{
  mViewer->setOverrideEncoding( encoding );
}

//-----------------------------------------------------------------------------
void KMReaderWin::clearCache()
{
  clear();
}

// enter items for the "Important changes" list here:
static const char * const kmailChanges[] = {
  I18N_NOOP( "KMail is now based on the Akonadi Personal Information Management framework, which brings many "
  "changes all around.")
};
static const int numKMailChanges =
  sizeof kmailChanges / sizeof *kmailChanges;

// enter items for the "new features" list here, so the main body of
// the welcome page can be left untouched (probably much easier for
// the translators). Note that the <li>...</li> tags are added
// automatically below:
static const char * const kmailNewFeatures[] = {
  I18N_NOOP( "Push email (IMAP IDLE)" ),
  I18N_NOOP( "Improved virtual folders" ),
  I18N_NOOP( "Improved searches" ),
  I18N_NOOP( "Support for adding notes (annotations) to mails" ),
  I18N_NOOP( "Tag folders" ),
  I18N_NOOP( "Less GUI freezes, mail checks happen in the background" )
};
static const int numKMailNewFeatures =
  sizeof kmailNewFeatures / sizeof *kmailNewFeatures;


//-----------------------------------------------------------------------------
//static
QString KMReaderWin::newFeaturesMD5()
{
  QByteArray str;
  for ( int i = 0 ; i < numKMailChanges ; ++i )
    str += kmailChanges[i];
  for ( int i = 0 ; i < numKMailNewFeatures ; ++i )
    str += kmailNewFeatures[i];
  KMD5 md5( str );
  return md5.base64Digest();
}

//-----------------------------------------------------------------------------
void KMReaderWin::displaySplashPage( const QString &info )
{
  mViewer->displaySplashPage( info );
}

void KMReaderWin::displayBusyPage()
{
  QString info =
    i18n( "<h2 style='margin-top: 0px;'>Retrieving Folder Contents</h2><p>Please wait . . .</p>&nbsp;" );

  displaySplashPage( info );
}

void KMReaderWin::displayOfflinePage()
{
  QString info =
    i18n( "<h2 style='margin-top: 0px;'>Offline</h2><p>KMail is currently in offline mode. "
        "Click <a href=\"kmail:goOnline\">here</a> to go online . . .</p>&nbsp;" );

  displaySplashPage( info );
}


//-----------------------------------------------------------------------------
void KMReaderWin::displayAboutPage()
{
  KLocalizedString info =
    ki18nc("%1: KMail version; %2: help:// URL; "
         "%3: generated list of new features; "
         "%4: First-time user text (only shown on first start); "
         "%5: generated list of important changes; "
         "--- end of comment ---",
         "<h2 style='margin-top: 0px;'>Welcome to KMail %1</h2><p>KMail is the email client by KDE."
         "It is designed to be fully compatible with "
         "Internet mailing standards including MIME, SMTP, POP3, and IMAP."
         "</p>\n"
         "<ul><li>KMail has many powerful features which are described in the "
         "<a href=\"%2\">documentation</a></li>\n"
         "%5\n" // important changes
         "%3\n" // new features
         "%4\n" // first start info
         "<p>We hope that you will enjoy KMail.</p>\n"
         "<p>Thank you,</p>\n"
         "<p style='margin-bottom: 0px'>&nbsp; &nbsp; The KMail Team</p>")
           .subs( KDEPIM_VERSION )
           .subs( "help:/kmail/index.html" );

  if ( ( numKMailNewFeatures > 1 ) || ( numKMailNewFeatures == 1 && strlen(kmailNewFeatures[0]) > 0 ) ) {
    QString featuresText =
      i18n("<p>Some of the new features in this release of KMail include "
           "(compared to KMail %1, which is part of KDE Software Compilation %2):</p>\n",
           QString("1.13"), KDE::versionString() ); // prior KMail and KDE version
    featuresText += "<ul>\n";
    for ( int i = 0 ; i < numKMailNewFeatures ; i++ )
      featuresText += "<li>" + i18n( kmailNewFeatures[i] ) + "</li>\n";
    featuresText += "</ul>\n";
    info = info.subs( featuresText );
  }
  else
    info = info.subs( QString() ); // remove the place holder

  if( kmkernel->firstStart() ) {
    info = info.subs( i18n("<p>Please take a moment to fill in the KMail "
                           "configuration panel at Settings-&gt;Configure "
                           "KMail.\n"
                           "You need to create at least a default identity and "
                           "an incoming as well as outgoing mail account."
                           "</p>\n") );
  } else {
    info = info.subs( QString() ); // remove the place holder
  }

  if ( ( numKMailChanges > 1 ) || ( numKMailChanges == 1 && strlen(kmailChanges[0]) > 0 ) ) {
    QString changesText =
      i18n("<p><span style='font-size:125%; font-weight:bold;'>"
           "Important changes</span> (compared to KMail %1):</p>\n",
       QString("1.13"));
    changesText += "<ul>\n";
    for ( int i = 0 ; i < numKMailChanges ; i++ )
      changesText += i18n("<li>%1</li>\n", i18n( kmailChanges[i] ) );
    changesText += "</ul>\n";
    info = info.subs( changesText );
  }
  else
    info = info.subs( QString() ); // remove the place holder

  displaySplashPage( info.toString() );
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotFind()
{
  mViewer->slotFind();
}
//-----------------------------------------------------------------------------
void KMReaderWin::slotCopySelectedText()
{
  QString selection = mViewer->selectedText();
  selection.replace( QChar::Nbsp, ' ' );
  QApplication::clipboard()->setText( selection );
}

//-----------------------------------------------------------------------------
void KMReaderWin::setMsgPart( KMime::Content* aMsgPart )
{
  mViewer->setMessagePart( aMsgPart );
}

//-----------------------------------------------------------------------------
QString KMReaderWin::copyText() const
{
  return mViewer->selectedText();
}

//-----------------------------------------------------------------------------
void KMReaderWin::setHtmlOverride( bool override )
{
  mViewer->setHtmlOverride( override );
}

bool KMReaderWin::htmlOverride() const
{
  return mViewer->htmlOverride();
}

//-----------------------------------------------------------------------------
void KMReaderWin::setHtmlLoadExtOverride( bool override )
{
  mViewer->setHtmlLoadExtOverride( override );
}

//-----------------------------------------------------------------------------
bool KMReaderWin::htmlMail() const
{
  return mViewer->htmlMail();
}

//-----------------------------------------------------------------------------
bool KMReaderWin::htmlLoadExternal()
{
  return mViewer->htmlLoadExternal();
}

//-----------------------------------------------------------------------------
Akonadi::Item KMReaderWin::message() const
{
  return mViewer->messageItem();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoCompose()
{
  KMCommand *command = new KMMailtoComposeCommand( urlClicked(), message() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoForward()
{
  KMCommand *command = new KMMailtoForwardCommand( mMainWindow, urlClicked(),
                                                   message() );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoAddAddrBook()
{
  const KUrl url = urlClicked();
  if( url.isEmpty() )
    return;
  const QString emailString = KPIMUtils::decodeMailtoUrl( url );

  KPIM::AddEmailAddressJob *job = new KPIM::AddEmailAddressJob( emailString, mMainWindow, this );
  job->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoOpenAddrBook()
{
  const KUrl url = urlClicked();
  if( url.isEmpty() )
    return;	
  const QString emailString = KPIMUtils::decodeMailtoUrl( url );

  KPIM::OpenEmailAddressJob *job = new KPIM::OpenEmailAddressJob( emailString, mMainWindow, this );
  job->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotAddBookmarks()
{
  const KUrl url = urlClicked();
  if( url.isEmpty() )
    return;
  KMCommand *command = new KMAddBookmarksCommand( url, this );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotUrlSave()
{
  const KUrl url = urlClicked();
  if( url.isEmpty() )
    return;
  KMCommand *command = new KMUrlSaveCommand( url, mMainWindow );
  command->start();
}

//-----------------------------------------------------------------------------
void KMReaderWin::slotMailtoReply()
{
  KMCommand *command = new KMMailtoReplyCommand( mMainWindow, urlClicked(),
                                                 message(), copyText() );
  command->start();
}


CSSHelper* KMReaderWin::cssHelper() const
{
  return mViewer->cssHelper();
}

bool KMReaderWin::htmlLoadExtOverride() const
{
  return mViewer->htmlLoadExtOverride();
}
void KMReaderWin::setDecryptMessageOverwrite( bool overwrite )
{
  mViewer->setDecryptMessageOverwrite( overwrite );
}
const AttachmentStrategy * KMReaderWin::attachmentStrategy() const
{
  return mViewer->attachmentStrategy();
}

QString KMReaderWin::overrideEncoding() const
{
  return mViewer->overrideEncoding();
}

KToggleAction *KMReaderWin::toggleFixFontAction()
{
  return mViewer->toggleFixFontAction();
}

KAction *KMReaderWin::toggleMimePartTreeAction()
{
  return mViewer->toggleMimePartTreeAction();
}

KAction *KMReaderWin::selectAllAction()
{
  return mViewer->selectAllAction();
}

const HeaderStrategy * KMReaderWin::headerStrategy() const
{
  return mViewer->headerStrategy();
}

HeaderStyle * KMReaderWin::headerStyle() const
{
  return mViewer->headerStyle();
}

KAction *KMReaderWin::copyURLAction()
{
  return mViewer->copyURLAction();
}

KAction *KMReaderWin::copyImageLocation()
{
  return mViewer->copyImageLocation();
}

KAction *KMReaderWin::copyAction()
{
  return mViewer->copyAction();
}
KAction *KMReaderWin::urlOpenAction()
{
  return mViewer->urlOpenAction();
}
void KMReaderWin::setPrinting(bool enable)
{
  mViewer->setPrinting( enable );
}

KAction* KMReaderWin::speakTextAction()
{
  return mViewer->speakTextAction();
}

void KMReaderWin::clear(bool force )
{
  mViewer->clear( force ? Viewer::Force : Viewer::Delayed );
}

void KMReaderWin::setMessage( const Akonadi::Item &item, Viewer::UpdateMode updateMode)
{
  kDebug() << Q_FUNC_INFO << parentWidget();
  mViewer->setMessageItem( item, updateMode );
}

void KMReaderWin::setMessage( KMime::Message::Ptr message)
{
  mViewer->setMessage( message );
}


KUrl KMReaderWin::urlClicked() const
{
  return mViewer->urlClicked();
}

void KMReaderWin::update( bool force )
{
  mViewer->update( force ? Viewer::Force : Viewer::Delayed );
}

void KMReaderWin::slotUrlClicked( const Akonadi::Item & item, const KUrl & url )
{
  if ( item.isValid() && item.parentCollection().isValid() ) {
    QSharedPointer<FolderCollection> fd = FolderCollection::forCollection( item.parentCollection(), false );
    KMail::Util::handleClickedURL( url, fd );
    return;
  }
  //No folder so we can't have identity and template.
  KMail::Util::handleClickedURL( url );
}

void KMReaderWin::slotShowReader( KMime::Content* msgPart, bool htmlMail, const QString &encoding )
{
  KMReaderMainWin *win = new KMReaderMainWin( msgPart, htmlMail, encoding );
  win->show();
}

void KMReaderWin::slotShowMessage( KMime::Message::Ptr message, const QString& encoding )
{
  KMReaderMainWin *win = new KMReaderMainWin();  
  win->showMessage( encoding, message );
  win->show();
}

void KMReaderWin::slotDeleteMessage(const Akonadi::Item& item)
{
  if ( !item.isValid() )
    return;
  KMTrashMsgCommand *command = new KMTrashMsgCommand( item.parentCollection(), item, -1 );
  command->start();
}

#include "kmreaderwin.moc"


