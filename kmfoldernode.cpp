// kmfoldernode.cpp

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kmfolderdir.h"

//-----------------------------------------------------------------------------
KMFolderNode::KMFolderNode(KMFolderDir* aParent, const QString& aName)
//: KMFolderNodeInherited(aParent)
{
  mType = "node";
  mName = aName;
  mParent = aParent;
  mDir = FALSE;
  mId = 0;
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

//-----------------------------------------------------------------------------
uint KMFolderNode::id() const
{
  if (mId > 0)
    return mId;
  // compatibility, returns 0 on error
  return name().toUInt();
}

#include "kmfoldernode.moc"
