/* simple hash table for kmail.  inspired by QDict
 */

#ifndef __KMDICT
#define __KMDICT


class KMDictItem
{
public:
  long key;
  KMDictItem *next;
};


class KMDict
{
public:
  /** Creates a hash table with @p size columns. */
  KMDict(int size = 17);
  
  /** Destroys the hash table object. */
  ~KMDict();

  /** Initializes the hash table to @p size colums. */
  void init(int size);
  
  /** Clears the hash table, removing all items. */
  void clear();
  
  /** Returns the size of the hash table. */
  int size() { return mSize; }
  
  /** Inserts an item, replacing old ones with the same key. */
  void replace(long key, KMDictItem *item);
  
  /** Removes an item. */
  void remove(long key);
  
  /** Find an item by key.  Returns pointer to it, or 0 if not found. */
  KMDictItem *find(long key);
  
protected:
  /** Removes all items _following_ @p item with key @p key. */
  void removeFollowing(KMDictItem *item, long key);
  
  /** The size of the hash. */
  int mSize;
  
  /** The buckets. */
  KMDictItem **mVecs;
};

#endif /* __KMDICT */
