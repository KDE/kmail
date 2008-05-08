/* -*- mode: C++; c-file-style: "gnu" -*-
    This file is part of KMail, the KDE mail client.
    Copyright (c) 1996-1998 Stefan Taferner <taferner@kde.org>

    KMail is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License, version 2, as
    published by the Free Software Foundation.

    KMail is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "kmfolderindex.h"
#include "kmfolder.h"
#include <config-kmail.h>

#include <kdebug.h>
#include <kde_file.h>

#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QByteArray>
#include <QDateTime>

#include <unistd.h>

// Current version of the table of contents (index) files
#define INDEX_VERSION 1506

#ifndef MAX_LINE
#define MAX_LINE 4096
#endif

#ifndef INIT_MSGS
#define INIT_MSGS 8
#endif

#include <errno.h>
#include <assert.h>
#include <utime.h>
#include <fcntl.h>

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif
#include <QCursor>
#include <kmessagebox.h>
#include <klocale.h>
#include "kmmsgdict.h"
#include "kcursorsaver.h"

// We define functions as kmail_swap_NN so that we don't get compile errors
// on platforms where bswap_NN happens to be a function instead of a define.

/* Swap bytes in 32 bit value.  */
#ifdef bswap_32
#define kmail_swap_32(x) bswap_32(x)
#else
#define kmail_swap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

int KMFolderIndex::openInternal( OpenInternalOptions options )
{
  int rc = 0;
  if ( folder()->path().isEmpty() )
  {
    mAutoCreateIndex = false;
    rc = createIndexFromContents();
  }
  else {
    bool shouldCreateIndexFromContents = false;
    KMFolderIndex::IndexStatus index_status = indexStatus();
    if ( KMFolderIndex::IndexOk != index_status ) // test if contents file has changed
    {
      if ( ( options & CheckIfIndexTooOld ) && KMFolderIndex::IndexTooOld == index_status ) {
         QString msg = i18n("<qt><p>The index of folder '%2' seems "
                            "to be out of date. To prevent message "
                            "corruption the index will be "
                            "regenerated. As a result deleted "
                            "messages might reappear and status "
                            "flags might be lost.</p>"
                            "<p>Please read the corresponding entry "
                            "in the <a href=\"%1\">FAQ section of the manual "
                            "of KMail</a> for "
                            "information about how to prevent this "
                            "problem from happening again.</p></qt>",
                            QString("help:/kmail/faq.html#faq-index-regeneration"),
                            objectName());
        // When KMail is starting up we have to show a non-blocking message
        // box so that the initialization can continue. We don't show a
        // queued message box when KMail isn't starting up because queued
        // message boxes don't have a "Don't ask again" checkbox.
        if ( kmkernel->startingUp() ) {
          KConfigGroup configGroup( KMKernel::config(),
                                    "Notification Messages" );
          bool showMessage =
            configGroup.readEntry( "showIndexRegenerationMessage", true );
          if ( showMessage ) {
            KMessageBox::queuedMessageBox( 0, KMessageBox::Information,
                                           msg, i18n("Index Out of Date"),
                                           KMessageBox::AllowLink );
          }
        } else {
          KCursorSaver idle( KBusyPtr::idle() );
          KMessageBox::information( 0, msg, i18n("Index Out of Date"),
                                    "showIndexRegenerationMessage",
                                    KMessageBox::AllowLink );
        }
      } // IndexTooOld
#ifdef KMAIL_SQLITE_INDEX
#else
      mIndexStream = 0;
#endif
      shouldCreateIndexFromContents = true;
      emit statusMsg( i18n("Folder `%1' changed; recreating index.", objectName()) );
    } else {
#ifdef KMAIL_SQLITE_INDEX
      if ( !openDatabase( SQLITE_OPEN_READWRITE ) )
        shouldCreateIndexFromContents = true;
#else
      mIndexStream = KDE_fopen(QFile::encodeName(indexLocation()), "r+"); // index file
      kDebug( StorageDebug ) << "KDE_fopen(indexLocation()=" << indexLocation() << ", \"r+\") == mIndexStream == " << mIndexStream;
      if ( mIndexStream ) {
# ifndef Q_WS_WIN
        fcntl(fileno(mIndexStream), F_SETFD, FD_CLOEXEC);
# endif
        if (!updateIndexStreamPtr())
          return 1;
      }
      else
        shouldCreateIndexFromContents = true;
#endif
    }

    if ( shouldCreateIndexFromContents )
      rc = createIndexFromContents();
    else {
      rc = readIndex() ? 0 : 1;
      if ( rc != 0 && ( options & CreateIndexFromContentsWhenReadIndexFailed ) )
        rc = createIndexFromContents();
    }
  } // !folder()->path().isEmpty()

  mChanged = false;
  return rc;
}

int KMFolderIndex::createInternal()
{
  if ( folder()->path().isEmpty() ) {
    mAutoCreateIndex = false;
  }
  else {
/* js: not needed! writeIndex() does this... */

// I've re-enabled this (for Linux), because writeIndex() only does this when
// writing the temporary index did not fail. In case of an error in writeIndex(),
// it returns early without doing this, so better be safe and do it here as well.
//    --tmcguire
#ifdef KMAIL_SQLITE_INDEX
    //bool ok = openDatabase( SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE );
#else
    int old_umask = umask( 077 );
    mIndexStream = KDE_fopen( QFile::encodeName(indexLocation() ), "w+" );
    const bool updateIndexStreamPtrResult = updateIndexStreamPtr( true );
    umask( old_umask );
    if ( !updateIndexStreamPtrResult )
      return 1;

    if ( !mIndexStream )
      return errno;
# ifndef Q_WS_WIN
    fcntl( fileno( mIndexStream ), F_SETFD, FD_CLOEXEC );
# endif
#endif
  }

  mOpenCount++;
  mChanged = false;

  return writeIndex();
}
