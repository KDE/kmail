/*
    This file is part of KMail.
    Copyright (c) 2002-2003 Don Sanders <sanders@kde.org>,
    Copyright (c) 2003      Zack Rusin  <zack@kde.org>,
    Based on the work of Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <qlayout.h>

#include <kapplication.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kparts/genericfactory.h>

#include "aboutdata.h"
#include "kmmainwin.h"
#include "kmail_part.h"
#include <knotifyclient.h>
#include <dcopclient.h>
#include "kmmainwidget.h"
#include "kmfoldertree.h"
#include "kmstartup.h"
#include "kmbroadcaststatus.h"
#if KDE_IS_VERSION( 3, 1, 90 )
#  include "sidebarextension.h"
#endif

typedef KParts::GenericFactory< KMailPart > KMailFactory;
K_EXPORT_COMPONENT_FACTORY( libkmailpart, KMailFactory );

KMailPart::KMailPart(QWidget *parentWidget, const char *widgetName,
		     QObject *parent, const char *name, const QStringList &) :
  DCOPObject("KMailIface"), KParts::ReadOnlyPart(parent, name), 
  mParentWidget( parentWidget )
{
  kdDebug(5006) << "KMailPart()" << endl;
  kdDebug(5006) << "  InstanceName: " << kapp->instanceName() << endl;

  setInstance(KMailFactory::instance());

  kdDebug(5006) << "KMailPart()..." << endl;
  kdDebug(5006) << "  InstanceName: " << kapp->instanceName() << endl;

  KGlobal::locale()->insertCatalogue("libkdenetwork");

  // Check that all updates have been run on the config file:
  KMail::checkConfigUpdates();

  // Make sure that the KNotify Daemon is running (this is necessary for people
  // using KMail without KDE)
  KNotifyClient::startDaemon();

  KMail::lockOrDie();

  kapp->dcopClient()->suspend(); // Don't handle DCOP requests yet

  //local, do the init
  KMKernel *kmailKernel = new KMKernel();
  kmailKernel->init();
  kmailKernel->setXmlGuiInstance( KMailFactory::instance() );

  // Will this cause trouble? Comment it out just in case
  // Yes, it's wrong in kontact... (DF)
  //kapp->dcopClient()->setDefaultObject( kmailKernel->objId() );

  // and session management
  kmailKernel->doSessionManagement();

  // any dead letters?
  kmailKernel->recoverDeadLetters();

  kmsetSignalHandler(kmsignalHandler);
  kapp->dcopClient()->resume(); // Ok. We are ready for DCOP requests.

  // create a canvas to insert our widget
  QWidget *canvas = new QWidget(parentWidget, widgetName);
  canvas->setFocusPolicy(QWidget::ClickFocus);
  setWidget(canvas);
  KGlobal::iconLoader()->addAppDir("kmail");
#if 0
  //It's also possible to make a part out of a readerWin
  KMReaderWin *mReaderWin = new KMReaderWin( canvas, canvas, actionCollection() );
  connect(mReaderWin, SIGNAL(urlClicked(const KURL&,int)),
	  mReaderWin, SLOT(slotUrlClicked()));
  QVBoxLayout *topLayout = new QVBoxLayout(canvas);
  topLayout->addWidget(mReaderWin);
  mReaderWin->setAutoDelete( true );
  kernel->inboxFolder()->open();
  KMMessage *msg = kernel->inboxFolder()->getMsg(0);
  mReaderWin->setMsg( msg, true );
  mReaderWin->setFocusPolicy(QWidget::ClickFocus);
  m_extension = new KMailBrowserExtension(this);
#if KDE_IS_VERSION( 3, 1, 90 )
  mStatusBar  = new KMailStatusBarExtension(this);
  mStatusBar->addStatusBarItem( mainWidget->progressDialog(), 0, true );
  //new KParts::SideBarExtension( kernel->mainWin()-mainKMWidget()->leftFrame(), this );
#endif
  KGlobal::iconLoader()->addAppDir("kmail");
  setXMLFile( "kmmainwin.rc" );
  mReaderWin->show();
  kernel->inboxFolder()->close();
#else
  mainWidget = new KMMainWidget( canvas, "mainWidget", actionCollection());
  QVBoxLayout *topLayout = new QVBoxLayout(canvas);
  topLayout->addWidget(mainWidget);
  mainWidget->setFocusPolicy(QWidget::ClickFocus);
  m_extension = new KMailBrowserExtension(this);
#if KDE_IS_VERSION( 3, 1, 90 )
  mStatusBar  = new KMailStatusBarExtension(this);
  mStatusBar->addStatusBarItem( mainWidget->progressDialog(), 0, true );
  new KParts::SideBarExtension( mainWidget->folderTree(),
                                this,
                                "KMailSidebar" );
#endif
  KGlobal::iconLoader()->addAppDir("kmail");
  setXMLFile( "kmmainwin.rc" );
  mainWidget->show();
#endif
}

KMailPart::~KMailPart()
{
  kernel->dumpDeadLetters();
  mainWidget->destruct();
  QPtrListIterator<KMainWindow> it(*KMainWindow::memberList);
  KMainWindow *window = 0;
  while ((window = it.current()) != 0) {
    ++it;
    if (window->inherits("KMTopLevelWidget"))
      window->close(TRUE);
  }
  kernel->notClosedByUser();
  delete kernel;
  KMail::cleanup();
}

KAboutData *KMailPart::createAboutData()
{
  return new KMail::AboutData();
}

bool KMailPart::openFile()
{
  kdDebug(5006) << "KMailPart:openFile()" << endl;

  widget->show();
  return true;
}

void KMailPart::guiActivateEvent(KParts::GUIActivateEvent *e)
{
  kdDebug(5006) << "KMailPart::guiActivateEvent" << endl;
  KParts::ReadOnlyPart::guiActivateEvent(e);
}

void KMailPart::exit()
{
  delete this;
}

QWidget* KMailPart::parentWidget() const
{
  return mParentWidget;
}

KMailBrowserExtension::KMailBrowserExtension(KMailPart *parent) :
  KParts::BrowserExtension(parent, "KMailBrowserExtension")
{
}

KMailBrowserExtension::~KMailBrowserExtension()
{
}

#if KDE_IS_VERSION( 3, 1, 90 )
KMailStatusBarExtension::KMailStatusBarExtension( KMailPart *parent )
  : KParts::StatusBarExtension( parent ), mParent( parent )
{
}

KMainWindow * KMailStatusBarExtension::mainWindow() const
{
  return static_cast<KMainWindow*>( mParent->parentWidget() );
}

#endif


#include "kmail_part.moc"

