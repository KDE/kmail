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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
#ifndef kmfoldernode_h
#define kmfoldernode_h

#include <qobject.h>
#include <qstring.h>
#include <qptrlist.h>

class KMFolderDir;

class KMFolderNode: public QObject
{
  Q_OBJECT

public:
  KMFolderNode( KMFolderDir * parent, const QString & name );
  virtual ~KMFolderNode();

  /** Is it a directory where mail folders are stored or is it a folder that
    contains mail ?
    Note that there are some kinds of mail folders like the type mh uses that
    are directories on disk but are handled as folders here. */
  virtual bool isDir(void) const;
  virtual void setDir(bool aDir) { mDir = aDir; }

  /** Returns ptr to owning directory object or 0 if none. This
    is just a wrapper for convenient access. */
  KMFolderDir* parent(void) const ;
  void setParent( KMFolderDir* aParent );
  //	{ return (KMFolderDir*)KMFolderNodeInherited::parent(); }

  /** Returns full path to the directory where this node is stored or 0
   if the node has no parent. Example: if this object represents a folder
   ~joe/Mail/inbox then path() returns "/home/joe/Mail" and name() returns
   "inbox". */
  virtual QString path() const;

  /** Returns type of the folder (or folder node/dir). This type can be e.g.:
    "in" for folders that have at least one account associated
         (this is hardcoded in KMFolder::type()),
    "out" for the outgoing mail queue of the KMSender object,
    "plain" for an ordinary folder,
    "dir" for a directory,
    "node" for the abstract folder node.
    Other types may follow. */
  virtual const char* type(void) const;
  virtual void setType(const char*);

  /** Name of the node. Also used as file name. */
  QString name() const { return mName; }
  void setName(const QString& aName) { mName = aName; }

  /** Label of the node for visualzation purposes. Default the same as
      the name. */
  virtual QString label() const;

  /** URL of the node for visualization purposes. */
  virtual QString prettyURL() const = 0;

  /** ID of the node */
  uint id() const;
  void setId( uint id ) { mId = id; }

protected:
  QString mName;
  const char* mType;
  KMFolderDir *mParent;
  bool mDir;
  uint mId;
};

typedef QPtrList<KMFolderNode> KMFolderNodeList;


#endif /*kmfoldernode_h*/
