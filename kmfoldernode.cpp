// kmfoldernode.cpp

#include "kmfoldernode.h"
#include "kmfolderdir.h"

//-----------------------------------------------------------------------------
KMFolderNode::KMFolderNode(KMFolderDir* aParent, const QString& aName)
//: KMFolderNodeInherited(aParent)
{

  mType = "node";
  mName = aName;
  mParent = aParent;
  mDir = FALSE;

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
  return mDir;
}


//-----------------------------------------------------------------------------
QString KMFolderNode::path() const
{
  if (parent()) return parent()->path();
  return 0;
}

//-----------------------------------------------------------------------------
QString KMFolderNode::label(void) const
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
