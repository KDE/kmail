// kmfoldernode.cpp

#include "kmfoldernode.h"
#include "kmfolderdir.h"

static QString sEmpty("");

//-----------------------------------------------------------------------------
KMFolderNode::KMFolderNode(KMFolderDir* aParent, const char* aName):
  KMFolderNodeInherited(aParent)
{
  initMetaObject();

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
const QString& KMFolderNode::path(void) const
{
  if (parent()) return parent()->path();
  return sEmpty;
}

#include "kmfoldernode.moc"
