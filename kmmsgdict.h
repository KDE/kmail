/* kmail message dictionary
 * keeps location information for every message
 * the message serial number is the key for the dictionary
 */

#ifndef __KMMSGDICT
#define __KMMSGDICT

#include <qintdict.h>
#include <qptrdict.h>

class KMFolder;
class KMMsgBase;
class KMMsgDictEntry;


class KMMsgDict
{
public:
  KMMsgDict();
  ~KMMsgDict();
  
  /** Insert a new message.  The message serial number is specified in
   * @p msgSerNum and may be zero, in which case a new serial number is
   * generated.  Returns the message serial number. */
  unsigned long insert(unsigned long msgSerNum, const KMMsgBase *msg, int index = -1);
  
  /** Insert a new message.  The message serial number is taken from
   * the message, and passed to the other insert().  Returns the message
   * serial number. */
  unsigned long insert(const KMMsgBase *msg, int index = -1);
  
  /** Removes a message. */
  void remove(unsigned long msgSerNum);

  /** Removes a message, and returns its message serial number. */
  unsigned long remove(const KMMsgBase *msg);
  
  /** Returns (folder,index) pair for a message. */
  void getLocation(unsigned long key, KMFolder **retFolder, int *retIndex);
  void getLocation(const KMMsgBase *msg, KMFolder **retFolder, int *retIndex);
  
  /** Returns a message serial number for a (folder,index) pair.  Zero
    if no such message. */
  unsigned long getMsgSerNum(KMFolder *folder, int index);
  
  /** Reads the .folder.index.ids file.  Returns 0 on success. */
  int readFolderIds(KMFolder *folder);
  
  /** Writes the .folder.index.ids file.  Returns 0 on success. */
  int writeFolderIds(KMFolder *folder);
  
  /** Returns true if the folder has a .folder.index.ids file.  */
  bool hasFolderIds(KMFolder *folder);
  
protected:
  /** Initialize (to zero) messages in a folder array. */
  void initArray(QMemArray<unsigned long> *array, int start = 0);
  
  /** Returns the next message serial number for use. */
  unsigned long getNextMsgSerNum();
  
  /** The dictionary. */
  QIntDict<KMMsgDictEntry> *dict;
  /** The reverse-dictionary. */
  QPtrDict<QMemArray<unsigned long> > *rdict;

  /** Highest message serial number we know of. */
  unsigned long nextMsgSerNum;
};

#endif /* __KMMSGDICT */
