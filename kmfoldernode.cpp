// kmfoldernode.cpp

#include "kmfoldernode.h"
#include "kmfolderdir.h"


//-----------------------------------------------------------------------------
KMFolderNode::KMFolderNode(KMFolderDir* aParent, const char* aName):
  KMFolderNodeInherited(aParent)
{
  mName = aName;
  mName.detach();
  setName(mName);
}


//-----------------------------------------------------------------------------
KMFolderNode::~KMFolderNode()
{
}


//-----------------------------------------------------------------------------
bool KMFolderNode::isDir(void) const
{
  return FALSE;
}


//-----------------------------------------------------------------------------
const QString KMFolderNode::path(void) const
{
  static QString p;

  if (parent())
  {
    p = parent()->path();
    p.append("/");
    p.append(parent()->name());
  }
  else p = "";

  return p;
}
