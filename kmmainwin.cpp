/*
 * kmail: KDE mail client
 * Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "kmmainwin.h"
#include "kmmainwidget.h"
#include "kstatusbar.h"
#include "progresswidget/progressdialog.h"
#include "progresswidget/statusbarprogresswidget.h"
#include "misc/broadcaststatus.h"
#include "util.h"
#include "tag/tagactionmanager.h"

#include <QTimer>

#include <KMenuBar>
#include <KToggleAction>
#include <kapplication.h>
#include <klocale.h>
#include <kedittoolbar.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kxmlguifactory.h>
#include <kstringhandler.h>
#include <kstandardaction.h>
#include <kdebug.h>
#include <ktip.h>
#include <kicon.h>
#include <kaction.h>

#include <QLabel>
#include "messagecomposer/sender/messagesender.h"




KMMainWin::KMMainWin(QWidget *)
    : KXmlGuiWindow( 0 ),
      mReallyClose( false )
{
  setObjectName( QLatin1String("kmail-mainwindow#") );
  // Set this to be the group leader for all subdialogs - this means
  // modal subdialogs will only affect this dialog, not the other windows
  setAttribute( Qt::WA_GroupLeader );

  KAction *action  = new KAction( KIcon(QLatin1String("window-new")), i18n("New &Window"), this );
  actionCollection()->addAction( QLatin1String("new_mail_client"), action );
  connect( action, SIGNAL(triggered(bool)), SLOT(slotNewMailReader()) );

  resize( 700, 500 ); // The default size

  mKMMainWidget = new KMMainWidget( this, this, actionCollection() );
  connect(mKMMainWidget,SIGNAL(recreateGui()),this,SLOT(slotUpdateGui()));
  setCentralWidget( mKMMainWidget );
  setupStatusBar();
  if ( kmkernel->xmlGuiInstance().isValid() )
    setComponentData( kmkernel->xmlGuiInstance() );

  setStandardToolBarMenuEnabled( true );

  KStandardAction::configureToolbars( this, SLOT(slotEditToolbars()),
                                      actionCollection() );

  KStandardAction::keyBindings( guiFactory(), SLOT(configureShortcuts()),
                                actionCollection() );

  mHideMenuBarAction = KStandardAction::showMenubar( this, SLOT(slotToggleMenubar()), actionCollection() );
  mHideMenuBarAction->setChecked( GlobalSettings::self()->showMenuBar() );
  slotToggleMenubar( true );


  KStandardAction::quit( this, SLOT(slotQuit()), actionCollection() );
  createGUI( QLatin1String("kmmainwin.rc") );

  //must be after createGUI, otherwise e.g toolbar settings are not loaded
  applyMainWindowSettings( KMKernel::self()->config()->group( "Main Window") );

  connect( KPIM::BroadcastStatus::instance(), SIGNAL(statusMsg(QString)),
           this, SLOT(displayStatusMsg(QString)) );

  connect( mKMMainWidget, SIGNAL(captionChangeRequest(QString)),
           SLOT(setCaption(QString)) );

  if ( kmkernel->firstInstance() )
    QTimer::singleShot( 200, this, SLOT(slotShowTipOnStart()) );
}

KMMainWin::~KMMainWin()
{
  saveMainWindowSettings( KMKernel::self()->config()->group( "Main Window") );
  KMKernel::self()->config()->sync();
}

void KMMainWin::displayStatusMsg( const QString& aText )
{
  if ( !statusBar() || !mLittleProgress )
    return;
  const int statusWidth = statusBar()->width() - mLittleProgress->width()
                    - fontMetrics().maxWidth();

  const QString text = fontMetrics().elidedText( QLatin1Char(' ') + aText, Qt::ElideRight,
                                           statusWidth );

  // ### FIXME: We should disable richtext/HTML (to avoid possible denial of service attacks),
  // but this code would double the size of the status bar if the user hovers
  // over an <foo@bar.com>-style email address :-(
//  text.replace("&", "&amp;");
//  text.replace("<", "&lt;");
//  text.replace(">", "&gt;");

  statusBar()->changeItem( text, 1 );
}

void KMMainWin::slotToggleMenubar(bool dontShowWarning)
{
    if ( menuBar() ) {
        if ( mHideMenuBarAction->isChecked() ) {
            menuBar()->show();
        } else {
            if ( !dontShowWarning ) {
                const QString accel = mHideMenuBarAction->shortcut().toString();
                KMessageBox::information( this,
                                          i18n( "<qt>This will hide the menu bar completely."
                                                " You can show it again by typing %1.</qt>", accel ),
                                          i18n("Hide menu bar"), QLatin1String("HideMenuBarWarning") );
            }
            menuBar()->hide();
        }
        GlobalSettings::self()->setShowMenuBar( mHideMenuBarAction->isChecked() );
    }
}


//-----------------------------------------------------------------------------
void KMMainWin::slotNewMailReader()
{
  KMMainWin *d;

  d = new KMMainWin();
  d->show();
  d->resize( d->size() );
}


void KMMainWin::slotEditToolbars()
{
  saveMainWindowSettings(KMKernel::self()->config()->group( "Main Window") );
  KEditToolBar dlg(guiFactory(), this);
  connect( &dlg, SIGNAL(newToolBarConfig()), SLOT(slotUpdateGui()) );

  dlg.exec();
}

void KMMainWin::slotUpdateGui()
{
  // remove dynamically created actions before editing
  mKMMainWidget->clearFilterActions();
  mKMMainWidget->tagActionManager()->clearActions();

  createGUI(QLatin1String("kmmainwin.rc"));
  applyMainWindowSettings(KMKernel::self()->config()->group( "Main Window") );

  // plug dynamically created actions again
  mKMMainWidget->initializeFilterActions();
  mKMMainWidget->tagActionManager()->createActions();
}

void KMMainWin::setupStatusBar()
{
  /* Create a progress dialog and hide it. */
  mProgressDialog = new KPIM::ProgressDialog( statusBar(), this );
  mProgressDialog->hide();

  mLittleProgress = new StatusbarProgressWidget( mProgressDialog, statusBar() );
  mLittleProgress->show();

  statusBar()->insertItem( i18n("Starting..."), 1, 4 );
  QTimer::singleShot( 2000, KPIM::BroadcastStatus::instance(), SLOT(reset()) );
  statusBar()->setItemAlignment( 1, Qt::AlignLeft | Qt::AlignVCenter );
  statusBar()->addPermanentWidget( mKMMainWidget->vacationScriptIndicator() );
  statusBar()->addPermanentWidget( mLittleProgress );
  mLittleProgress->show();
}

void KMMainWin::slotQuit()
{
  mReallyClose = true;
  close();
}

//-----------------------------------------------------------------------------
bool KMMainWin::restoreDockedState( int n )
{
  // Default restore behavior is to show the window once it is restored.
  // Override this if the main window was hidden in the system tray
  // when the session was saved.
  KConfigGroup config( kapp->sessionConfig(), QString::number( n ) );
  bool show = !config.readEntry ("docked", false );

  return KMainWindow::restore ( n, show );
}

void KMMainWin::saveProperties( KConfigGroup &config )
{
  // This is called by the session manager on log-off
  // Save the shown/hidden status so we can restore to the same state.
  KMainWindow::saveProperties( config );
  config.writeEntry( "docked", isHidden() );
}

bool KMMainWin::queryClose()
{
  if ( kmkernel->shuttingDown() || kapp->sessionSaving() || mReallyClose )
    return true;
  return kmkernel->canQueryClose();
}

void KMMainWin::slotShowTipOnStart()
{
  KTipDialog::showTip( this );
}
#include "moc_kmmainwin.cpp"
