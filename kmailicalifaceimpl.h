/*
    This file is part of KMail.

    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 - 2004 Bo Thorsen <bo@klaralvdalens-datakonsult.se>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#ifndef KMAILICALIFACEIMPL_H
#define KMAILICALIFACEIMPL_H

#include "kmailicalIface.h"
#include "kmfoldertype.h"

#include <kfoldertree.h>

#include <qdict.h>
#include <qguardedptr.h>
#include <qmap.h>

class KMFolder;
class KMMessage;
class KMFolderDir;
class KMFolderTreeItem;

namespace {

// Local helper classes
class ExtraFolder {
public:
  ExtraFolder( KMFolder* f, KMail::FolderContentsType t ) : folder( f ), type( t ) {}
  KMFolder* folder;
  KMail::FolderContentsType type;
};

class Accumulator {
public:
  Accumulator( const QString& t, const QString& f, int c )
  :type( t ), folder( f ), count( c ) {}

  void add( const QString& incidence ) {
    incidences << incidence;
    count--;
  }
  bool isFull() { return count == 0; }

  const QString type;
  const QString folder;
  QStringList incidences;
  int count;
};

}

class KMailICalIfaceImpl : public QObject, virtual public KMailICalIface {
  Q_OBJECT
public:
  KMailICalIfaceImpl();

  bool addIncidence( const QString& type, const QString& folder,
                     const QString& uid, const QString& ical );
  bool deleteIncidence( const QString& type, const QString& folder,
                        const QString& uid );
  QStringList incidences( const QString& type, const QString& folder );

  QStringList subresources( const QString& type );

  bool isWritableFolder( const QString& type, const QString& resource );

  // This saves the iCals/vCards in the entries in the folder.
  // The format in the string list is uid, entry, uid, entry...
  bool update( const QString& type, const QString& folder,
               const QStringList& entries );

  // Update a single entry in the storage layer
  bool update( const QString& type, const QString& folder,
               const QString& uid, const QString& entry );

  // tell KOrganizer about messages to be deleted
  void msgRemoved( KMFolder*, KMMessage* );

  /** Initialize all folders. */
  void initFolders();

  /** Disconnect all slots and close the dirs. */
  void cleanup();

  /**
   * Returns true if resource mode is enabled and folder is one of the
   * resource folders.
   */
  bool isResourceImapFolder( KMFolder* folder ) const;

  /**
   * Returns true if isResourceImapFolder( folder ) returns true, and
   * imap folders should be hidden.
   */
  bool hideResourceImapFolder( KMFolder* folder ) const;

  /**
   * Returns the resource folder type. Other is returned if resource
   * isn't enabled or it isn't a resource folder.
   */
  KFolderTreeItem::Type folderType( KMFolder* folder ) const;

  /**
   * Returns the name of the standard icon for a folder of given type or
   * QString::null if the type is no groupware type.
   */
  QString folderPixmap( KFolderTreeItem::Type type ) const;

  /** Returns the localized name of a folder of given type.
   */
  QString folderName( KFolderTreeItem::Type type, int language = -1 ) const;

  /** Get the folder that holds *type* entries */
  KMFolder* folderFromType( const QString& type, const QString& folder );

  /** Return the ical type of a folder */
  QString icalFolderType( KMFolder* folder ) const;

  /** Find message matching a given UID. */
  KMMessage* findMessageByUID( const QString& uid, KMFolder* folder );

  /** Convenience function to delete a message. */
  static void deleteMsg( KMMessage* msg );

  bool isEnabled() const { return mUseResourceIMAP; }

  /** Called when a folders contents have changed */
  void folderContentsTypeChanged( KMFolder*, KMail::FolderContentsType );

public slots:
  /* (Re-)Read configuration file */
  void readConfig();

  void slotIncidenceAdded( KMFolder* folder, Q_UINT32 sernum );
  void slotIncidenceDeleted( KMFolder* folder, Q_UINT32 sernum );
  void slotRefresh( const QString& type);

private slots:
  void slotRefreshCalendar();
  void slotRefreshTasks();
  void slotRefreshJournals();
  void slotRefreshContacts();
  void slotRefreshNotes();

  void slotCheckDone();
  void slotMessageRetrieved( KMMessage* );

private:
  /** Helper function for initFolders. Initializes a single folder. */
  KMFolder* initFolder( KFolderTreeItem::Type itemType, const char* typeString );

  KMFolder* extraFolder( const QString& type, const QString& folder );

  void loadPixmaps() const;

  QGuardedPtr<KMFolder> mContacts;
  QGuardedPtr<KMFolder> mCalendar;
  QGuardedPtr<KMFolder> mNotes;
  QGuardedPtr<KMFolder> mTasks;
  QGuardedPtr<KMFolder> mJournals;

  // The extra IMAP resource folders
  QDict<ExtraFolder> mExtraFolders;

  // used for collecting incidences during async loading
  QDict<Accumulator> mAccumulators;

  unsigned int mFolderLanguage;

  KMFolderDir* mFolderParentDir;
  KMFolder*    mFolderParent;
  KMFolderType mFolderType;

  // groupware folder icons:
  static QPixmap *pixContacts, *pixCalendar, *pixNotes, *pixTasks, *pixJournals;

  bool mUseResourceIMAP;
  bool mHideFolders;

  /*
   * Bunch of maps to keep track of incidents currently in transfer, ones
   * which need to be ungotten, once we are done, once with updates pending.
   * Since these are transient attributes of only a small but changing number
   * of incidences they are not encapsulated in a struct or somesuch.
   */
  QMap<QString, Q_UINT32> mUIDToSerNum;
  QMap<Q_UINT32, bool> mTheUnGetMes;
  QMap<QString, QString> mPendingUpdates;
  QMap<QString, bool> mInTransit;

};

#endif // KMAILICALIFACEIMPL_H
