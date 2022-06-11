#ifdef __APPLE__
# ifdef __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# else
#  define WORDS_BIGENDIAN 0
# endif
#else
#cmakedefine WORDS_BIGENDIAN ${CMAKE_WORDS_BIGENDIAN}
#endif
