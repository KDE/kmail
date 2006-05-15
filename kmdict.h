/* simple hash table for kmail.  inspired by QDict
 */

#ifndef __KMDICT
#define __KMDICT

#include <algorithm>

namespace KMail {
// List of prime numbers shamelessly stolen from GCC STL
enum { num_primes = 29 };

static const unsigned long prime_list[ num_primes ] =
{
  31ul,        53ul,         97ul,         193ul,       389ul,
  769ul,       1543ul,       3079ul,       6151ul,      12289ul,
  24593ul,     49157ul,      98317ul,      196613ul,    393241ul,
  786433ul,    1572869ul,    3145739ul,    6291469ul,   12582917ul,
  25165843ul,  50331653ul,   100663319ul,  201326611ul, 402653189ul,
  805306457ul, 1610612741ul, 3221225473ul, 4294967291ul
};

inline unsigned long nextPrime( unsigned long n )
{
  const unsigned long *first = prime_list;
  const unsigned long *last = prime_list + num_primes;
  const unsigned long *pos = std::lower_bound( first, last, n );
  return pos == last ? *( last - 1 ) : *pos;
}

}

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
 
  /** Inserts an item without replacing ones with the same key. */
  void insert(long key, KMDictItem *item);
 
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
