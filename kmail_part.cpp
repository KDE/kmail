/*
    This file is part of KMail.
    Copyright (c) 2002 Don Sanders <sanders@kde.org>,
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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/

#include <qlabel.h>
#include <qlayout.h>

#include <kapplication.h>
#include <kinstance.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <kiconloader.h>
#include <kaction.h>
#include <kdebug.h>
#include <kparts/genericfactory.h>

#include "kmkernel.h"
#include "kmmainwin.h"
#include "kmailIface.h"
#include "kmail_part.h"
#include <klocale.h>
#include <kglobal.h>
#include <knotifyclient.h>
#include <dcopclient.h>
#include "kmreaderwin.h"
#include "kmmainwidget.h"
#include "kmfolder.h"
#include "kmmessage.h"
#include "kmstartup.h"

typedef KParts::GenericFactory< KMailPart > KMailFactory;
K_EXPORT_COMPONENT_FACTORY( libkmailpart, KMailFactory );

KMailPart::KMailPart(QWidget *parentWidget, const char *widgetName,
		     QObject *parent, const char *name, const QStringList &) :
  KParts::ReadOnlyPart(parent, name), DCOPObject("KMailIface")
{
  kdDebug() << "KMailPart()" << endl;
  kdDebug() << "  InstanceName: " << kapp->instanceName() << endl;

  setInstance(KMailFactory::instance());

  kdDebug() << "KMailPart()..." << endl;
  kdDebug() << "  InstanceName: " << kapp->instanceName() << endl;

  KGlobal::locale()->insertCatalogue("libkdenetwork");

  // Check that all updates have been run on the config file:
  kmail::checkConfigUpdates();
  kmail::lockOrDie();

  kapp->dcopClient()->suspend(); // Don't handle DCOP requests yet

  //local, do the init
  KMKernel *kmailKernel = new KMKernel();
  kmailKernel->init();
  kmailKernel->setXmlGuiInstance( KMailFactory::instance() );

  // Will this cause trouble? Comment it out just in case
  kapp->dcopClient()->setDefaultObject( kmailKernel->objId() );

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
      window->close();
  }
  kernel->notClosedByUser();
  delete kernel;
  kmail::cleanup();
}

KAboutData *KMailPart::createAboutData()
{
  KAboutData *about = new KAboutData("kmail", I18N_NOOP("KMail"),
                                     "3.1beta1", I18N_NOOP("A KDE Email Client"),
                                     KAboutData::License_GPL,
                                     I18N_NOOP("(c) 1997-2002, The KMail Team"));
  about->addAuthor("Kermit the frog",I18N_NOOP("Original author and maintainer"));
  return about;
}

bool KMailPart::openFile()
{
  kdDebug() << "KMailPart:openFile()" << endl;

  widget->show();
  return true;
}

void KMailPart::guiActivateEvent(KParts::GUIActivateEvent *e)
{
  kdDebug() << "KMailPart::guiActivateEvent" << endl;
  KParts::ReadOnlyPart::guiActivateEvent(e);
}

void KMailPart::exit()
{
  delete this;
}

KMailBrowserExtension::KMailBrowserExtension(KMailPart *parent) :
  KParts::BrowserExtension(parent, "KMailBrowserExtension")
{
}

KMailBrowserExtension::~KMailBrowserExtension()
{
}

using namespace KParts;
#include "kmail_part.moc"

