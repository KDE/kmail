// kmreadermainwin
// (c) 2002 Don Sanders <sanders@kde.org>
// License: GPL
//
// A toplevel KMainWindow derived class for displaying
// single messages or single message parts.
//
// Could be extended to include support for normal main window
// widgets like a toolbar.

#include <qaccel.h>
#include <qimage.h>
#include <qmultilineedit.h>
#include <qtextcodec.h>
#include <kapplication.h>
#include <kcharsets.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstdaccel.h>
#include <kwin.h>

#include "kbusyptr.h"
#include "kmcommands.h"
#include "kmdisplayvcard.h"
#include "kmkernel.h"
#include "kmmessage.h"
#include "mailinglist-magic.h"
#include "kmmsgpart.h"
#include "kpopupmenu.h"
#include "kmreaderwin.h"

#include "kmreadermainwin.h"
#include "kmreadermainwin.moc"

KMReaderMainWin::KMReaderMainWin( bool htmlOverride, char *name )
  : KMTopLevelWidget( name ), mMsg( 0 )
{
  KWin::setIcons(winId(), kapp->icon(), kapp->miniIcon());
  mReaderWin = new KMReaderWin( this, this );
  mReaderWin->setShowCompleteMessage( true );
  mReaderWin->setAutoDelete( true );
  mReaderWin->setHtmlOverride( htmlOverride );
  setCentralWidget( mReaderWin );
  setupAccel();
}


KMReaderMainWin::KMReaderMainWin( char *name )
  : KMTopLevelWidget( name ), mMsg( 0 )
{
  mReaderWin = new KMReaderWin( this, this );
  mReaderWin->setAutoDelete( true );
  setCentralWidget( mReaderWin );
  setupAccel();
}


KMReaderMainWin::KMReaderMainWin(KMMessagePart* aMsgPart,
    bool aHTML, const QString& aFileName, const QString& pname,
    const QTextCodec *codec, char *name )
  : KMTopLevelWidget( name ), mMsg( 0 )
{
  resize( 550, 600 );
  mReaderWin = new KMReaderWin( this, this ); //new reader
  mReaderWin->setMsgPart( aMsgPart, aHTML, aFileName, pname, codec );
  setCentralWidget( mReaderWin );
  setupAccel();
}


KMReaderMainWin::~KMReaderMainWin()
{
}


void KMReaderMainWin::showMsg( const QTextCodec *codec, KMMessage *msg )
{
  mReaderWin->setCodec( codec );
  mReaderWin->setMsg( msg, true );
  setCaption( msg->subject() );
  mMsg = msg;
  mCodec = codec;
}


void KMReaderMainWin::setupAccel()
{
  QAccel *accel = new QAccel(mReaderWin, "showMsg()");
  accel->connectItem(accel->insertItem(Key_Up),
                     mReaderWin, SLOT(slotScrollUp()));
  accel->connectItem(accel->insertItem(Key_Down),
                     mReaderWin, SLOT(slotScrollDown()));
  accel->connectItem(accel->insertItem(Key_Prior),
                     mReaderWin, SLOT(slotScrollPrior()));
  accel->connectItem(accel->insertItem(Key_Next),
                     mReaderWin, SLOT(slotScrollNext()));
#ifndef NO_NEW_FEATURES
  accel->connectItem(accel->insertItem(KStdAccel::shortcut(KStdAccel::Copy)),
                     mReaderWin, SLOT(slotCopySelectedText()));
  connect( mReaderWin, SIGNAL(popupMenu(KMMessage&,const KURL&,const QPoint&)),
	  this, SLOT(slotMsgPopup(KMMessage&,const KURL&,const QPoint&)));
#endif
}


void KMReaderMainWin::slotMsgPopup(KMMessage &aMsg, const KURL &aUrl, const QPoint& aPoint)
{
  KPopupMenu * menu = new KPopupMenu;
  mUrl = aUrl;
  mMsg = &aMsg;

  if (!aUrl.isEmpty()) {
    if (aUrl.protocol() == "mailto") {
      // popup on a mailto URL
      menu->insertItem(i18n("Send To..."), this, SLOT(slotMailtoCompose()));

      if ( mMsg ) {
        menu->insertItem(i18n("Send Reply To..."), this,
                         SLOT(slotMailtoReply()));
        menu->insertItem(i18n("Forward To..."), this,
                         SLOT(slotMailtoForward()));
        menu->insertSeparator();
      }
      menu->insertItem(i18n("Add to Addressbook"), this,
		       SLOT(slotMailtoAddAddrBook()));
      menu->insertItem(i18n("Open in Addressbook..."), this,
		       SLOT(slotMailtoOpenAddrBook()));
      menu->insertItem(i18n("Copy to Clipboard"), this, SLOT(slotUrlCopy()));
		
    } else {
      // popup on a not-mailto URL
      menu->insertItem(i18n("Open URL..."), this, SLOT(slotUrlOpen()));
      menu->insertItem(i18n("Save Link As..."), this, SLOT(slotUrlSave()));
      menu->insertItem(i18n("Copy to Clipboard"), this, SLOT(slotUrlCopy()));
    }
  } else {
    // popup somewhere else (i.e., not a URL) on the message

    if (!mMsg) // no message
    {
      delete menu;
      return;
    }

    menu->insertItem( i18n("&Reply..."), this, SLOT(slotReplyToMsg()) );
    menu->insertItem( i18n("Reply &All..."), this, SLOT(slotReplyAllToMsg()) );
    menu->insertItem( i18n("Message->","&Forward"), this,
		      SLOT(slotForwardMsg()) );
    menu->insertItem( i18n("Message->Forward->","As &Attachment..."), this,
		      SLOT(slotForwardAttachedMsg()) );
    menu->insertItem( i18n("Message->Forward->","&Redirect..."), this,
		      SLOT(slotRedirectMsg()) );
    menu->insertItem( i18n("&Bounce..."), this, SLOT(slotBounceMsg()) );
    menu->insertSeparator();

    QPopupMenu* filterMenu = new QPopupMenu(menu);
    filterMenu->insertItem( i18n("Filter on &Subject..."), this,
			    SLOT(slotSubjectFilter()) );
    filterMenu->insertItem( i18n("Filter on &From..."), this,
			    SLOT(slotFromFilter()) );
    filterMenu->insertItem( i18n("Filter on &To..."), this,
			    SLOT(slotToFilter()) );

    int fml = filterMenu->insertItem( i18n("Filter on Mailing-&List..."), this,
				      SLOT(slotMailingListFilter()) );
    QCString name;
    QString value;
    QString lname = KMMLInfo::name( mMsg, name, value );
    filterMenu->setItemEnabled( fml, !lname.isNull() );
    if ( !lname.isNull() )
      filterMenu->changeItem( fml, i18n( "Filter on Mailing-List %1..." ).arg( lname ) );
    menu->insertItem( i18n("Create F&ilter"), filterMenu );

    QPopupMenu* copyMenu = new QPopupMenu(menu);
    KMMenuCommand::folderToPopupMenu( false, this, &mMenuToFolder, copyMenu );
    menu->insertItem( i18n("&Copy To" ), copyMenu );
    menu->insertSeparator();

    menu->insertItem( i18n("Fixed Font &Widths"), this,
		      SLOT(slotToggleFixedFont()) );
    menu->insertItem( i18n("&View Source"), this,
		      SLOT(slotShowMsgSrc()) );
    menu->insertSeparator();

    menu->insertItem( i18n("&Print..."), this, SLOT(slotPrintMsg()) );
    menu->insertItem( i18n("Save &As..."), this, SLOT(slotSaveMsg()) );
  }
  menu->exec(aPoint, 0);
  delete menu;
}

void KMReaderMainWin::slotMailtoCompose()
{
  KMCommand *command = new KMMailtoComposeCommand( mUrl, mMsg );
  command->start();
}

void KMReaderMainWin::slotMailtoForward()
{
  KMCommand *command = new KMMailtoForwardCommand( this, mUrl, mMsg );
  command->start();
}

void KMReaderMainWin::slotMailtoAddAddrBook()
{
  KMCommand *command = new KMMailtoAddAddrBookCommand( mUrl, this );
  command->start();
}

void KMReaderMainWin::slotMailtoOpenAddrBook()
{
  KMCommand *command = new KMMailtoOpenAddrBookCommand( mUrl, this );
  command->start();
}

void KMReaderMainWin::slotUrlCopy()
{
  KMCommand *command = new KMUrlCopyCommand( mUrl );
  command->start();
}

void KMReaderMainWin::slotUrlOpen()
{
  KMCommand *command = new KMUrlOpenCommand( mUrl, mReaderWin );
  command->start();
}

void KMReaderMainWin::slotUrlSave()
{
  KMCommand *command = new KMUrlSaveCommand( mUrl, this );
  command->start();
}

void KMReaderMainWin::slotMailtoReply()
{
  KMCommand *command = new KMMailtoReplyCommand( this, mUrl,
    mMsg, mReaderWin->copyText() );
  command->start();
}

void KMReaderMainWin::slotReplyToMsg()
{
  KMCommand *command = new KMReplyToCommand( this, mMsg,
					     mReaderWin->copyText() );
  command->start();
}

void KMReaderMainWin::slotReplyAllToMsg()
{
  KMCommand *command = new KMReplyToAllCommand( this,  mMsg,
						mReaderWin->copyText() );
  command->start();
}

void KMReaderMainWin::slotForwardMsg()
{
  QPtrList<KMMsgBase> msgList;
  msgList.append( mMsg );
  KMCommand *command = new KMForwardCommand( this, msgList, 0 );
  command->start();
}

void KMReaderMainWin::slotForwardAttachedMsg()
{
  QPtrList<KMMsgBase> msgList;
  msgList.append( mMsg );
  KMCommand *command = new KMForwardAttachedCommand( this, msgList, 0 );
  command->start();
}

void KMReaderMainWin::slotRedirectMsg()
{
  KMCommand *command = new KMRedirectCommand( this, mMsg );
  command->start();
}

void KMReaderMainWin::slotBounceMsg()
{
  KMCommand *command = new KMBounceCommand( this, mMsg );
  command->start();
}

void KMReaderMainWin::slotSubjectFilter()
{
  KMCommand *command = new KMFilterCommand( "Subject", mMsg->subject() );
  command->start();
}

void KMReaderMainWin::slotFromFilter()
{
  KMCommand *command = new KMFilterCommand( "From",  mMsg->from() );
  command->start();
}

void KMReaderMainWin::slotToFilter()
{
  KMCommand *command = new KMFilterCommand( "To",  mMsg->to() );
  command->start();
}

void KMReaderMainWin::slotMailingListFilter()
{
  KMCommand *command = new KMMailingListFilterCommand( this, mMsg );
  command->start();
}

void KMReaderMainWin::copySelectedToFolder( int menuId )
{
  if (!mMenuToFolder[menuId])
    return;

  KMFolder *destFolder = mMenuToFolder[menuId];
  QPtrList<KMMsgBase> msgList;
  msgList.append( mMsg );
  KMCommand *command = new KMCopyCommand( 0, destFolder, msgList );
  command->start();
}

void KMReaderMainWin::slotToggleFixedFont()
{
  KMCommand *command = new KMToggleFixedCommand( mReaderWin );
  command->start();
}

void KMReaderMainWin::slotShowMsgSrc()
{
  KMCommand *command = new KMShowMsgSrcCommand( this, mMsg,
    mCodec, mReaderWin->isfixedFont() );
  command->start();
}

void KMReaderMainWin::slotPrintMsg()
{
  KMCommand *command = new KMPrintCommand( this, mMsg );
  command->start();
}

void KMReaderMainWin::slotSaveMsg()
{
  QPtrList<KMMsgBase> msgList;
  msgList.append( mMsg );
  KMSaveMsgCommand *saveCommand = new KMSaveMsgCommand( this, msgList, 0 );
  if (saveCommand->url().isEmpty())
    delete saveCommand;
  else
    saveCommand->start();
}
