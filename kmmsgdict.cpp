/* kmail message dictionary */
/* Author: Ronen Tzur <rtzur@shani.net> */

#include "kmfolder.h"
#include "kmfoldermgr.h"
#include "kmkernel.h"
#include "kmmsgbase.h"
#include "kmmsgdict.h"

#include <stdio.h>
#include <unistd.h>

//-----------------------------------------------------------------------------

// Current version of the .index.ids files
#define IDS_VERSION 1000

// The asterisk at the end is important
#define IDS_HEADER "# KMail-Index-IDs V%d\n*"

//-----------------------------------------------------------------------------

class KMMsgDictEntry
{
public:
  KMMsgDictEntry(KMFolder *aFolder, int aIndex)
    { folder = aFolder; index = aIndex; }

  KMMsgDictEntry(const KMMsgDictEntry *a)
    { folder = a->folder; index = a->index; }

  KMFolder *folder;
  int index;
};

//-----------------------------------------------------------------------------

KMMsgDict::KMMsgDict()
{
  dict = new QIntDict<KMMsgDictEntry>(997);
  dict->setAutoDelete(true);

  rdict = new QPtrDict<QMemArray<unsigned long> >;

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
                                const KMMsgBase *msg, int index = -1)
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
  
  if (folder) {
    QMemArray<unsigned long> *array = rdict->find(folder);
    if (!array) {
      array = new QMemArray<unsigned long>(25);
      rdict->insert(folder, array);
      initArray(array);
    }
    int size = array->size();
    if (index >= size) {
      array->resize(QMAX(size + 25, index + 1));
      initArray(array, size);
    }
    array->at(index) = msn;
  }
  
  return msn;
}

unsigned long KMMsgDict::insert(const KMMsgBase *msg, int index = -1)
{
  unsigned long msn = msg->getMsgSerNum();
  return insert(msn, msg, index);
}

//-----------------------------------------------------------------------------

void KMMsgDict::remove(unsigned long msgSerNum)
{
  long key = (long)msgSerNum;
  KMMsgDictEntry *entry = dict->find(key);
  
  QMemArray<unsigned long> *array = rdict->find(entry->folder);
  if (array)
    array->at(entry->index) = 0;
  
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
    *retFolder = entry->folder;
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
  QMemArray<unsigned long> *array = rdict->find(folder);
  if (array && index >= 0 && index < array->size())
    msn = array->at(index);
  return msn;
}

//-----------------------------------------------------------------------------

int KMMsgDict::readFolderIds(KMFolder *folder)
{
  QString filename = folder->indexLocation() + ".ids";
  FILE *fp = fopen(filename.local8Bit(), "r");
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

  QMemArray<unsigned long> *array = new QMemArray<unsigned long>(count);

  for (int index = 0; index < count; index++) {
    unsigned long msn;
    if (!fread(&msn, sizeof(msn), 1, fp)) {
      for (int i = 0; i < index; i++) {
        msn = array->at(i);
        dict->remove((long)msn);
      }
      delete array;
      fclose(fp);
      return -1;
    }

    KMMsgDictEntry *entry = new KMMsgDictEntry(folder, index);
    dict->insert((long)msn, entry);
    if (msn >= nextMsgSerNum)
      nextMsgSerNum = msn + 1;
    array->at(index) = msn;
  }

  rdict->insert(folder, array);
  
  fclose(fp);
  unlink(filename.local8Bit());
  return 0;
}

//-----------------------------------------------------------------------------

int KMMsgDict::writeFolderIds(KMFolder *folder)
{
  QString filename = folder->indexLocation() + ".ids";
  FILE *fp = fopen(filename.local8Bit(), "w");
  if (!fp)
    return -1;
  
  fprintf(fp, IDS_HEADER, IDS_VERSION);
  
  QMemArray<unsigned long> *array = rdict->find(folder);
  int count = 0;
  if (array) {
    count = array->size() - 1;
    while (count >= 0) {
      if (array->at(count))
        break;
      count--;
    }
    count++;
  }
  
  if (!fwrite(&count, sizeof(count), 1, fp)) {
    fclose(fp);
    unlink(filename.local8Bit());
    return -1;
  }
  
  for (int index = 0; index < count; index++) {
    unsigned long msn = array->at(index);
    if (!fwrite(&msn, sizeof(msn), 1, fp)) {
      fclose(fp);
      unlink(filename.local8Bit());
      return -1;
    }
  }
  
  fclose(fp);
  return 0;
}

//-----------------------------------------------------------------------------

bool KMMsgDict::hasFolderIds(KMFolder *folder)
{
  QMemArray<unsigned long> *array = rdict->find(folder);
  return (array != 0);
}

//-----------------------------------------------------------------------------

void KMMsgDict::initArray(QMemArray<unsigned long> *array, int start = 0)
{
  int size = array->size();
  for (int index = start; index < size; index++)
    array->at(index) = 0;
}
