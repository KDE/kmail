/* KMail list that manages the contents of one directory that may
 * contain folders and/or other directories.
 */
#ifndef kmfolderdir_h
#define kmfolderdir_h

#include <qstring.h>
#include "kmfoldernode.h"

class KMFolderDir: public KMFolderNode, public KMFolderNodeList
{
public:
  KMFolderDir(KMFolderDir* parent=NULL, const char* path=NULL);
  virtual ~KMFolderDir();

  virtual bool isDir(void) const { return TRUE; }

  /** Read contents of directory */
  virtual bool reload(void);
};


//-----------------------------------------------------------------------------

class KMFolderRootDir: public KMFolderDir
{
public:
  KMFolderRootDir(const char* name=NULL);
  virtual const QString path(void) const; // returns name() here

  // set the absolute path
  virtual void setPath(const char*);

protected:
  QString mPath;
};

#endif /*kmfolderdir_h*/
