#ifndef __KDEPIM__KMAIL__STL_UTIL_H__
#define __KDEPIM__KMAIL__STL_UTIL_H__

template <typename T>
static inline void DeleteAndSetToZero( const T* & t ) {
  delete t; t = 0;
}

template <typename T>
static inline void deleteAll( T & c ) {
  for ( typename T::iterator it = c.begin() ; it != c.end() ; ++it ) {
    delete *it ; *it = 0;
  }
}

#endif // __KDEPIM__KMAIL__STL_UTIL_H__
