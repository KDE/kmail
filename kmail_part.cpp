/*  -*- mode: C++; c-file-style: "gnu" -*-

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmail_part.h"

#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kmfoldertree.h"
#include "kmstartup.h"
#include "aboutdata.h"
#include "kmkernel.h"
#include "kmfolder.h"
#include "kmacctmgr.h"
#include "sidebarextension.h"
#include "infoextension.h"
#include "recentaddresses.h"
using KRecentAddress::RecentAddresses;

#include <kapplication.h>
#include <kparts/mainwindow.h>
#include <kparts/genericfactory.h>
#include <knotifyclient.h>
#include <dcopclient.h>
#include <kiconloader.h>
#include <kdebug.h>

#include <qlayout.h>


typedef KParts::GenericFactory< KMailPart > KMailFactory;
K_EXPORT_COMPONENT_FACTORY( libkmailpart, KMailFactory )

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

  // import i18n data and icons from libraries:
  KMail::insertLibraryCataloguesAndIcons();

  // Make sure that the KNotify Daemon is running (this is necessary for people
  // using KMail without KDE)
  KNotifyClient::startDaemon();

  KMail::lockOrDie();

  kapp->dcopClient()->suspend(); // Don't handle DCOP requests yet

  //local, do the init
  KMKernel *kmailKernel = new KMKernel();
  kmailKernel->init();
  kmailKernel->setXmlGuiInstance( KMailFactory::instance() );

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
  kmkernel->inboxFolder()->open();
  KMMessage *msg = kmkernel->inboxFolder()->getMsg(0);
  mReaderWin->setMsg( msg, true );
  mReaderWin->setFocusPolicy(QWidget::ClickFocus);
  m_extension = new KMailBrowserExtension(this);
  mStatusBar  = new KMailStatusBarExtension(this);
  //new KParts::SideBarExtension( kmkernel->mainWin()-mainKMWidget()->leftFrame(), this );
  KGlobal::iconLoader()->addAppDir("kmail");
  setXMLFile( "kmmainwin.rc" );
  kmkernel->inboxFolder()->close();
#else
  mainWidget = new KMMainWidget( canvas, "mainWidget", this, actionCollection(),
                                 kapp->config());
  QVBoxLayout *topLayout = new QVBoxLayout(canvas);
  topLayout->addWidget(mainWidget);
  mainWidget->setFocusPolicy(QWidget::ClickFocus);
  m_extension = new KMailBrowserExtension(this);
  mStatusBar  = new KMailStatusBarExtension(this);
  new KParts::SideBarExtension( mainWidget->folderTree(),
                                this,
                                "KMailSidebar" );

  // Get to know when the user clicked on a folder in the KMail part and update the headerWidget of Kontact
  KParts::InfoExtension *ie = new KParts::InfoExtension( this, "KMailInfo" );
  connect( mainWidget->folderTree(), SIGNAL(folderSelected(KMFolder*)), this, SLOT(exportFolder(KMFolder*)) );
  connect( mainWidget->folderTree(), SIGNAL(iconChanged(KMFolderTreeItem*)),
           this, SLOT(slotIconChanged(KMFolderTreeItem*)) );
  connect( mainWidget->folderTree(), SIGNAL(nameChanged(KMFolderTreeItem*)),
           this, SLOT(slotNameChanged(KMFolderTreeItem*)) );
  connect( this, SIGNAL(textChanged(const QString&)), ie, SIGNAL(textChanged(const QString&)) );
  connect( this, SIGNAL(iconChanged(const QPixmap&)), ie, SIGNAL(iconChanged(const QPixmap&)) );

  KGlobal::iconLoader()->addAppDir( "kmail" );
  setXMLFile( "kmmainwin.rc" );
#endif
}

KMailPart::~KMailPart()
{
  kdDebug(5006) << "Closing last KMMainWin: stopping mail check" << endl;
  // Running KIO jobs prevent kapp from exiting, so we need to kill them
  // if they are only about checking mail (not important stuff like moving messages)
  kmkernel->abortMailCheck();
  kmkernel->acctMgr()->cancelMailCheck();

  mainWidget->destruct();
  kmkernel->cleanup();
  delete kmkernel;
  KMail::cleanup(); // pid file (see kmstartup.cpp)
}

KAboutData *KMailPart::createAboutData()
{
  return new KMail::AboutData();
}

bool KMailPart::openFile()
{
  kdDebug(5006) << "KMailPart:openFile()" << endl;

  mainWidget->show();
  return true;
}

void KMailPart::exportFolder( KMFolder *folder )
{
  KMFolderTreeItem* fti = static_cast< KMFolderTreeItem* >( mainWidget->folderTree()->currentItem() );

  if ( folder != 0 )
    emit textChanged( folder->label() );

  if ( fti )
    emit iconChanged( fti->normalIcon( 22 ) );
}

void KMailPart::slotIconChanged( KMFolderTreeItem *fti )
{
  emit iconChanged( fti->normalIcon( 22 ) );
}

void KMailPart::slotNameChanged( KMFolderTreeItem *fti )
{
  emit textChanged( fti->folder()->label() );
}

//-----------------------------------------------------------------------------

// The sole purpose of the following class is to publicize the protected
// method KParts::MainWindow::createGUI() since we need to call it so that
// the toolbar is redrawn when necessary.
// It can be removed once createGUI() has been made public _and_ we don't
// longer rely on kdelibs 3.2.
class KPartsMainWindowWithPublicizedCreateGUI : public KParts::MainWindow
{
public:
  void createGUIPublic( KParts::Part *part ) {
    createGUI( part );
  }
};

//-----------------------------------------------------------------------------

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

KMailStatusBarExtension::KMailStatusBarExtension( KMailPart *parent )
  : KParts::StatusBarExtension( parent ), mParent( parent )
{
}

KMainWindow * KMailStatusBarExtension::mainWindow() const
{
  return static_cast<KMainWindow*>( mParent->parentWidget() );
}


#include "kmail_part.moc"

