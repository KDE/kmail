// kmfoldernode.cpp

#include "kmfoldernode.h"
#include "kmfolderdir.h"


//-----------------------------------------------------------------------------
KMFolderNode::KMFolderNode(KMFolderDir* aParent, const QCString& aName):
  KMFolderNodeInherited(aParent)
{
  initMetaObject();

  mType = "node";
  mName = aName;
  
  setName(mName);
}


//-----------------------------------------------------------------------------
KMFolderNode::~KMFolderNode()
{
}


//-----------------------------------------------------------------------------
const char* KMFolderNode::type(void) const
{
  return mType;
}


//-----------------------------------------------------------------------------
void KMFolderNode::setType(const char* aType)
{
  mType = aType;
}


//-----------------------------------------------------------------------------
bool KMFolderNode::isDir(void) const
{
  return FALSE;
}


//-----------------------------------------------------------------------------
const QCString KMFolderNode::path(void) const
{
  if (parent()) return parent()->path();
  return 0;
}


//-----------------------------------------------------------------------------
const QString KMFolderNode::label(void) const
{
  return name();
}

#include "kmfoldernode.moc"
