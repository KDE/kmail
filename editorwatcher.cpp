/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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
*/

#include "editorwatcher.h"

#include <config.h>

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kopenwith.h>
#include <kprocess.h>
#include <kuserprofile.h>

#include <qsocketnotifier.h>

#include <cassert>

// inotify stuff taken from kdelibs/kio/kio/kdirwatch.cpp
#ifdef HAVE_SYS_INOTIFY
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <fcntl.h>
#elif HAVE_INOTIFY
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <linux/types.h>
// Linux kernel headers are documented to not compile
#define _S390_BITOPS_H
#include <linux/inotify.h>

static inline int inotify_init (void)
{
  return syscall (__NR_inotify_init);
}

static inline int inotify_add_watch (int fd, const char *name, __u32 mask)
{
  return syscall (__NR_inotify_add_watch, fd, name, mask);
}

static inline int inotify_rm_watch (int fd, __u32 wd)
{
  return syscall (__NR_inotify_rm_watch, fd, wd);
}
#endif

using namespace KMail;

EditorWatcher::EditorWatcher(const KURL & url, const QString &mimeType, bool openWith,
                             QObject * parent, QWidget *parentWidget) :
    QObject( parent ),
    mUrl( url ),
    mMimeType( mimeType ),
    mOpenWith( openWith ),
    mEditor( 0 ),
    mParentWidget( parentWidget ),
    mHaveInotify( false ),
    mFileOpen( false ),
    mEditorRunning( false ),
    mFileModified( true ), // assume the worst unless we know better
    mDone( false )
{
  assert( mUrl.isLocalFile() );
  connect( &mTimer, SIGNAL(timeout()), SLOT(checkEditDone()) );
}

bool EditorWatcher::start()
{
  // find an editor
  KURL::List list;
  list.append( mUrl );
  KService::Ptr offer = KServiceTypeProfile::preferredService( mMimeType, "Application" );
  if ( mOpenWith || !offer ) {
    KOpenWithDlg dlg( list, i18n("Edit with:"), QString::null, 0 );
    if ( !dlg.exec() )
      return false;
    offer = dlg.service();
    if ( !offer )
      return false;
  }

#ifdef HAVE_INOTIFY
  // monitor file
  mInotifyFd = inotify_init();
  if ( mInotifyFd > 0 ) {
    mInotifyWatch = inotify_add_watch( mInotifyFd, mUrl.path().latin1(), IN_CLOSE | IN_OPEN | IN_MODIFY );
    if ( mInotifyWatch >= 0 ) {
      QSocketNotifier *sn = new QSocketNotifier( mInotifyFd, QSocketNotifier::Read, this );
      connect( sn, SIGNAL(activated(int)), SLOT(inotifyEvent()) );
      mHaveInotify = true;
      mFileModified = false;
    }
  } else {
    kdWarning(5006) << k_funcinfo << "Failed to activate INOTIFY!" << endl;
  }
#endif

  // start the editor
  QStringList params = KRun::processDesktopExec( *offer, list, false );
  mEditor = new KProcess( this );
  *mEditor << params;
  connect( mEditor, SIGNAL(processExited(KProcess*)), SLOT(editorExited()) );
  if ( !mEditor->start() )
    return false;
  mEditorRunning = true;

  mEditTime.start();
  return true;
}

void EditorWatcher::inotifyEvent()
{
  assert( mHaveInotify );
#ifdef HAVE_INOTIFY
  int pending = -1;
  char buffer[4096];
  ioctl( mInotifyFd, FIONREAD, &pending );
  while ( pending > 0 ) {
    int size = read( mInotifyFd, buffer, QMIN( pending, (int)sizeof(buffer) ) );
    pending -= size;
    if ( size < 0 )
      break; // error
    int offset = 0;
    while ( size > 0 ) {
      struct inotify_event *event = (struct inotify_event *) &buffer[offset];
      size -= sizeof( struct inotify_event ) + event->len;
      offset += sizeof( struct inotify_event ) + event->len;
      if ( event->mask & IN_OPEN )
        mFileOpen = true;
      if ( event->mask & IN_CLOSE )
        mFileOpen = false;
      if ( event->mask & IN_MODIFY )
        mFileModified = true;
    }
  }
#endif
  mTimer.start( 500, true );

}

void EditorWatcher::editorExited()
{
  mEditorRunning = false;
  mTimer.start( 500, true );
}

void EditorWatcher::checkEditDone()
{
  if ( mEditorRunning || (mFileOpen && mHaveInotify) || mDone )
    return;
  // protect us against double-deletion by calling this method again while
  // the subeventloop of the message box is running
  mDone = true;
  // nobody can edit that fast, we seem to be unable to detect
  // when the editor will be closed
  if ( mEditTime.elapsed() <= 3000 ) {
    KMessageBox::error( mParentWidget,
                        i18n( "KMail is unable to detect when the choosen editor is closed. "
                              "To avoid data loss, editing the attachment will be aborted." ),
                        i18n( "Unable to edit attachment" ) );
  }

  emit editDone( this );
  deleteLater();
}

#include "editorwatcher.moc"
