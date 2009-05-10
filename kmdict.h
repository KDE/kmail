/*
 * simple hash table for kmail.  inspired by QDict
 */

#ifndef __KMDICT
#define __KMDICT

/**
 * @short Class representing items in a KMDict
 */
class KMDictItem
{
public:
  long key;
  KMDictItem *next;
};

/**
 * @short KMDict implements a lightweight dictionary with serial numbers as keys.
 *
 * KMDict is a leightweight dictionary used exclusively by KMMsgDict. It uses
 * serial numbers as keys.
 *
 * @author  Ronen Tzur <rtzur@shani.net>
 */
class KMDict
{
  friend class MessageDictTester;
public:
  /** Creates a hash table with @p size columns. */
  KMDict(int size = 17);

  /** Destroys the hash table object. */
  ~KMDict();

  /** Clears the hash table, removing all items. */
  void clear();

  /** Returns the size of the hash table. */
  int size() { return mSize; }

  /** Inserts an item, replacing old ones with the same key. */
  void replace(long key, KMDictItem *item);

  /** Inserts an item without replacing ones with the same key. */
  void insert(long key, KMDictItem *item);

  /** Removes an item. */
  void remove(long key);

  /** Find an item by key.  Returns pointer to it, or 0 if not found. */
  KMDictItem *find(long key);

private:
  /** Removes all items _following_ @p item with key @p key. */
  void removeFollowing(KMDictItem *item, long key);

  /** Initializes the hash table to @p size columns. */
  void init(int size);

  /** The size of the hash. */
  int mSize;

  /** The buckets. */
  KMDictItem **mVecs;
};

#endif /* __KMDICT */
