/*
    This file is part of KMail.

    Copyright (c) 2003 Steffen Hansen <steffen@klaralvdalens-datakonsult.se>
    Copyright (c) 2003 Bo Thorsen <bo@klaralvdalens-datakonsult.se>

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
*/

#ifndef KMAILICALIFACEIMPL_H
#define KMAILICALIFACEIMPL_H

#include "kmailicalIface.h"

#include <kfoldertree.h>

#include "kmfoldertype.h"

class KMFolder;
class KMMessage;
class KMFolderDir;
class KMFolderTreeItem;


class KMailICalIfaceImpl : public QObject, virtual public KMailICalIface {
  Q_OBJECT
public:
  KMailICalIfaceImpl();

  virtual bool addIncidence( const QString& folder, const QString& uid, 
			     const QString& ical );
  virtual bool deleteIncidence( const QString& folder, const QString& uid );
  virtual QStringList incidences( const QString& folder );

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
   * Returns the resource folder type. Other is returned if resource
   * isn't enabled or it isn't a resource folder.
   */
  KFolderTreeItem::Type folderType( KMFolder* folder ) const;

  bool setFolderPixmap(const KMFolder& folder, KMFolderTreeItem& fti) const;

  /** Return the localized hame of a folder type. */
  QString folderName( KFolderTreeItem::Type type, int language = -1 ) const;

  /** Get the folder that holds *type* entries */
  KMFolder* folderFromType( const QString& type );

  /** Return the ical type of a folder */
  QString icalFolderType( KMFolder* folder ) const;

  /* (Re-)Read configuration file */
  void readConfig();

  /** Find message matching a given UID. */
  static KMMessage* findMessageByUID( const QString& uid, KMFolder* folder );

  /** Convenience function to delete a message. */
  static void deleteMsg( KMMessage* msg );

public slots:
  void slotIncidenceAdded( KMFolder* folder, const QString& ical );
  void slotIncidenceDeleted( KMFolder* folder, const QString& uid );
  void slotRefresh( const QString& type);

private:
  /** Helper function for initFolders. Initializes a single folder. */
  KMFolder* initFolder( KFolderTreeItem::Type itemType, const char* typeString );

  void loadPixmaps() const;

  KMFolder* mContacts;
  KMFolder* mCalendar;
  KMFolder* mNotes;
  KMFolder* mTasks;
  KMFolder* mJournals;

  unsigned int mFolderLanguage;

  KMFolderDir* mFolderParent;
  KMFolderType mFolderType;

  // groupware folder icons:
  static QPixmap *pixContacts, *pixCalendar, *pixNotes, *pixTasks;

  bool mUseResourceIMAP;
};

#endif // KMAILICALIFACEIMPL_H
