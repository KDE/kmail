// kmfoldernode.cpp

#include "kmfoldernode.h"
#include "kmfolderdir.h"

static QString sEmpty("");

//-----------------------------------------------------------------------------
KMFolderNode::KMFolderNode(KMFolderDir* aParent, const char* aName):
  KMFolderNodeInherited(aParent)
{
  initMetaObject();

  mType = "node";
  mName = aName;
  mName.detach();
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
const QString& KMFolderNode::path(void) const
{
  if (parent()) return parent()->path();
  return sEmpty;
}


//-----------------------------------------------------------------------------
const QString KMFolderNode::label(void) const
{
  return name();
}

#include "kmfoldernode.moc"
