/* Basic Node for folder directory tree. Childs are KMFolder and KMFolderDir.
 * The owner of such nodes are usually objects of type KMFolderDir
 *
 * Author: Stefan Taferner <taferner@alpin.or.at>
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
  KMFolderNode(KMFolderDir* parent, const QString& name);
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
  virtual QString label(void) const;

protected:
  QString mName;
  const char* mType;
  KMFolderDir *mParent;
  bool mDir;
};

typedef QPtrList<KMFolderNode> KMFolderNodeList;


#endif /*kmfoldernode_h*/
