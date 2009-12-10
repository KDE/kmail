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
#include "kmstartup.h"
#include "aboutdata.h"

#include <QPixmap>
#include <QVBoxLayout>

#include <kparts/mainwindow.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <ksettings/dispatcher.h>
#include <kmailpartadaptor.h>
#include <kicon.h>
#include <akonadi/collection.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/changerecorder.h>
#include "folderselectiontreeview.h"
#include "foldertreeview.h"

#include <QLayout>
#include <kglobal.h>

K_PLUGIN_FACTORY( KMailFactory, registerPlugin<KMailPart>(); )
K_EXPORT_PLUGIN( KMailFactory( KMail::AboutData() ) )

using namespace KMail;

KMailPart::KMailPart(QWidget *parentWidget, QObject *parent, const QVariantList &) :
  KParts::ReadOnlyPart( parent ),
  mParentWidget( parentWidget )
{
  kDebug() << "InstanceName:" << KGlobal::mainComponent().componentName();
  setComponentData(KMailFactory::componentData());
  kDebug() << "InstanceName:" << KGlobal::mainComponent().componentName();

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
  canvas->setObjectName( "canvas" );
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
                                 KGlobal::config() );
  mainWidget->setObjectName( "partmainwidget" );
  QVBoxLayout *topLayout = new QVBoxLayout(canvas);
  topLayout->addWidget(mainWidget);
  topLayout->setMargin(0);
  mainWidget->setFocusPolicy(Qt::ClickFocus);
  mStatusBar  = new KMailStatusBarExtension(this);
  mStatusBar->addStatusBarItem( mainWidget->vacationScriptIndicator(), 2, false );

  // Get to know when the user clicked on a folder in the KMail part and update the headerWidget of Kontact
  connect( mainWidget->folderTreeView(), SIGNAL( currentChanged( const Akonadi::Collection &) ), this, SLOT( slotFolderChanged( const Akonadi::Collection& ) ) );
  connect( kmkernel->monitor(), SIGNAL(collectionChanged( const Akonadi::Collection &, const QSet<QByteArray> &)), this, SLOT(slotCollectionChanged( const Akonadi::Collection &, const QSet<QByteArray> &)));
  setXMLFile( "kmail_part.rc", true );
#endif

  KSettings::Dispatcher::registerComponent( KMailFactory::componentData(), mKMailKernel, "slotConfigChanged" );
}

KMailPart::~KMailPart()
{
  kDebug() << "Closing last KMMainWin: stopping mail check";
  // Running KIO jobs prevent kapp from exiting, so we need to kill them
  // if they are only about checking mail (not important stuff like moving messages)
#if 0
  kmkernel->abortMailCheck();
  kmkernel->acctMgr()->cancelMailCheck();
#else
  kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  mainWidget->destruct();
  kmkernel->cleanup();
  delete kmkernel;
  KMail::cleanup(); // pid file (see kmstartup.cpp)
}

bool KMailPart::openFile()
{
  kDebug();

  mainWidget->show();
  return true;
}

void KMailPart::slotFolderChanged( const Akonadi::Collection &col )
{
  if ( col.isValid() ) {
    emit textChanged( col.name() );
    if ( col.hasAttribute<Akonadi::EntityDisplayAttribute>() &&
         !col.attribute<Akonadi::EntityDisplayAttribute>()->iconName().isEmpty() ) {
      emit iconChanged( col.attribute<Akonadi::EntityDisplayAttribute>()->icon().pixmap( 22, 22 ) );
    }
  }
}
void KMailPart::slotCollectionChanged( const Akonadi::Collection &collection, const QSet<QByteArray> &attributeNames )
{
  if( !attributeNames.contains("ENTITYDISPLAY"))
     return;
  slotFolderChanged(collection);
}

//-----------------------------------------------------------------------------

void KMailPart::guiActivateEvent(KParts::GUIActivateEvent *e)
{
  kDebug();
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

