#include "folderjob.h"

#include "kmmessage.h"
#include "kmfolder.h"

#include <kdebug.h>

namespace KMail {

//----------------------------------------------------------------------------
FolderJob::FolderJob( KMMessage *msg, JobType jt, KMFolder* folder )
  : mType( jt ), mDestFolder( folder ), mPassiveDestructor( false )
{
  if ( msg ) {
    mMsgList.append(msg);
    mSets = msg->headerField("X-UID");
  }
}

//----------------------------------------------------------------------------
FolderJob::FolderJob( QPtrList<KMMessage>& msgList, const QString& sets,
                          JobType jt, KMFolder *folder )
  : mMsgList( msgList ),mType( jt ),
    mSets( sets ), mDestFolder( folder ), mPassiveDestructor( false )
{
}

//----------------------------------------------------------------------------
FolderJob::FolderJob( JobType jt )
  : mType( jt ), mPassiveDestructor( false )
{
}

//----------------------------------------------------------------------------
FolderJob::~FolderJob()
{
  if( !mPassiveDestructor ) {
    emit finished();
  }
}

//----------------------------------------------------------------------------
void
FolderJob::start()
{
  execute();
}

//----------------------------------------------------------------------------
QPtrList<KMMessage>
FolderJob::msgList() const
{
  return mMsgList;
}

}

#include "folderjob.moc"
