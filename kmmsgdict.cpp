/* kmail message dictionary */
/* Author: Ronen Tzur <rtzur@shani.net> */

#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmmsgdict.h"

#include <qfileinfo.h>

#include <kdebug.h>

#include <stdio.h>
#include <unistd.h>

#include <errno.h>

//-----------------------------------------------------------------------------

// Current version of the .index.ids files
#define IDS_VERSION 1000

// The asterisk at the end is important
#define IDS_HEADER "# KMail-Index-IDs V%d\n*"

//-----------------------------------------------------------------------------

class KMMsgDictEntry
{
public:
  KMMsgDictEntry(const KMFolder *aFolder, int aIndex)
    { folder = aFolder; index = aIndex; }

  KMMsgDictEntry(const KMMsgDictEntry *a)
    { folder = a->folder; index = a->index; }

  const KMFolder *folder;
  int index;
};

//-----------------------------------------------------------------------------

class KMMsgDictREntry
{
public:
  KMMsgDictREntry(int size = 0)
  {
    array = new QMemArray<ulong>(size);
    fp = NULL;
  }
  
  ~KMMsgDictREntry()
  {
    delete array;
    if (fp)
      fclose(fp);
  }
  
  void set(int index, ulong msn)
  {
    if (array && index >= 0) {
      int size = array->size();
      if (index >= size) {
        int newsize = QMAX(size + 25, index + 1);
        array->resize(newsize);
        for (int j = size; j < newsize; j++)
          array->at(j) = 0;
      }
      array->at(index) = msn;
    }
  }
  
  ulong get(int index)
  {
    if (array && index >= 0 && (unsigned)index < array->size())
      return array->at(index);
    return 0;
  }
  
  int getRealSize()
  {
    int count = array->size() - 1;
    while (count >= 0) {
      if (array->at(count))
        break;
      count--;
    }
    return count + 1;
  }
  
  void sync()
  {
    fflush(fp);
  }
  
public:
  QMemArray<ulong> *array;
  FILE *fp;
};

//-----------------------------------------------------------------------------

KMMsgDict::KMMsgDict()
{
  dict = new QIntDict<KMMsgDictEntry>(997);
  dict->setAutoDelete(true);

  rdict = new QPtrDict<KMMsgDictREntry>;
  rdict->setAutoDelete(true);

  nextMsgSerNum = 1;
  kernel->folderMgr()->readMsgDict(this);
  kernel->imapFolderMgr()->readMsgDict(this);
}

//-----------------------------------------------------------------------------

KMMsgDict::~KMMsgDict()
{
  kernel->folderMgr()->writeMsgDict(this);
  kernel->imapFolderMgr()->writeMsgDict(this);
  delete dict;
  delete rdict;
}

//-----------------------------------------------------------------------------

unsigned long KMMsgDict::getNextMsgSerNum() {
  unsigned long msn = nextMsgSerNum;
  nextMsgSerNum++;
  return msn;
}

//-----------------------------------------------------------------------------

unsigned long KMMsgDict::insert(unsigned long msgSerNum,
                                const KMMsgBase *msg, int index)
{
  unsigned long msn = msgSerNum;
  if (!msn)
    msn = getNextMsgSerNum();
  else {
    if (msn >= nextMsgSerNum)
      nextMsgSerNum = msn + 1;
  }
  
  KMFolder *folder = msg->parent();
  if (folder && index == -1)
    index = folder->find((const KMMsgBasePtr)msg);
  
  KMMsgDictEntry *entry = new KMMsgDictEntry(folder, index);
  dict->replace((long)msn, entry);
  
  KMMsgDictREntry *rentry = rdict->find(folder);
  if (!rentry) {
    rentry = new KMMsgDictREntry();
    rdict->insert((void *)folder, rentry);
  }
  rentry->set(index, msn);
  
  return msn;
}

unsigned long KMMsgDict::insert(const KMMsgBase *msg, int index)
{
  unsigned long msn = msg->getMsgSerNum();
  return insert(msn, msg, index);
}

//-----------------------------------------------------------------------------

void KMMsgDict::remove(unsigned long msgSerNum)
{
  long key = (long)msgSerNum;
  KMMsgDictEntry *entry = dict->find(key);
  if (!entry)
    return;
  
  KMMsgDictREntry *rentry = rdict->find((void *)entry->folder);
  if (rentry)
    rentry->set(entry->index, 0);
  
  dict->remove((long)key);
}

unsigned long KMMsgDict::remove(const KMMsgBase *msg)
{
  unsigned long msn = msg->getMsgSerNum();
  remove(msn);
  return msn;
}

//-----------------------------------------------------------------------------

void KMMsgDict::getLocation(unsigned long key,
                            KMFolder **retFolder, int *retIndex)
{
  KMMsgDictEntry *entry = dict->find((long)key);
  if (entry) {
    *retFolder = (KMFolder *)entry->folder;
    *retIndex = entry->index;
  } else {
    *retFolder = 0;
    *retIndex = 0;
  }
}

void KMMsgDict::getLocation(const KMMsgBase *msg,
                            KMFolder **retFolder, int *retIndex)
{
  getLocation(msg->getMsgSerNum(), retFolder, retIndex);
}

//-----------------------------------------------------------------------------

unsigned long KMMsgDict::getMsgSerNum(KMFolder *folder, int index)
{
  unsigned long msn = 0;
  KMMsgDictREntry *rentry = rdict->find((void *)folder);
  if (rentry)
    msn = rentry->get(index);
  return msn;
}

//-----------------------------------------------------------------------------

QString KMMsgDict::getFolderIdsLocation(const KMFolder *folder) const
{
  return folder->indexLocation() + ".ids";
}

//-----------------------------------------------------------------------------

bool KMMsgDict::isFolderIdsOutdated(const KMFolder *folder)
{
  bool outdated = false;
  
  QFileInfo indexInfo(folder->indexLocation());
  QFileInfo idsInfo(getFolderIdsLocation(folder));
  
  if (!indexInfo.exists() || !idsInfo.exists())
    outdated = true;
  if (indexInfo.lastModified() > idsInfo.lastModified())
    outdated = true;
  
  return outdated;
}

//-----------------------------------------------------------------------------

int KMMsgDict::readFolderIds(const KMFolder *folder)
{
  if (isFolderIdsOutdated(folder))
    return -1;
  
  QString filename = getFolderIdsLocation(folder);
  FILE *fp = fopen(filename.local8Bit(), "r+");
  if (!fp)
    return -1;
  
  int version = 0;
  fscanf(fp, IDS_HEADER, &version);
  if (version < IDS_VERSION) {
    fclose(fp);
    return -1;
  }
  
  int count;
  if (!fread(&count, sizeof(count), 1, fp)) {
    fclose(fp);
    return -1;
  }

  KMMsgDictREntry *rentry = new KMMsgDictREntry(count);

  for (int index = 0; index < count; index++) {
    unsigned long msn;

    if (!fread(&msn, sizeof(msn), 1, fp)) {
      for (int i = 0; i < index; i++) {
        msn = rentry->get(i);
        dict->remove((long)msn);
      }
      delete rentry;
      fclose(fp);
      return -1;
    }
    
    //if (!msn)
      //kdDebug(5006) << "Dict found zero serial number in folder " << folder->label() << endl;
    
    KMMsgDictEntry *entry = new KMMsgDictEntry(folder, index);
    dict->insert((long)msn, entry);
    if (msn >= nextMsgSerNum)
      nextMsgSerNum = msn + 1;

    rentry->set(index, msn);
  }

  fclose(fp);
  rdict->insert((void *)folder, rentry);

  return 0;
}

//-----------------------------------------------------------------------------

KMMsgDictREntry *KMMsgDict::openFolderIds(const KMFolder *folder)
{
  KMMsgDictREntry *rentry = rdict->find((void *)folder);
  if (!rentry) {
    rentry = new KMMsgDictREntry();
    rdict->insert((void *)folder, rentry);
  }
  
  if (!rentry->fp) {
    QString filename = getFolderIdsLocation(folder);
    rentry->fp = fopen(filename.local8Bit(), "r+");
    if (!rentry->fp)
      rentry->fp = fopen(filename.local8Bit(), "w+");
  }
  
  if (rentry->fp) {
    FILE *fp = rentry->fp;
    rewind(fp);
    
    fprintf(fp, IDS_HEADER, IDS_VERSION);
    if (ferror(fp))
      rentry = 0;
      
  } else {
    kdDebug(5006) << "Dict cannot open with folder " << folder->label() << ": "
                  << strerror(errno) << " (" << errno << ")" << endl;
    rentry = 0;
  }
    
  return rentry;
}

//-----------------------------------------------------------------------------

int KMMsgDict::writeFolderIds(const KMFolder *folder)
{
  KMMsgDictREntry *rentry = openFolderIds(folder);
  if (!rentry)
    return 0;
  FILE *fp = rentry->fp;

  // kdDebug(5006) << "Dict writing for folder " << folder->label() << endl;
  
  int count = rentry->getRealSize();
  if (!fwrite(&count, sizeof(count), 1, fp)) {
    kdDebug(5006) << "Dict cannot write count with folder " << folder->label() << ": "
                  << strerror(errno) << " (" << errno << ")" << endl;
    return -1;
  }
  
  for (int index = 0; index < count; index++) {
    unsigned long msn = rentry->get(index);
    if (!fwrite(&msn, sizeof(msn), 1, fp))
      return -1;
  }
  
  rentry->sync();

  long eof = ftell(fp);
  QString filename = getFolderIdsLocation(folder);
  truncate(filename.local8Bit(), eof);
  
  return 0;
}

//-----------------------------------------------------------------------------

int KMMsgDict::touchFolderIds(const KMFolder *folder)
{
  KMMsgDictREntry *rentry = openFolderIds(folder);
  if (rentry)
    rentry->sync();
  return 0;
}

//-----------------------------------------------------------------------------

int KMMsgDict::appendtoFolderIds(const KMFolder *folder, int index)
{
  KMMsgDictREntry *rentry = openFolderIds(folder);
  if (!rentry)
    return 0;
  FILE *fp = rentry->fp;
  
//  kdDebug(5006) << "Dict appending for folder " << folder->label() << endl;
  
  int count;
  if (!fread(&count, sizeof(count), 1, fp)) {
    kdDebug(5006) << "Dict cannot read count for folder " << folder->label() << ": "
                  << strerror(errno) << " (" << errno << ")" << endl;
    return 0;
  }
  count++;
  fseek(fp, -4, SEEK_CUR);
  if (!fwrite(&count, sizeof(count), 1, fp)) {
    kdDebug(5006) << "Dict cannot write count for folder " << folder->label() << ": "
                  << strerror(errno) << " (" << errno << ")" << endl;
    return 0;
  }
  
  long ofs = (count - 1) * sizeof(ulong);
  if (ofs > 0)
    fseek(rentry->fp, ofs, SEEK_CUR);

  ulong msn = rentry->get(index);
  if (!fwrite(&msn, sizeof(msn), 1, rentry->fp)) {
    kdDebug(5006) << "Dict cannot write count for folder " << folder->label() << ": "
                  << strerror(errno) << " (" << errno << ")" << endl;
    return 0;
  }
  
  rentry->sync();
  
  return 0;
}

//-----------------------------------------------------------------------------

bool KMMsgDict::hasFolderIds(const KMFolder *folder)
{
  KMMsgDictREntry *rentry = rdict->find((void *)folder);
  return rentry != 0;
}

//-----------------------------------------------------------------------------

bool KMMsgDict::removeFolderIds(const KMFolder *folder)
{
  KMMsgDictREntry *rentry = rdict->find((void *)folder);
  if (rentry)
    rdict->remove(rentry);
  QString filename = getFolderIdsLocation(folder);
  return unlink(filename.local8Bit());
}
