/* kmail message dictionary */
/* Author: Ronen Tzur <rtzur@shani.net> */

#include "kmmsgdict.h"

#include "kmfolderindex.h"
#include "kmfolder.h"
#include "kmdict.h"
#include "globalsettings.h"
#include "folderstorage.h"

#include <kdebug.h>
#include <kde_file.h>
#include <kglobal.h>

#include <QFileInfo>
#include <QVector>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <config-kmail.h>

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

/**
 * @short an entry in the global message dictionary consisting of a pointer
 * to a folder and the index of a message in the folder.
 */
class KMMsgDictEntry : public KMDictItem
{
public:
  KMMsgDictEntry(const KMFolder *aFolder, int aIndex)
  : folder( aFolder ), index( aIndex )
    {}

  const KMFolder *folder;
  int index;
};

/**
 * @short A "reverse entry", consisting of an array of DictEntry pointers.
 *
 * Each folder (storage) holds such an entry. That's useful for looking up the
 * serial number of a message at a certain index in the folder, since that is the
 * key of these entries.
 */
class KMMsgDictREntry
{
public:
  KMMsgDictREntry(int size = 0)
  {
    array.resize(size);
    memset(array.data(), 0, array.size() * sizeof(KMMsgDictEntry *));  // faster than a loop
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
        int newsize = qMax(size + 25, index + 1);
        array.resize(newsize);
        for (int j = size; j < newsize; j++)
          array[j] = 0;
      }
      array[index] = entry;
    }
  }

  KMMsgDictEntry *get(int index)
  {
    if (index >= 0 && index < array.size())
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

  FILE *fp;
  bool swapByteOrder;
  off_t baseOffset;

private:

  QVector<KMMsgDictEntry *> array;
};


class KMMsgDictSingletonProvider
{
  public:
    KMMsgDict instance;
};

K_GLOBAL_STATIC( KMMsgDictSingletonProvider, theKMMsgDictSingletonProvider )

//-----------------------------------------------------------------------------

KMMsgDict::KMMsgDict()
{
  int lastSizeOfDict = GlobalSettings::self()->msgDictSizeHint();
  lastSizeOfDict = ( lastSizeOfDict * 11 ) / 10;
  GlobalSettings::self()->setMsgDictSizeHint( 0 );
  dict = new KMDict( lastSizeOfDict );
  nextMsgSerNum = 1;
}

//-----------------------------------------------------------------------------

KMMsgDict::~KMMsgDict()
{
  delete dict;
}

//-----------------------------------------------------------------------------

const KMMsgDict* KMMsgDict::instance()
{
  // better safe than sorry; check whether the global static has already been destroyed
  if ( theKMMsgDictSingletonProvider.isDestroyed() )
  {
    return 0;
  }
  return &theKMMsgDictSingletonProvider->instance;
}

KMMsgDict* KMMsgDict::mutableInstance()
{
  // better safe than sorry; check whether the global static has already been destroyed
  if ( theKMMsgDictSingletonProvider.isDestroyed() )
  {
    return 0;
  }
  return &theKMMsgDictSingletonProvider->instance;
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
  if ( !folder ) {
    kDebug() << "Cannot insert the message,"
      << "null pointer to storage. Requested serial: " << msgSerNum;
    kDebug() << " Message info: Subject:" << msg->subject() <<", To:"
      << msg->toStrip() << ", Date:" << msg->dateStr();
    return 0;
  }
  if (index == -1)
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
  if ( !folder ) {
    kDebug() << "Cannot replace the message serial"
      << "number, null pointer to storage. Requested serial: " << msgSerNum;
    kDebug() << " Message info: Subject:" << msg->subject() <<", To:"
      << msg->toStrip() << ", Date:" << msg->dateStr();
    return;
  }
  if ( index == -1 )
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
    KMMsgDictREntry *rentry = entry->folder->storage()->rDict();
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
  KMMsgDictREntry *rentry = msg->parent()->storage()->rDict();
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
                            KMFolder **retFolder, int *retIndex) const
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
                            KMFolder **retFolder, int *retIndex) const
{
  getLocation(msg->getMsgSerNum(), retFolder, retIndex);
}

void KMMsgDict::getLocation( const KMMessage * msg, KMFolder * *retFolder, int * retIndex ) const
{
  getLocation( msg->toMsgBase().getMsgSerNum(), retFolder, retIndex );
}

//-----------------------------------------------------------------------------

unsigned long KMMsgDict::getMsgSerNum(KMFolder *folder, int index) const
{
  unsigned long msn = 0;
  if ( folder ) {
    KMMsgDictREntry *rentry = folder->storage()->rDict();
    if (rentry)
      msn = rentry->getMsn(index);
  }
  return msn;
}

//-----------------------------------------------------------------------------

//static
QList<unsigned long> KMMsgDict::serNumList(QList<KMMsgBase *> msgList)
{
  QList<unsigned long> ret;
  for ( int i = 0; i < msgList.count(); i++ ) {
    unsigned long serNum = msgList.at(i)->getMsgSerNum();
    assert( serNum );
    ret.append( serNum );
  }
  return ret;
}

//-----------------------------------------------------------------------------

QString KMMsgDict::getFolderIdsLocation( const FolderStorage &storage )
{
  return storage.idsLocation();
}

//-----------------------------------------------------------------------------

bool KMMsgDict::isFolderIdsOutdated( const FolderStorage &storage )
{
  bool outdated = false;

  QFileInfo indexInfo( storage.indexLocation() );
  QFileInfo idsInfo( getFolderIdsLocation( storage ) );

  if (!indexInfo.exists() || !idsInfo.exists())
    outdated = true;
  if (indexInfo.lastModified() > idsInfo.lastModified())
    outdated = true;

  return outdated;
}

//-----------------------------------------------------------------------------

int KMMsgDict::readFolderIds( FolderStorage& storage )
{
  if ( isFolderIdsOutdated( storage ) )
    return -1;

  QString filename = getFolderIdsLocation( storage );
  FILE *fp = KDE_fopen(QFile::encodeName(filename), "r+");
  if (!fp)
    return -1;

  int version = 0;
  fscanf(fp, IDS_HEADER, &version);
  if (version != IDS_VERSION) {
    fclose(fp);
    return -1;
  }

  bool swapByteOrder;
  quint32 byte_order;
  if (!fread(&byte_order, sizeof(byte_order), 1, fp)) {
    fclose(fp);
    return -1;
  }
  swapByteOrder = (byte_order == 0x78563412);

  quint32 count;
  if (!fread(&count, sizeof(count), 1, fp)) {
    fclose(fp);
    return -1;
  }
  if (swapByteOrder)
     count = kmail_swap_32(count);

  // quick consistency check to avoid allocating huge amount of memory
  // due to reading corrupt file (#71549)
  long pos = KDE_ftell( fp );       // store current position
  KDE_fseek( fp, 0, SEEK_END );
  long fileSize = KDE_ftell( fp );  // how large is the file ?
  KDE_fseek( fp, pos, SEEK_SET );   // back to previous position

  // the file must at least contain what we try to read below
  if ( (fileSize - pos) < ( long )(count * sizeof(quint32)) ) {
    fclose(fp);
    return -1;
  }

  KMMsgDictREntry *rentry = new KMMsgDictREntry(count);

  for (unsigned int index = 0; index < count; index++) {
    quint32 msn;

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

    // We found a serial number that is zero. This is not allowed, and would
    // later cause problems like in bug 149715.
    // Therefore, use a fresh serial number instead
    if ( msn == 0 ) {
      kWarning() << "Found serial number zero at index" << index << "in folder" << filename;
      msn = getNextMsgSerNum();
      Q_ASSERT( msn != 0 );
    }

    // Insert into the dict. Don't use dict->replace() as we _know_
    // there is no entry with the same msn, we just made sure.
    KMMsgDictEntry *entry = new KMMsgDictEntry( storage.folder(), index);
    dict->insert((long)msn, entry);
    if (msn >= nextMsgSerNum)
      nextMsgSerNum = msn + 1;

    rentry->set(index, entry);
  }
  // Remember how many items we put into the dict this time so we can create
  // it with an appropriate size next time.
  GlobalSettings::setMsgDictSizeHint( GlobalSettings::msgDictSizeHint() + count );

  fclose(fp);
  storage.setRDict(rentry);

  return 0;
}

//-----------------------------------------------------------------------------

KMMsgDictREntry *KMMsgDict::openFolderIds( const FolderStorage& storage, bool truncate)
{
  KMMsgDictREntry *rentry = storage.rDict();
  if (!rentry) {
    rentry = new KMMsgDictREntry();
    storage.setRDict(rentry);
  }

  if (!rentry->fp) {
    QString filename = getFolderIdsLocation( storage );
    FILE *fp = truncate ? 0 : KDE_fopen(QFile::encodeName(filename), "r+");
    if (fp)
    {
      int version = 0;
      fscanf(fp, IDS_HEADER, &version);
      if (version == IDS_VERSION)
      {
         quint32 byte_order = 0;
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
      fp = KDE_fopen(QFile::encodeName(filename), "w+");
      if (!fp)
      {
        kDebug() << "Dict '" << filename
                      << "' cannot open with folder" << storage.label() << ":"
                      << strerror(errno) << "(" << errno << ")";
         delete rentry;
         storage.setRDict(0);
         return 0;
      }
      fprintf(fp, IDS_HEADER, IDS_VERSION);
      quint32 byteOrder = 0x12345678;
      fwrite(&byteOrder, sizeof(byteOrder), 1, fp);
      rentry->swapByteOrder = false;
    }
    rentry->baseOffset = KDE_ftell( fp );
    rentry->fp = fp;
  }

  return rentry;
}

//-----------------------------------------------------------------------------

int KMMsgDict::writeFolderIds( const FolderStorage &storage )
{
  KMMsgDictREntry *rentry = openFolderIds( storage, true );
  if (!rentry)
    return 0;
  FILE *fp = rentry->fp;

  KDE_fseek( fp, rentry->baseOffset, SEEK_SET );
  // kDebug() << "Dict writing for folder" << storage.label();
  quint32 count = rentry->getRealSize();
  if (!fwrite(&count, sizeof(count), 1, fp)) {
    kDebug() << "Dict cannot write count with folder" << storage.label() <<":"
                  << strerror(errno) << "(" << errno << ")";
    return -1;
  }

  for (unsigned int index = 0; index < count; index++) {
    quint32 msn = rentry->getMsn(index);
    if (!fwrite(&msn, sizeof(msn), 1, fp))
      return -1;
    if ( msn == 0 ) {
      kWarning() << "Serial number of message at index" << index << "is zero in folder" << storage.label();
    }
  }

  rentry->sync();

  off_t eof = KDE_ftell( fp );
  QString filename = getFolderIdsLocation( storage );
  if ( fclose( rentry->fp ) != 0 ) {
    return -1;
  }
  if ( truncate( QFile::encodeName( filename ), eof ) != 0 ) {
    return -1;
  }
  rentry->fp = 0;

  return 0;
}

//-----------------------------------------------------------------------------

int KMMsgDict::touchFolderIds( const FolderStorage &storage )
{
  KMMsgDictREntry *rentry = openFolderIds( storage, false);
  if (rentry) {
    rentry->sync();
    fclose(rentry->fp);
    rentry->fp = 0;
  }
  return 0;
}

//-----------------------------------------------------------------------------

int KMMsgDict::appendToFolderIds( FolderStorage& storage, int index)
{
  KMMsgDictREntry *rentry = openFolderIds( storage, false);
  if (!rentry)
    return 0;
  FILE *fp = rentry->fp;

//  kDebug() << "Dict appending for folder" << storage.label();

  KDE_fseek( fp, rentry->baseOffset, SEEK_SET );
  quint32 count;
  if (!fread(&count, sizeof(count), 1, fp)) {
    kDebug() << "Dict cannot read count for folder" << storage.label() <<":"
                  << strerror(errno) << "(" << errno << ")";
    return 0;
  }
  if (rentry->swapByteOrder)
     count = kmail_swap_32(count);

  count++;

  if (rentry->swapByteOrder)
     count = kmail_swap_32(count);
  KDE_fseek( fp, rentry->baseOffset, SEEK_SET );
  if (!fwrite(&count, sizeof(count), 1, fp)) {
    kDebug() << "Dict cannot write count for folder" << storage.label() <<":"
                  << strerror(errno) << "(" << errno << ")";
    return 0;
  }

  long ofs = (count - 1) * sizeof(ulong);
  if ( ofs > 0 ) {
    KDE_fseek( fp, ofs, SEEK_CUR );
  }

  quint32 msn = rentry->getMsn(index);
  if (rentry->swapByteOrder)
     msn = kmail_swap_32(msn);
  if (!fwrite(&msn, sizeof(msn), 1, fp)) {
    kDebug() << "Dict cannot write count for folder" << storage.label() <<":"
                  << strerror(errno) << "(" << errno << ")";
    return 0;
  }

  rentry->sync();
  fclose(rentry->fp);
  rentry->fp = 0;

  return 0;
}

//-----------------------------------------------------------------------------

bool KMMsgDict::hasFolderIds( const FolderStorage& storage )
{
  return storage.rDict() != 0;
}

//-----------------------------------------------------------------------------

bool KMMsgDict::removeFolderIds( FolderStorage& storage )
{
  storage.setRDict( 0 );
  QString filename = getFolderIdsLocation( storage );
  return unlink( QFile::encodeName( filename) );
}
