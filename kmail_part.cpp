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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include "kmail_part.h"

#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kmfoldertree.h"
#include "kmstartup.h"
#include "aboutdata.h"
#include "kmfolder.h"
#include "accountmanager.h"
#include <QPixmap>
#include <QVBoxLayout>
using KMail::AccountManager;

#include <kparts/mainwindow.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <ksettings/dispatcher.h>
#include <kmailpartadaptor.h>

#include <QLayout>
#include <kglobal.h>

K_PLUGIN_FACTORY( KMailFactory, registerPlugin<KMailPart>(); )
K_EXPORT_PLUGIN( KMailFactory( KMail::AboutData() ) )

KMailPart::KMailPart(QWidget *parentWidget, QObject *parent, const QVariantList &) :
  KParts::ReadOnlyPart( parent ),
  mParentWidget( parentWidget )
{
  kDebug(5006) << "InstanceName:" << KGlobal::mainComponent().componentName();
  setComponentData(KMailFactory::componentData());
  kDebug(5006) << "InstanceName:" << KGlobal::mainComponent().componentName();

  // import i18n data and icons from libraries:
  KMail::insertLibraryCataloguesAndIcons();


  KMail::lockOrDie();

  //local, do the init
  KMKernel *mKMailKernel = new KMKernel();
  mKMailKernel->init();
  mKMailKernel->setXmlGuiInstance( KMailFactory::componentData() );

  // and session management
  mKMailKernel->doSessionManagement();

  // any dead letters?
  mKMailKernel->recoverDeadLetters();

  kmsetSignalHandler(kmsignalHandler);
  kmkernel->setupDBus(); // Ok. We are ready for D-Bus requests.
  (void) new KmailpartAdaptor( this );
  QDBusConnection::sessionBus().registerObject( "/KMailPart", this );

  // create a canvas to insert our widget
  QWidget *canvas = new QWidget( parentWidget );
  canvas->setFocusPolicy(Qt::ClickFocus);
  setWidget(canvas);
  KIconLoader::global()->addAppDir("kmail");
  KIconLoader::global()->addAppDir( "kdepim" );
#if 0
  //It's also possible to make a part out of a readerWin
  KMReaderWin *mReaderWin = new KMReaderWin( canvas, canvas, actionCollection() );
  connect(mReaderWin, SIGNAL(urlClicked(const KUrl&,int)),
	  mReaderWin, SLOT(slotUrlClicked()));
  QVBoxLayout *topLayout = new QVBoxLayout(canvas);
  topLayout->addWidget(mReaderWin);
  mReaderWin->setAutoDelete( true );
  kmkernel->inboxFolder()->open();
  KMMessage *msg = kmkernel->inboxFolder()->getMsg(0);
  mReaderWin->setMsg( msg, true );
  mReaderWin->setFocusPolicy(Qt::ClickFocus);
  mStatusBar  = new KMailStatusBarExtension(this);
  //new KParts::SideBarExtension( kmkernel->mainWin()-mainKMWidget()->leftFrame(), this );
  setXMLFile( "kmail_part.rc" );
  kmkernel->inboxFolder()->close();
#else
  mainWidget = new KMMainWidget( canvas, this, actionCollection(),
                                 KGlobal::config().data());
  QVBoxLayout *topLayout = new QVBoxLayout(canvas);
  topLayout->addWidget(mainWidget);
  topLayout->setMargin(0);
  mainWidget->setFocusPolicy(Qt::ClickFocus);
  mStatusBar  = new KMailStatusBarExtension(this);
  mStatusBar->addStatusBarItem( mainWidget->vacationScriptIndicator(), 2, false );

  // Get to know when the user clicked on a folder in the KMail part and update the headerWidget of Kontact
  connect( mainWidget->folderTree(), SIGNAL(folderSelected(KMFolder*)), this, SLOT(exportFolder(KMFolder*)) );
  connect( mainWidget->folderTree(), SIGNAL(iconChanged(KMFolderTreeItem*)),
           this, SLOT(slotIconChanged(KMFolderTreeItem*)) );
  connect( mainWidget->folderTree(), SIGNAL(nameChanged(KMFolderTreeItem*)),
           this, SLOT(slotNameChanged(KMFolderTreeItem*)) );

  setXMLFile( "kmail_part.rc" );
#endif

  KSettings::Dispatcher::registerComponent( KMailFactory::componentData(), mKMailKernel,
                                            SLOT( slotConfigChanged() ) );
}

KMailPart::~KMailPart()
{
  kDebug(5006) <<"Closing last KMMainWin: stopping mail check";
  // Running KIO jobs prevent kapp from exiting, so we need to kill them
  // if they are only about checking mail (not important stuff like moving messages)
  kmkernel->abortMailCheck();
  kmkernel->acctMgr()->cancelMailCheck();

  mainWidget->destruct();
  kmkernel->cleanup();
  delete kmkernel;
  KMail::cleanup(); // pid file (see kmstartup.cpp)
}

bool KMailPart::openFile()
{
  kDebug(5006);

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
  kDebug(5006);
  KParts::ReadOnlyPart::guiActivateEvent(e);
  mainWidget->initializeFilterActions();
  mainWidget->initializeMessageTagActions();
  mainWidget->initializeFolderShortcutActions();
  mainWidget->updateVactionScriptStatus();
}

void KMailPart::exit()
{
  delete this;
}

QWidget* KMailPart::parentWidget() const
{
  return mParentWidget;
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

