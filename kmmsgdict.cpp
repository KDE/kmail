/* kmail message dictionary */
/* Author: Ronen Tzur <rtzur@shani.net> */

#include "kmfolderindex.h"
#include "kmfolder.h"
#include "kmmsgdict.h"
#include "kmdict.h"
#include "globalsettings.h"

#include <qfileinfo.h>

#include <kdebug.h>

#include <stdio.h>
#include <unistd.h>

#include <errno.h>

#include <config.h>

#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#endif

// We define functions as kmail_swap_NN so that we don't get compile errors
// on platforms where bswap_NN happens to be a function instead of a define.

/* Swap bytes in 32 bit value.  */
#ifdef bswap_32
#define kmail_swap_32(x) bswap_32(x)
#else
#define kmail_swap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) |		      \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif


//-----------------------------------------------------------------------------

// Current version of the .index.ids files
#define IDS_VERSION 1002

// The asterisk at the end is important
#define IDS_HEADER "# KMail-Index-IDs V%d\n*"

//-----------------------------------------------------------------------------

class KMMsgDictEntry : public KMDictItem
{
public:
  KMMsgDictEntry(const KMFolder *aFolder, int aIndex)
    { folder = aFolder; index = aIndex; }

  const KMFolder *folder;
  int index;
};

//-----------------------------------------------------------------------------

class KMMsgDictREntry
{
public:
  KMMsgDictREntry(int size = 0)
  {
    array.resize(size);
    for (int i = 0; i < size; i++)
      array.at(i) = 0;
    fp = 0;
    swapByteOrder = false;
    baseOffset = 0;
  }

  ~KMMsgDictREntry()
  {
    array.resize(0);
    if (fp)
      fclose(fp);
  }

  void set(int index, KMMsgDictEntry *entry)
  {
    if (index >= 0) {
      int size = array.size();
      if (index >= size) {
        int newsize = QMAX(size + 25, index + 1);
        array.resize(newsize);
        for (int j = size; j < newsize; j++)
          array.at(j) = 0;
      }
      array.at(index) = entry;
    }
  }

  KMMsgDictEntry *get(int index)
  {
    if (index >= 0 && (unsigned)index < array.size())
      return array.at(index);
    return 0;
  }

  ulong getMsn(int index)
  {
    KMMsgDictEntry *entry = get(index);
    if (entry)
      return entry->key;
    return 0;
  }

  int getRealSize()
  {
    int count = array.size() - 1;
    while (count >= 0) {
      if (array.at(count))
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
  QMemArray<KMMsgDictEntry *> array;
  FILE *fp;
  bool swapByteOrder;
  off_t baseOffset;
};


//-----------------------------------------------------------------------------

KMMsgDict::KMMsgDict()
{
  int lastSizeOfDict = GlobalSettings::msgDictSizeHint();
  lastSizeOfDict = ( lastSizeOfDict * 11 ) / 10;
  GlobalSettings::setMsgDictSizeHint( 0 );
  dict = new KMDict( lastSizeOfDict );
  nextMsgSerNum = 1;
}

//-----------------------------------------------------------------------------

KMMsgDict::~KMMsgDict()
{
  delete dict;
}

//-----------------------------------------------------------------------------

unsigned long KMMsgDict::getNextMsgSerNum() {
  unsigned long msn = nextMsgSerNum;
  nextMsgSerNum++;
  return msn;
}

void KMMsgDict::deleteRentry(KMMsgDictREntry *entry)
{
  delete entry;
}

//-----------------------------------------------------------------------------
unsigned long KMMsgDict::insert(unsigned long msn, const KMMessage * msg, int idx ) {
  return insert( msn, &msg->toMsgBase(), idx );
}


unsigned long KMMsgDict::insert(unsigned long msgSerNum,
                                const KMMsgBase *msg, int index)
{
  unsigned long msn = msgSerNum;
  if (!msn) {
    msn = getNextMsgSerNum();
  } else {
    if (msn >= nextMsgSerNum)
      nextMsgSerNum = msn + 1;
  }

  KMFolderIndex* folder = static_cast<KMFolderIndex*>( msg->storage() );
  if (folder && index == -1)
    index = folder->find(msg);

  // Should not happen, indicates id file corruption
  while (dict->find((long)msn)) {
    msn = getNextMsgSerNum();
    folder->setDirty( true ); // rewrite id file
  }

  // Insert into the dict. Don't use dict->replace() as we _know_
  // there is no entry with the same msn, we just made sure.
  KMMsgDictEntry *entry = new KMMsgDictEntry(folder->folder(), index);
  dict->insert((long)msn, entry);

  KMMsgDictREntry *rentry = folder->rDict();
  if (!rentry) {
    rentry = new KMMsgDictREntry();
    folder->setRDict(rentry);
  }
  rentry->set(index, entry);

  return msn;
}

unsigned long KMMsgDict::insert(const KMMsgBase *msg, int index)
{
  unsigned long msn = msg->getMsgSerNum();
  return insert(msn, msg, index);
}

//-----------------------------------------------------------------------------

void KMMsgDict::replace(unsigned long msgSerNum,
		       const KMMsgBase *msg, int index)
{
  KMFolderIndex* folder = static_cast<KMFolderIndex*>( msg->storage() );
  if ( folder && index == -1 )
    index = folder->find( msg );

  remove( msgSerNum );
  KMMsgDictEntry *entry = new KMMsgDictEntry( folder->folder(), index );
  dict->insert( (long)msgSerNum, entry );

  KMMsgDictREntry *rentry = folder->rDict();
  if (!rentry) {
    rentry = new KMMsgDictREntry();
    folder->setRDict(rentry);
  }
  rentry->set(index, entry);

}

//-----------------------------------------------------------------------------

void KMMsgDict::remove(unsigned long msgSerNum)
{
  long key = (long)msgSerNum;
  KMMsgDictEntry *entry = (KMMsgDictEntry *)dict->find(key);
  if (!entry)
    return;

  if (entry->folder) {
    KMMsgDictREntry *rentry = entry->folder->rDict();
    if (rentry)
      rentry->set(entry->index, 0);
  }

  dict->remove((long)key);
}

unsigned long KMMsgDict::remove(const KMMsgBase *msg)
{
  unsigned long msn = msg->getMsgSerNum();
  remove(msn);
  return msn;
}

//-----------------------------------------------------------------------------

void KMMsgDict::update(const KMMsgBase *msg, int index, int newIndex)
{
  KMMsgDictREntry *rentry = msg->parent()->rDict();
  if (rentry) {
    KMMsgDictEntry *entry = rentry->get(index);
    if (entry) {
      entry->index = newIndex;
      rentry->set(index, 0);
      rentry->set(newIndex, entry);
    }
  }
}

//-----------------------------------------------------------------------------

void KMMsgDict::getLocation(unsigned long key,
                            KMFolder **retFolder, int *retIndex)
{
  KMMsgDictEntry *entry = (KMMsgDictEntry *)dict->find((long)key);
  if (entry) {
    *retFolder = (KMFolder *)entry->folder;
    *retIndex = entry->index;
  } else {
    *retFolder = 0;
    *retIndex = -1;
  }
}

void KMMsgDict::getLocation(const KMMsgBase *msg,
                            KMFolder **retFolder, int *retIndex)
{
  getLocation(msg->getMsgSerNum(), retFolder, retIndex);
}

void KMMsgDict::getLocation( const KMMessage * msg, KMFolder * *retFolder, int * retIndex ) {
  getLocation( msg->toMsgBase().getMsgSerNum(), retFolder, retIndex );
}

//-----------------------------------------------------------------------------

unsigned long KMMsgDict::getMsgSerNum(KMFolder *folder, int index)
{
  unsigned long msn = 0;
  KMMsgDictREntry *rentry = folder->rDict();
  if (rentry)
    msn = rentry->getMsn(index);
  return msn;
}

//-----------------------------------------------------------------------------

QString KMMsgDict::getFolderIdsLocation(const KMFolder *folder)
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

int KMMsgDict::readFolderIds(KMFolder *folder)
{
  if (isFolderIdsOutdated(folder))
    return -1;

  QString filename = getFolderIdsLocation(folder);
  FILE *fp = fopen(QFile::encodeName(filename), "r+");
  if (!fp)
    return -1;

  int version = 0;
  fscanf(fp, IDS_HEADER, &version);
  if (version != IDS_VERSION) {
    fclose(fp);
    return -1;
  }

  bool swapByteOrder;
  Q_UINT32 byte_order;
  if (!fread(&byte_order, sizeof(byte_order), 1, fp)) {
    fclose(fp);
    return -1;
  }
  swapByteOrder = (byte_order == 0x78563412);

  Q_UINT32 count;
  if (!fread(&count, sizeof(count), 1, fp)) {
    fclose(fp);
    return -1;
  }
  if (swapByteOrder)
     count = kmail_swap_32(count);

  KMMsgDictREntry *rentry = new KMMsgDictREntry(count);

  for (unsigned int index = 0; index < count; index++) {
    Q_UINT32 msn;

    bool readOk = fread(&msn, sizeof(msn), 1, fp);
    if (swapByteOrder)
       msn = kmail_swap_32(msn);

    if (!readOk || dict->find(msn)) {
      for (unsigned int i = 0; i < index; i++) {
        msn = rentry->getMsn(i);
        dict->remove((long)msn);
      }
      delete rentry;
      fclose(fp);
      return -1;
    }

    //if (!msn)
      //kdDebug(5006) << "Dict found zero serial number in folder " << folder->label() << endl;

    // Insert into the dict. Don't use dict->replace() as we _know_
    // there is no entry with the same msn, we just made sure.
    KMMsgDictEntry *entry = new KMMsgDictEntry(folder, index);
    dict->insert((long)msn, entry);
    if (msn >= nextMsgSerNum)
      nextMsgSerNum = msn + 1;

    rentry->set(index, entry);
  }
  // Remember how many items we put into the dict this time so we can create 
  // it with an appropriate size next time.
  GlobalSettings::setMsgDictSizeHint( GlobalSettings::msgDictSizeHint() + count );

  fclose(fp);
  folder->setRDict(rentry);

  return 0;
}

//-----------------------------------------------------------------------------

KMMsgDictREntry *KMMsgDict::openFolderIds(KMFolder *folder, bool truncate)
{
  KMMsgDictREntry *rentry = folder->rDict();
  if (!rentry) {
    rentry = new KMMsgDictREntry();
    folder->setRDict(rentry);
  }

  if (!rentry->fp) {
    QString filename = getFolderIdsLocation(folder);
    FILE *fp = truncate ? 0 : fopen(QFile::encodeName(filename), "r+");
    if (fp)
    {
      int version = 0;
      fscanf(fp, IDS_HEADER, &version);
      if (version == IDS_VERSION)
      {
         Q_UINT32 byte_order = 0;
         fread(&byte_order, sizeof(byte_order), 1, fp);
         rentry->swapByteOrder = (byte_order == 0x78563412);
      }
      else
      {
         fclose(fp);
         fp = 0;
      }
    }

    if (!fp)
    {
      fp = fopen(QFile::encodeName(filename), "w+");
      if (!fp)
      {
        kdDebug(5006) << "Dict '" << filename
                      << "' cannot open with folder " << folder->label() << ": "
                      << strerror(errno) << " (" << errno << ")" << endl;
         delete rentry;
         rentry = 0;
         return 0;
      }
      fprintf(fp, IDS_HEADER, IDS_VERSION);
      Q_UINT32 byteOrder = 0x12345678;
      fwrite(&byteOrder, sizeof(byteOrder), 1, fp);
      rentry->swapByteOrder = false;
    }
    rentry->baseOffset = ftell(fp);
    rentry->fp = fp;
  }

  return rentry;
}

//-----------------------------------------------------------------------------

int KMMsgDict::writeFolderIds(KMFolder *folder)
{
  KMMsgDictREntry *rentry = openFolderIds(folder, true);
  if (!rentry)
    return 0;
  FILE *fp = rentry->fp;

  fseek(fp, rentry->baseOffset, SEEK_SET);
  // kdDebug(5006) << "Dict writing for folder " << folder->label() << endl;
  Q_UINT32 count = rentry->getRealSize();
  if (!fwrite(&count, sizeof(count), 1, fp)) {
    kdDebug(5006) << "Dict cannot write count with folder " << folder->label() << ": "
                  << strerror(errno) << " (" << errno << ")" << endl;
    return -1;
  }

  for (unsigned int index = 0; index < count; index++) {
    Q_UINT32 msn = rentry->getMsn(index);
    if (!fwrite(&msn, sizeof(msn), 1, fp))
      return -1;
  }

  rentry->sync();

  off_t eof = ftell(fp);
  QString filename = getFolderIdsLocation(folder);
  truncate(QFile::encodeName(filename), eof);
  fclose(rentry->fp);
  rentry->fp = 0;

  return 0;
}

//-----------------------------------------------------------------------------

int KMMsgDict::touchFolderIds(KMFolder *folder)
{
  KMMsgDictREntry *rentry = openFolderIds(folder, false);
  if (rentry) {
    rentry->sync();
    fclose(rentry->fp);
    rentry->fp = 0;
  }
  return 0;
}

//-----------------------------------------------------------------------------

int KMMsgDict::appendtoFolderIds(KMFolder *folder, int index)
{
  KMMsgDictREntry *rentry = openFolderIds(folder, false);
  if (!rentry)
    return 0;
  FILE *fp = rentry->fp;

//  kdDebug(5006) << "Dict appending for folder " << folder->label() << endl;

  fseek(fp, rentry->baseOffset, SEEK_SET);
  Q_UINT32 count;
  if (!fread(&count, sizeof(count), 1, fp)) {
    kdDebug(5006) << "Dict cannot read count for folder " << folder->label() << ": "
                  << strerror(errno) << " (" << errno << ")" << endl;
    return 0;
  }
  if (rentry->swapByteOrder)
     count = kmail_swap_32(count);

  count++;

  if (rentry->swapByteOrder)
     count = kmail_swap_32(count);
  fseek(fp, rentry->baseOffset, SEEK_SET);
  if (!fwrite(&count, sizeof(count), 1, fp)) {
    kdDebug(5006) << "Dict cannot write count for folder " << folder->label() << ": "
                  << strerror(errno) << " (" << errno << ")" << endl;
    return 0;
  }

  long ofs = (count - 1) * sizeof(ulong);
  if (ofs > 0)
    fseek(fp, ofs, SEEK_CUR);

  Q_UINT32 msn = rentry->getMsn(index);
  if (rentry->swapByteOrder)
     msn = kmail_swap_32(msn);
  if (!fwrite(&msn, sizeof(msn), 1, fp)) {
    kdDebug(5006) << "Dict cannot write count for folder " << folder->label() << ": "
                  << strerror(errno) << " (" << errno << ")" << endl;
    return 0;
  }

  rentry->sync();
  fclose(rentry->fp);
  rentry->fp = 0;

  return 0;
}

//-----------------------------------------------------------------------------

bool KMMsgDict::hasFolderIds(const KMFolder *folder)
{
  return folder->rDict() != 0;
}

//-----------------------------------------------------------------------------

bool KMMsgDict::removeFolderIds(KMFolder *folder)
{
  folder->setRDict(0);
  QString filename = getFolderIdsLocation(folder);
  return unlink(QFile::encodeName(filename));
}
