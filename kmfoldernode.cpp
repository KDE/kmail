// kmfoldernode.cpp

#include "kmfoldernode.h"
#include "kmfolderdir.h"

//-----------------------------------------------------------------------------
KMFolderNode::KMFolderNode(KMFolderDir* aParent, const QString& aName)
//: KMFolderNodeInherited(aParent)
{
  initMetaObject();

  mType = "node";
  mName = aName;
  mParent = aParent;
  
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
const QString KMFolderNode::path() const
{
  if (parent()) return parent()->path();
  return 0;
}

//-----------------------------------------------------------------------------
const QString KMFolderNode::label(void) const
{
  return name();
}

//-----------------------------------------------------------------------------
KMFolderDir* KMFolderNode::parent(void) const
{
  return mParent;
}

//-----------------------------------------------------------------------------
void KMFolderNode::setParent( KMFolderDir* aParent )
{
  mParent = aParent;
}

#include "kmfoldernode.moc"
