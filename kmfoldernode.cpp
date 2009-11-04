// kmfoldernode.cpp

#include "kmfoldernode.h"

//-----------------------------------------------------------------------------
KMFolderNode::KMFolderNode( /*KMFolderDir * parent,*/ const QString & name )
  : mName( name ),
    mDir( false ),
    mId( 0 )
{
}


//-----------------------------------------------------------------------------
KMFolderNode::~KMFolderNode()
{
}

//-----------------------------------------------------------------------------
bool KMFolderNode::isDir(void) const
{
  return mDir;
}


//-----------------------------------------------------------------------------
QString KMFolderNode::path() const
{
  return QString();
}

//-----------------------------------------------------------------------------
QString KMFolderNode::label(void) const
{
  return name();
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
