/* kmail message dictionary */
/* Author: Ronen Tzur <rtzur@shani.net> */

#include "kmmsgdict.h"

#include "kmfolder.h"
#include "globalsettings.h"

#include <kdebug.h>
#include <kde_file.h>
#include <kglobal.h>
#include <kmime/kmime_message.h>

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
class KMMsgDictEntry //: public KMDictItem
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
#if 0
    KMMsgDictEntry *entry = get(index);
    if (entry)
      return entry->key;
#endif
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
  //dict = new KMDict( lastSizeOfDict );
  nextMsgSerNum = 1;
}

//-----------------------------------------------------------------------------

KMMsgDict::~KMMsgDict()
{
  //delete dict;
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
                                const KMime::Content *msg, int index)
{
#if 0 //TODO port to akonadi
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
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
    return 0;
#endif
}

unsigned long KMMsgDict::insert(const KMime::Content *msg, int index)
{
#if 0 //TODO port to akonadi
  unsigned long msn = msg->getMsgSerNum();
  return insert(msn, msg, index);
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
	return 0;
#endif
}

//-----------------------------------------------------------------------------

void KMMsgDict::replace(unsigned long msgSerNum,
		       const KMime::Content *msg, int index)
{
#if 0 //TODO port to akonadi
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
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

//-----------------------------------------------------------------------------

void KMMsgDict::remove(unsigned long msgSerNum)
{
#if 0
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
#endif
}

unsigned long KMMsgDict::remove(const KMime::Content *msg)
{
#if 0 //TODO port to akonadi
  unsigned long msn = msg->getMsgSerNum();
  remove(msn);
  return msn;
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
	return 0;
#endif
}

//-----------------------------------------------------------------------------

void KMMsgDict::update(const KMime::Content *msg, int index, int newIndex)
{
#if 0 //TODO port to akonadi
  KMMsgDictREntry *rentry = msg->parent()->storage()->rDict();
  if (rentry) {
    KMMsgDictEntry *entry = rentry->get(index);
    if (entry) {
      entry->index = newIndex;
      rentry->set(index, 0);
      rentry->set(newIndex, entry);
    }
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

//-----------------------------------------------------------------------------

void KMMsgDict::getLocation(unsigned long key,
                            KMFolder **retFolder, int *retIndex) const
{
#if 0
  KMMsgDictEntry *entry = (KMMsgDictEntry *)dict->find((long)key);
  if (entry) {
    *retFolder = (KMFolder *)entry->folder;
    *retIndex = entry->index;
  } else {
    *retFolder = 0;
    *retIndex = -1;
  }
#endif
}

void KMMsgDict::getLocation(const KMime::Content *msg,
                            KMFolder **retFolder, int *retIndex) const
{
#if 0 //TODO port to akonadi
  getLocation(msg->getMsgSerNum(), retFolder, retIndex);
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

void KMMsgDict::getLocation( const KMime::Message * msg, KMFolder * *retFolder, int * retIndex ) const
{
#if 0 //TODO port to akonadi
  getLocation( msg->toMsgBase().getMsgSerNum(), retFolder, retIndex );
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
}

//-----------------------------------------------------------------------------

unsigned long KMMsgDict::getMsgSerNum(KMFolder *folder, int index) const
{
#if 0
  unsigned long msn = 0;
  if ( folder ) {
    KMMsgDictREntry *rentry = folder->storage()->rDict();
    if (rentry)
      msn = rentry->getMsn(index);
  }
  return msn;
#endif
  return 0;
}

//-----------------------------------------------------------------------------

//static
QList<unsigned long> KMMsgDict::serNumList(QList<KMime::Content *> msgList)
{
  QList<unsigned long> ret;
#if 0 //TODO port to akonadi
 for ( int i = 0; i < msgList.count(); i++ ) {
    unsigned long serNum = msgList.at(i)->getMsgSerNum();
    assert( serNum );
    ret.append( serNum );
  }
#else
    kDebug() << "AKONADI PORT: Disabled code in  " << Q_FUNC_INFO;
#endif
  return ret;
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

