#include <qguardedptr.h>

#include <kdebug.h>

#include "kmmainwin.h"
#include "kmmessage.h"
#include "kmfoldertree.h"
#include "kmscoring.h"

KMScorableArticle::KMScorableArticle(const QCString &msg)
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
  : mConfDialog(0),
    mMainWin(0)
{
}

KMScoringManager::~KMScoringManager()
{
}

class KMFolder;

void
KMScoringManager::setMainWin(QObject *parent)
{
  mMainWin = dynamic_cast<KMMainWin*>(parent);
  if (!mMainWin) {
    kdDebug(5006) << "KMScoringManager::setMainWin() : mMainWin == 0" << endl;
  }
}


QStringList
KMScoringManager::getGroups() const
{
  QValueList<QGuardedPtr<KMFolder> > dummy;
    
  QStringList res;

  if (mMainWin) {
    KMFolderTree *tree = mMainWin->folderTree();
    if (tree)
      tree->createFolderList( &res, &dummy );
  }
  
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
#include "kmscoring.moc"
