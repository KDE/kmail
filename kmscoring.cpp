#include <qguardedptr.h>

#include <kdebug.h>

#include "kmmessage.h"
#include "kmfoldermgr.h"
#include "kmkernel.h"
#include "kmscoring.h"

KMScorableArticle::KMScorableArticle(const QString &msg)
  : mMsgStr(msg),
    mScore(0)
{
}

KMScorableArticle::~KMScorableArticle()
{
}


void
KMScorableArticle::addScore(short s)
{
  mScore += s;
}

QString
KMScorableArticle::from() const
{
  return getHeaderByType("From:");
}

QString
KMScorableArticle::subject() const
{
  return getHeaderByType("Subject:");
}

QString
KMScorableArticle::getHeaderByType(const QString &header) const
{
  if (mParsedHeaders.contains(header)) {
    return mParsedHeaders[header];
  }
  
  QString value;
  
  int start = 0, stop = 0;
  char ch = 0;

  start = mMsgStr.find(header);
  if (start == -1) return QString::null;
  if (mMsgStr.find("\n\n") < start) return QString::null;
  stop = mMsgStr.find("\n", start + 1);
  while (stop != -1 && (ch = mMsgStr.at(stop + 1)) == ' ' || ch == '\t')
    stop = mMsgStr.find("\n", stop + 1);
  if (stop == -1) value = KMMsgBase::decodeRFC2047String(mMsgStr.mid(start));
  else value = KMMsgBase::decodeRFC2047String(mMsgStr.mid(start,
                                                          stop - start));

  value.remove(0, header.length());
  value = value.stripWhiteSpace();

  mParsedHeaders[header] = value;

  return value;
  
}

//////////////////////////////

KMScorableGroup::KMScorableGroup()
{
}

KMScorableGroup::~KMScorableGroup()
{
}

//////////////////////////////

KMScoringManager::KMScoringManager()
    : mConfDialog(0)
{
}

KMScoringManager::~KMScoringManager()
{
}

class KMFolder;

QStringList
KMScoringManager::getGroups() const
{
  QValueList<QGuardedPtr<KMFolder> > dummy;
    
  QStringList res;
  kernel->folderMgr()->createFolderList( &res, &dummy );
  
  return res;
}


void
KMScoringManager::configure()
{
}

void
KMScoringManager::slotDialogDone()
{
}


KMScoringManager*
KMScoringManager::globalScoringManager()
{
  if ( !mScoringManager ) mScoringManager = new KMScoringManager;

  return mScoringManager;
}

KMScoringManager*
KMScoringManager::mScoringManager = 0;
